// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_DEMO_CLIENT_DEMO_CLIENT_H_
#define COMPONENTS_VIZ_DEMO_CLIENT_DEMO_CLIENT_H_

#include <map>
#include <vector>

#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "components/viz/common/frame_timing_details_map.h"
#include "components/viz/common/quads/compositor_frame_metadata.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/public/mojom/compositing/compositor_frame_sink.mojom.h"

namespace viz {
class CompositorFrame;
}

namespace demo {

// DemoClient is responsible for communicating with the display-compositor. It
// sends messages to the service over the mojom.CompositorFrameSink interface,
// and receives messages through the mojom.CompositorFrameSinkClient interface.
//
// The client needs to have an identifier, FrameSinkId, which doesn't change
// during the lifetime of the client. The embedder of the client needs to know
// about this FrameSinkId. The 'host', when it itself is not the embedder, also
// needs to know about this FrameSinkId, so that it can set up the
// frame-hierarchy correctly in the service.
//
// The client also needs to have a LocalSurfaceId, which represents an embedding
// of the client in a particular state. If, for example, the size of the client
// changes (or other attributes, like device scale factor), then a new
// LocalSurfaceId needs to be allocated. The LocalSurfaceId is used to submit
// the CompositorFrame. Both the embedder and the embedded clients need to know
// the LocalSurfaceId. It is possible for both the embedder and the embedded
// client to generate new LocalSurfaceId (typically using a
// ParentLocalSurfaceIdAllocator and ChildLocalSurfaceIdAllocator respectively).
// In this demo, only the embedder allocates the LocalSurfaceId.
class DemoClient : public viz::mojom::CompositorFrameSinkClient {
 public:
  DemoClient(const viz::FrameSinkId& frame_sink_id,
             const viz::LocalSurfaceIdAllocation& local_surface_id,
             const gfx::Rect& bounds);
  ~DemoClient() override;

  const viz::FrameSinkId& frame_sink_id() const { return frame_sink_id_; }

  // Initializes the mojo connection to the service.
  void Initialize(viz::mojom::CompositorFrameSinkClientRequest request,
                  viz::mojom::CompositorFrameSinkAssociatedPtrInfo sink_info);
  void Initialize(viz::mojom::CompositorFrameSinkClientRequest request,
                  viz::mojom::CompositorFrameSinkPtrInfo sink_info);

  // This prepares for this client to embed another client (i.e. this client
  // acts as the embedder). Since this client is the embedder, it allocates the
  // LocalSurfaceId, and returns that. The client that should be embedded (i.e.
  // the client represented by |frame_sink_id|) should use the returned
  // LocalSurfaceId to submit visual content (CompositorFrame).
  viz::LocalSurfaceIdAllocation Embed(const viz::FrameSinkId& frame_sink_id,
                                      const gfx::Rect& bounds);

  // When this client is resized, it is important that it also receives a new
  // LocalSurfaceId with the new size.
  void Resize(const gfx::Size& size,
              const viz::LocalSurfaceIdAllocation& local_surface_id);

 private:
  struct EmbedInfo {
    viz::LocalSurfaceIdAllocation lsid;
    gfx::Rect bounds;
    float degrees = 0.f;
  };

  viz::CompositorFrame CreateFrame(const viz::BeginFrameArgs& args)
      EXCLUSIVE_LOCKS_REQUIRED(lock_);
  viz::mojom::CompositorFrameSink* GetPtr();

  void InitializeOnThread(
      viz::mojom::CompositorFrameSinkClientRequest request,
      viz::mojom::CompositorFrameSinkAssociatedPtrInfo associated_sink_info,
      viz::mojom::CompositorFrameSinkPtrInfo sink_info);

  // viz::mojom::CompositorFrameSinkClient:
  void DidReceiveCompositorFrameAck(
      const std::vector<viz::ReturnedResource>& resources) override;
  void OnBeginFrame(const viz::BeginFrameArgs& args,
                    const viz::FrameTimingDetailsMap& timing_details) override;
  void OnBeginFramePausedChanged(bool paused) override;
  void ReclaimResources(
      const std::vector<viz::ReturnedResource>& resources) override;

  // This thread is created solely to demonstrate that the client can live in
  // its own thread (or even in its own process). A viz client does not need to
  // have its own thread.
  base::Thread thread_;

  const viz::FrameSinkId frame_sink_id_;
  viz::LocalSurfaceIdAllocation local_surface_id_ GUARDED_BY(lock_);
  gfx::Rect bounds_ GUARDED_BY(lock_);

  mojo::Binding<viz::mojom::CompositorFrameSinkClient> binding_;
  viz::mojom::CompositorFrameSinkAssociatedPtr associated_sink_;
  viz::mojom::CompositorFrameSinkPtr sink_;
  viz::FrameTokenGenerator next_frame_token_;
  uint32_t frame_count_ = 0;

  base::Lock lock_;
  // The |allocator_| is used only when this client acts as an embedder, and
  // embeds other clients.
  viz::ParentLocalSurfaceIdAllocator allocator_;
  std::map<viz::FrameSinkId, EmbedInfo> embeds_ GUARDED_BY(lock_);

  DISALLOW_COPY_AND_ASSIGN(DemoClient);
};

}  // namespace demo

#endif  // COMPONENTS_VIZ_DEMO_CLIENT_DEMO_CLIENT_H_
