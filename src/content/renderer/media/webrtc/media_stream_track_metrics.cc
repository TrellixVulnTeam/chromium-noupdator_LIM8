// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/media_stream_track_metrics.h"

#include <inttypes.h>
#include <string>

#include "base/hash/md5.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/child_thread_impl.h"
#include "content/public/common/service_names.mojom.h"
#include "content/renderer/render_thread_impl.h"

namespace content {

class MediaStreamTrackMetricsObserver {
 public:
  MediaStreamTrackMetricsObserver(MediaStreamTrackMetrics::Direction direction,
                                  MediaStreamTrackMetrics::Kind kind,
                                  std::string track_id,
                                  MediaStreamTrackMetrics* owner);
  ~MediaStreamTrackMetricsObserver();

  // Sends begin/end messages for the track if not already reported.
  void SendLifetimeMessageForTrack(
      MediaStreamTrackMetrics::LifetimeEvent event);

  MediaStreamTrackMetrics::Direction direction() {
    DCHECK(thread_checker_.CalledOnValidThread());
    return direction_;
  }

  MediaStreamTrackMetrics::Kind kind() {
    DCHECK(thread_checker_.CalledOnValidThread());
    return kind_;
  }

  std::string track_id() const {
    DCHECK(thread_checker_.CalledOnValidThread());
    return track_id_;
  }

 private:
  // False until start/end of lifetime messages have been sent.
  bool has_reported_start_;
  bool has_reported_end_;

  MediaStreamTrackMetrics::Direction direction_;
  MediaStreamTrackMetrics::Kind kind_;
  std::string track_id_;

  // Non-owning.
  MediaStreamTrackMetrics* owner_;
  base::ThreadChecker thread_checker_;
};

namespace {

// Used with std::find_if.
struct ObserverFinder {
  ObserverFinder(MediaStreamTrackMetrics::Direction direction,
                 MediaStreamTrackMetrics::Kind kind,
                 const std::string& track_id)
      : direction_(direction), kind_(kind), track_id_(track_id) {}
  bool operator()(
      const std::unique_ptr<MediaStreamTrackMetricsObserver>& observer) {
    return direction_ == observer->direction() && kind_ == observer->kind() &&
           track_id_ == observer->track_id();
  }
  MediaStreamTrackMetrics::Direction direction_;
  MediaStreamTrackMetrics::Kind kind_;
  std::string track_id_;
};

}  // namespace

MediaStreamTrackMetricsObserver::MediaStreamTrackMetricsObserver(
    MediaStreamTrackMetrics::Direction direction,
    MediaStreamTrackMetrics::Kind kind,
    std::string track_id,
    MediaStreamTrackMetrics* owner)
    : has_reported_start_(false),
      has_reported_end_(false),
      direction_(direction),
      kind_(kind),
      track_id_(std::move(track_id)),
      owner_(owner) {
  DCHECK(owner);
}

MediaStreamTrackMetricsObserver::~MediaStreamTrackMetricsObserver() {
  DCHECK(thread_checker_.CalledOnValidThread());
  SendLifetimeMessageForTrack(
      MediaStreamTrackMetrics::LifetimeEvent::kDisconnected);
}

void MediaStreamTrackMetricsObserver::SendLifetimeMessageForTrack(
    MediaStreamTrackMetrics::LifetimeEvent event) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (event == MediaStreamTrackMetrics::LifetimeEvent::kConnected) {
    // Both ICE CONNECTED and COMPLETED can trigger the first
    // start-of-life event, so we only report the first.
    if (has_reported_start_)
      return;
    DCHECK(!has_reported_start_ && !has_reported_end_);
    has_reported_start_ = true;
  } else {
    DCHECK(event == MediaStreamTrackMetrics::LifetimeEvent::kDisconnected);

    // We only report the first end-of-life event, since there are
    // several cases where end-of-life can be reached. We also don't
    // report end unless we've reported start.
    if (has_reported_end_ || !has_reported_start_)
      return;
    has_reported_end_ = true;
  }

  owner_->SendLifetimeMessage(track_id_, kind_, event, direction_);

  if (event == MediaStreamTrackMetrics::LifetimeEvent::kDisconnected) {
    // After disconnection, we can get reconnected, so we need to
    // forget that we've sent lifetime events, while retaining all
    // other state.
    DCHECK(has_reported_start_ && has_reported_end_);
    has_reported_start_ = false;
    has_reported_end_ = false;
  }
}

MediaStreamTrackMetrics::MediaStreamTrackMetrics()
    : ice_state_(webrtc::PeerConnectionInterface::kIceConnectionNew) {}

MediaStreamTrackMetrics::~MediaStreamTrackMetrics() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& observer : observers_) {
    observer->SendLifetimeMessageForTrack(LifetimeEvent::kDisconnected);
  }
}

void MediaStreamTrackMetrics::AddTrack(Direction direction,
                                       Kind kind,
                                       const std::string& track_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.push_back(std::make_unique<MediaStreamTrackMetricsObserver>(
      direction, kind, std::move(track_id), this));
  SendLifeTimeMessageDependingOnIceState(observers_.back().get());
}

void MediaStreamTrackMetrics::RemoveTrack(Direction direction,
                                          Kind kind,
                                          const std::string& track_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = std::find_if(observers_.begin(), observers_.end(),
                         ObserverFinder(direction, kind, track_id));
  if (it == observers_.end()) {
    // Since external apps could call removeTrack() with a stream they
    // never added, this can happen without it being an error.
    return;
  }

  observers_.erase(it);
}

void MediaStreamTrackMetrics::IceConnectionChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ice_state_ = new_state;
  for (const auto& observer : observers_) {
    SendLifeTimeMessageDependingOnIceState(observer.get());
  }
}

void MediaStreamTrackMetrics::SendLifeTimeMessageDependingOnIceState(
    MediaStreamTrackMetricsObserver* observer) {
  // There is a state transition diagram for these states at
  // http://dev.w3.org/2011/webrtc/editor/webrtc.html#idl-def-RTCIceConnectionState
  switch (ice_state_) {
    case webrtc::PeerConnectionInterface::kIceConnectionConnected:
    case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
      observer->SendLifetimeMessageForTrack(LifetimeEvent::kConnected);
      break;

    case webrtc::PeerConnectionInterface::kIceConnectionFailed:
    // We don't really need to handle FAILED (it is only supposed
    // to be preceded by CHECKING so we wouldn't yet have sent a
    // lifetime message) but we might as well use belt and
    // suspenders and handle it the same as the other "end call"
    // states. It will be ignored anyway if the call is not
    // already connected.
    case webrtc::PeerConnectionInterface::kIceConnectionNew:
    // It's a bit weird to count NEW as an end-lifetime event, but
    // it's possible to transition directly from a connected state
    // (CONNECTED or COMPLETED) to NEW, which can then be followed
    // by a new connection. The observer will ignore the end
    // lifetime event if it was not preceded by a begin-lifetime
    // event.
    case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
    case webrtc::PeerConnectionInterface::kIceConnectionClosed:
      observer->SendLifetimeMessageForTrack(LifetimeEvent::kDisconnected);
      break;

    default:
      // We ignore the remaining state (CHECKING) as it is never
      // involved in a transition from connected to disconnected or
      // vice versa.
      break;
  }
}

void MediaStreamTrackMetrics::SendLifetimeMessage(const std::string& track_id,
                                                  Kind kind,
                                                  LifetimeEvent event,
                                                  Direction direction) {
  RenderThreadImpl* render_thread = RenderThreadImpl::current();
  // |render_thread| can be NULL in certain cases when running as part
  // |of a unit test.
  if (render_thread) {
    if (event == LifetimeEvent::kConnected) {
      GetMediaStreamTrackMetricsHost()->AddTrack(
          MakeUniqueId(track_id, direction), kind == Kind::kAudio,
          direction == Direction::kReceive);
    } else {
      DCHECK_EQ(LifetimeEvent::kDisconnected, event);
      GetMediaStreamTrackMetricsHost()->RemoveTrack(
          MakeUniqueId(track_id, direction));
    }
  }
}

uint64_t MediaStreamTrackMetrics::MakeUniqueIdImpl(uint64_t pc_id,
                                                   const std::string& track_id,
                                                   Direction direction) {
  // We use a hash over the |track| pointer and the PeerConnection ID,
  // plus a boolean flag indicating whether the track is remote (since
  // you might conceivably have a remote track added back as a sent
  // track) as the unique ID.
  //
  // We don't need a cryptographically secure hash (which MD5 should
  // no longer be considered), just one with virtually zero chance of
  // collisions when faced with non-malicious data.
  std::string unique_id_string =
      base::StringPrintf("%" PRIu64 " %s %d", pc_id, track_id.c_str(),
                         direction == Direction::kReceive ? 1 : 0);

  base::MD5Context ctx;
  base::MD5Init(&ctx);
  base::MD5Update(&ctx, unique_id_string);
  base::MD5Digest digest;
  base::MD5Final(&digest, &ctx);

  static_assert(sizeof(digest.a) > sizeof(uint64_t), "need a bigger digest");
  return *reinterpret_cast<uint64_t*>(digest.a);
}

uint64_t MediaStreamTrackMetrics::MakeUniqueId(const std::string& track_id,
                                               Direction direction) {
  return MakeUniqueIdImpl(
      reinterpret_cast<uint64_t>(reinterpret_cast<void*>(this)), track_id,
      direction);
}

mojo::Remote<blink::mojom::MediaStreamTrackMetricsHost>&
MediaStreamTrackMetrics::GetMediaStreamTrackMetricsHost() {
  if (!track_metrics_host_) {
    ChildThreadImpl::current()->BindHostReceiver(
        track_metrics_host_.BindNewPipeAndPassReceiver());
  }
  return track_metrics_host_;
}

}  // namespace content
