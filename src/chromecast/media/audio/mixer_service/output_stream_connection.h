// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_AUDIO_MIXER_SERVICE_OUTPUT_STREAM_CONNECTION_H_
#define CHROMECAST_MEDIA_AUDIO_MIXER_SERVICE_OUTPUT_STREAM_CONNECTION_H_

#include <cstdint>
#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "chromecast/media/audio/mixer_service/mixer_connection.h"
#include "chromecast/media/audio/mixer_service/mixer_service.pb.h"
#include "chromecast/media/audio/mixer_service/mixer_socket.h"
#include "net/base/io_buffer.h"

namespace net {
class StreamSocket;
}  // namespace net

namespace chromecast {
namespace media {
namespace mixer_service {

// Mixer service connection for an audio output stream. Not thread-safe; all
// usage of a given instance must be on the same sequence.
class OutputStreamConnection : public MixerConnection,
                               public MixerClientSocket::Delegate {
 public:
  class Delegate {
   public:
    // Called to fill more audio data. The implementation should write up to
    // |frames| frames of audio data into |buffer|, and then call
    // SendNextBuffer() with the actual number of frames that were filled (or
    // 0 to indicate end-of-stream). The |playout_timestamp| indicates the
    // audio clock timestamp in microseconds when the first frame of the filled
    // data is expected to play out.
    virtual void FillNextBuffer(void* buffer,
                                int frames,
                                int64_t playout_timestamp) = 0;

    // Called when the end of the stream has been played out. At this point it
    // is safe to delete the OutputStreamConnection without dropping any audio.
    virtual void OnEosPlayed() = 0;

   protected:
    virtual ~Delegate() = default;
  };

  OutputStreamConnection(Delegate* delegate, const OutputStreamParams& params);
  ~OutputStreamConnection() override;

  // Connects to the mixer. After this is called, delegate methods may start
  // to be called. If the mixer connection is lost, this will automatically
  // reconnect.
  void Connect();

  // Sends filled audio data (written into the buffer provided to
  // FillNextBuffer()) to the mixer. |filled_frames| indicates the number of
  // frames of audio that were actually written into the buffer, or 0 to
  // indicate end-of-stream. |pts| is the PTS of the first frame of filled
  // audio; this is only meaningful when params.use_start_timestamp is |true|.
  void SendNextBuffer(int filled_frames, int64_t pts = 0);

  // Sets the volume multiplier for this audio stream.
  void SetVolumeMultiplier(float multiplier);

  // Indicates that playback should (re)start playing PTS |pts| at time
  // |start_timestamp| in microseconds relative to the audio clock.
  void SetStartTimestamp(int64_t start_timestamp, int64_t pts);

  // Informs the mixer how fast the PTS increases per frame. For example if the
  // playback rate is 2.0, then each frame increases the PTS by
  // 2.0 / sample_rate seconds.
  void SetPlaybackRate(float playback_rate);

  // Pauses playback.
  void Pause();

  // Resumes playback.
  void Resume();

 private:
  // MixerConnection implementation:
  void OnConnected(std::unique_ptr<net::StreamSocket> socket) override;
  void OnConnectionError() override;

  // MixerClientSocket::Delegate implementation:
  void HandleMetadata(const Generic& message) override;

  Delegate* const delegate_;
  const OutputStreamParams params_;

  const int frame_size_;
  const int fill_size_frames_;
  const scoped_refptr<net::IOBuffer> audio_buffer_;

  std::unique_ptr<MixerClientSocket> socket_;
  float volume_multiplier_ = 1.0f;

  int64_t start_timestamp_ = INT64_MIN;
  int64_t start_pts_ = INT64_MIN;

  float playback_rate_ = 1.0f;

  bool paused_ = false;

  DISALLOW_COPY_AND_ASSIGN(OutputStreamConnection);
};

}  // namespace mixer_service
}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_AUDIO_MIXER_SERVICE_OUTPUT_STREAM_CONNECTION_H_
