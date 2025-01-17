// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/threading/thread.h"
#include "content/renderer/media/webrtc/media_stream_track_metrics.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/api/media_stream_interface.h"

using webrtc::AudioSourceInterface;
using webrtc::AudioTrackInterface;
using webrtc::AudioTrackSinkInterface;
using webrtc::MediaStreamInterface;
using webrtc::ObserverInterface;
using webrtc::PeerConnectionInterface;
using webrtc::VideoTrackSourceInterface;
using webrtc::VideoTrackInterface;

namespace content {

// A very simple mock that implements only the id() method.
class MockAudioTrackInterface : public AudioTrackInterface {
 public:
  explicit MockAudioTrackInterface(const std::string& id) : id_(id) {}
  ~MockAudioTrackInterface() override {}

  std::string id() const override { return id_; }

  MOCK_METHOD1(RegisterObserver, void(ObserverInterface*));
  MOCK_METHOD1(UnregisterObserver, void(ObserverInterface*));
  MOCK_CONST_METHOD0(kind, std::string());
  MOCK_CONST_METHOD0(enabled, bool());
  MOCK_CONST_METHOD0(state, TrackState());
  MOCK_METHOD1(set_enabled, bool(bool));
  MOCK_METHOD1(set_state, bool(TrackState));
  MOCK_CONST_METHOD0(GetSource, AudioSourceInterface*());
  MOCK_METHOD1(AddSink, void(AudioTrackSinkInterface*));
  MOCK_METHOD1(RemoveSink, void(AudioTrackSinkInterface*));

 private:
  std::string id_;
};

// A very simple mock that implements only the id() method.
class MockVideoTrackInterface : public VideoTrackInterface {
 public:
  explicit MockVideoTrackInterface(const std::string& id) : id_(id) {}
  ~MockVideoTrackInterface() override {}

  std::string id() const override { return id_; }

  MOCK_METHOD1(RegisterObserver, void(ObserverInterface*));
  MOCK_METHOD1(UnregisterObserver, void(ObserverInterface*));
  MOCK_CONST_METHOD0(kind, std::string());
  MOCK_CONST_METHOD0(enabled, bool());
  MOCK_CONST_METHOD0(state, TrackState());
  MOCK_METHOD1(set_enabled, bool(bool));
  MOCK_METHOD1(set_state, bool(TrackState));
  MOCK_METHOD2(AddOrUpdateSink,
               void(rtc::VideoSinkInterface<webrtc::VideoFrame>*,
                    const rtc::VideoSinkWants&));
  MOCK_METHOD1(RemoveSink, void(rtc::VideoSinkInterface<webrtc::VideoFrame>*));
  MOCK_CONST_METHOD0(GetSource, VideoTrackSourceInterface*());

 private:
  std::string id_;
};

class MockMediaStreamTrackMetrics : public MediaStreamTrackMetrics {
 public:
  virtual ~MockMediaStreamTrackMetrics() {}

  MOCK_METHOD4(SendLifetimeMessage,
               void(const std::string&, Kind, LifetimeEvent, Direction));
  using MediaStreamTrackMetrics::MakeUniqueIdImpl;
};

class MediaStreamTrackMetricsTest : public testing::Test {
 public:
  MediaStreamTrackMetricsTest()
      : task_environment_(
            base::test::SingleThreadTaskEnvironment::MainThreadType::UI),
        signaling_thread_("signaling_thread") {}

  void SetUp() override {
    metrics_.reset(new MockMediaStreamTrackMetrics());
    stream_ = new rtc::RefCountedObject<MockMediaStream>("stream");
    signaling_thread_.Start();
  }

  void TearDown() override {
    signaling_thread_.Stop();
    metrics_.reset();
    stream_ = nullptr;
  }

  // Adds an audio track to |stream_| on the signaling thread to simulate how
  // notifications will be fired in Chrome.
  template <typename TrackType>
  void AddTrack(TrackType* track) {
    // Explicitly casting to this type is necessary since the
    // MediaStreamInterface has two methods with the same name.
    typedef bool (MediaStreamInterface::*AddTrack)(TrackType*);
    base::RunLoop run_loop;
    signaling_thread_.task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(
            base::IgnoreResult<AddTrack>(&MediaStreamInterface::AddTrack),
            stream_, base::Unretained(track)),
        run_loop.QuitClosure());
    run_loop.Run();
  }

  template <typename TrackType>
  void RemoveTrack(TrackType* track) {
    // Explicitly casting to this type is necessary since the
    // MediaStreamInterface has two methods with the same name.
    typedef bool (MediaStreamInterface::*RemoveTrack)(TrackType*);
    base::RunLoop run_loop;
    signaling_thread_.task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(
            base::IgnoreResult<RemoveTrack>(&MediaStreamInterface::RemoveTrack),
            stream_, base::Unretained(track)),
        run_loop.QuitClosure());
    run_loop.Run();
  }

  // Convenience methods to cast the mock track types into their webrtc
  // equivalents.
  void AddAudioTrack(AudioTrackInterface* track) { AddTrack(track); }
  void RemoveAudioTrack(AudioTrackInterface* track) { RemoveTrack(track); }
  void AddVideoTrack(VideoTrackInterface* track) { AddTrack(track); }
  void RemoveVideoTrack(VideoTrackInterface* track) { RemoveTrack(track); }

  scoped_refptr<MockAudioTrackInterface> MakeAudioTrack(const std::string& id) {
    return new rtc::RefCountedObject<MockAudioTrackInterface>(id);
  }

  scoped_refptr<MockVideoTrackInterface> MakeVideoTrack(const std::string& id) {
    return new rtc::RefCountedObject<MockVideoTrackInterface>(id);
  }

  std::unique_ptr<MockMediaStreamTrackMetrics> metrics_;
  scoped_refptr<MediaStreamInterface> stream_;

  base::test::SingleThreadTaskEnvironment task_environment_;
  base::Thread signaling_thread_;
};

TEST_F(MediaStreamTrackMetricsTest, MakeUniqueId) {
  // The important testable properties of the unique ID are that it
  // should differ when any of the three constituents differ
  // (PeerConnection pointer, track ID, remote or not. Also, testing
  // that the implementation does not discard the upper 32 bits of the
  // PeerConnection pointer is important.
  //
  // The important hard-to-test property is that the ID be generated
  // using a hash function with virtually zero chance of
  // collisions. We don't test this, we rely on MD5 having this
  // property.

  // Lower 32 bits the same, upper 32 differ.
  EXPECT_NE(
      metrics_->MakeUniqueIdImpl(0x1000000000000001, "x",
                                 MediaStreamTrackMetrics::Direction::kReceive),
      metrics_->MakeUniqueIdImpl(0x2000000000000001, "x",
                                 MediaStreamTrackMetrics::Direction::kReceive));

  // Track ID differs.
  EXPECT_NE(metrics_->MakeUniqueIdImpl(
                42, "x", MediaStreamTrackMetrics::Direction::kReceive),
            metrics_->MakeUniqueIdImpl(
                42, "y", MediaStreamTrackMetrics::Direction::kReceive));

  // Remove vs. local track differs.
  EXPECT_NE(metrics_->MakeUniqueIdImpl(
                42, "x", MediaStreamTrackMetrics::Direction::kReceive),
            metrics_->MakeUniqueIdImpl(
                42, "x", MediaStreamTrackMetrics::Direction::kSend));
}

TEST_F(MediaStreamTrackMetricsTest, BasicRemoteStreams) {
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kReceive,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kReceive,
                     MediaStreamTrackMetrics::Kind::kVideo, "video");

  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "audio", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kReceive));
  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "video", MediaStreamTrackMetrics::Kind::kVideo,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kReceive));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("audio", MediaStreamTrackMetrics::Kind::kAudio,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kReceive));
  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("video", MediaStreamTrackMetrics::Kind::kVideo,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kReceive));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionDisconnected);
}

TEST_F(MediaStreamTrackMetricsTest, BasicLocalStreams) {
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kVideo, "video");

  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "audio", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "video", MediaStreamTrackMetrics::Kind::kVideo,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("audio", MediaStreamTrackMetrics::Kind::kAudio,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("video", MediaStreamTrackMetrics::Kind::kVideo,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  metrics_->IceConnectionChange(PeerConnectionInterface::kIceConnectionFailed);
}

TEST_F(MediaStreamTrackMetricsTest, LocalStreamAddedAferIceConnect) {
  metrics_->IceConnectionChange(
        PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "audio", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "video", MediaStreamTrackMetrics::Kind::kVideo,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));

  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kVideo, "video");
}

TEST_F(MediaStreamTrackMetricsTest, RemoteStreamAddedAferIceConnect) {
  metrics_->IceConnectionChange(
        PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "audio", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kReceive));
  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "video", MediaStreamTrackMetrics::Kind::kVideo,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kReceive));

  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kReceive,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kReceive,
                     MediaStreamTrackMetrics::Kind::kVideo, "video");
}

TEST_F(MediaStreamTrackMetricsTest, LocalStreamTrackRemoved) {
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kAudio, "first");
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kAudio, "second");

  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "first", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "second", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("first", MediaStreamTrackMetrics::Kind::kAudio,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  metrics_->RemoveTrack(MediaStreamTrackMetrics::Direction::kSend,
                        MediaStreamTrackMetrics::Kind::kAudio, "first");

  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("second", MediaStreamTrackMetrics::Kind::kAudio,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  metrics_->IceConnectionChange(PeerConnectionInterface::kIceConnectionFailed);
}

TEST_F(MediaStreamTrackMetricsTest, RemoveAfterDisconnect) {
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");

  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "audio", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("audio", MediaStreamTrackMetrics::Kind::kAudio,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  metrics_->IceConnectionChange(PeerConnectionInterface::kIceConnectionFailed);

  // This happens after the call is disconnected so no lifetime
  // message should be sent.
  metrics_->RemoveTrack(MediaStreamTrackMetrics::Direction::kSend,
                        MediaStreamTrackMetrics::Kind::kAudio, "audio");
}

TEST_F(MediaStreamTrackMetricsTest, RemoteStreamMultipleDisconnects) {
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kReceive,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");

  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "audio", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kReceive));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("audio", MediaStreamTrackMetrics::Kind::kAudio,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kReceive));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionDisconnected);
  metrics_->IceConnectionChange(PeerConnectionInterface::kIceConnectionFailed);
  metrics_->RemoveTrack(MediaStreamTrackMetrics::Direction::kReceive,
                        MediaStreamTrackMetrics::Kind::kAudio, "audio");
}

TEST_F(MediaStreamTrackMetricsTest, RemoteStreamConnectDisconnectTwice) {
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kReceive,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");

  for (size_t i = 0; i < 2; ++i) {
    EXPECT_CALL(
        *metrics_,
        SendLifetimeMessage("audio", MediaStreamTrackMetrics::Kind::kAudio,
                            MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                            MediaStreamTrackMetrics::Direction::kReceive));
    metrics_->IceConnectionChange(
        PeerConnectionInterface::kIceConnectionConnected);

    EXPECT_CALL(*metrics_,
                SendLifetimeMessage(
                    "audio", MediaStreamTrackMetrics::Kind::kAudio,
                    MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                    MediaStreamTrackMetrics::Direction::kReceive));
    metrics_->IceConnectionChange(
        PeerConnectionInterface::kIceConnectionDisconnected);
  }

  metrics_->RemoveTrack(MediaStreamTrackMetrics::Direction::kReceive,
                        MediaStreamTrackMetrics::Kind::kAudio, "audio");
}

TEST_F(MediaStreamTrackMetricsTest, LocalStreamRemovedNoDisconnect) {
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kVideo, "video");

  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "audio", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "video", MediaStreamTrackMetrics::Kind::kVideo,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("audio", MediaStreamTrackMetrics::Kind::kAudio,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("video", MediaStreamTrackMetrics::Kind::kVideo,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  metrics_->RemoveTrack(MediaStreamTrackMetrics::Direction::kSend,
                        MediaStreamTrackMetrics::Kind::kAudio, "audio");
  metrics_->RemoveTrack(MediaStreamTrackMetrics::Direction::kSend,
                        MediaStreamTrackMetrics::Kind::kVideo, "video");
}

TEST_F(MediaStreamTrackMetricsTest, LocalStreamLargerTest) {
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kVideo, "video");

  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "audio", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "video", MediaStreamTrackMetrics::Kind::kVideo,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("audio", MediaStreamTrackMetrics::Kind::kAudio,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  metrics_->RemoveTrack(MediaStreamTrackMetrics::Direction::kSend,
                        MediaStreamTrackMetrics::Kind::kAudio, "audio");

  // Add back audio
  EXPECT_CALL(*metrics_, SendLifetimeMessage(
                             "audio", MediaStreamTrackMetrics::Kind::kAudio,
                             MediaStreamTrackMetrics::LifetimeEvent::kConnected,
                             MediaStreamTrackMetrics::Direction::kSend));
  metrics_->AddTrack(MediaStreamTrackMetrics::Direction::kSend,
                     MediaStreamTrackMetrics::Kind::kAudio, "audio");

  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("audio", MediaStreamTrackMetrics::Kind::kAudio,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  metrics_->RemoveTrack(MediaStreamTrackMetrics::Direction::kSend,
                        MediaStreamTrackMetrics::Kind::kAudio, "audio");
  EXPECT_CALL(
      *metrics_,
      SendLifetimeMessage("video", MediaStreamTrackMetrics::Kind::kVideo,
                          MediaStreamTrackMetrics::LifetimeEvent::kDisconnected,
                          MediaStreamTrackMetrics::Direction::kSend));
  metrics_->RemoveTrack(MediaStreamTrackMetrics::Direction::kSend,
                        MediaStreamTrackMetrics::Kind::kVideo, "video");
}

}  // namespace content
