// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/tracing.h"

#include <memory>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/strings/string_util.h"
#include "base/timer/timer.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/tracing_controller.h"
#include "content/public/test/test_utils.h"
#include "services/tracing/public/mojom/constants.mojom.h"

namespace {

using content::BrowserThread;

class StringTraceEndpoint
    : public content::TracingController::TraceDataEndpoint {
 public:
  StringTraceEndpoint(std::string* result, const base::Closure& callback)
      : result_(result), completion_callback_(callback) {}

  void ReceiveTraceChunk(std::unique_ptr<std::string> chunk) override {
    *result_ += result_->empty() ? "[" : "";
    DCHECK(chunk);
    *result_ += *chunk;
  }

  void ReceiveTraceFinalContents(
      std::unique_ptr<const base::DictionaryValue> metadata) override {
    if (!result_->empty())
      *result_ += "]";
    completion_callback_.Run();
  }

 private:
  ~StringTraceEndpoint() override {}

  std::string* result_;
  base::Closure completion_callback_;

  DISALLOW_COPY_AND_ASSIGN(StringTraceEndpoint);
};

class InProcessTraceController {
 public:
  static InProcessTraceController* GetInstance() {
    return base::Singleton<InProcessTraceController>::get();
  }

  InProcessTraceController() {}
  virtual ~InProcessTraceController() {}

  bool BeginTracing(
      const base::trace_event::TraceConfig& trace_config,
      tracing::StartTracingDoneCallback start_tracing_done_callback) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    return content::TracingController::GetInstance()->StartTracing(
        trace_config, start_tracing_done_callback);
  }

  bool EndTracing(std::string* json_trace_output) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    if (!content::TracingController::GetInstance()->StopTracing(
            new StringTraceEndpoint(
                json_trace_output,
                base::Bind(&InProcessTraceController::OnTracingComplete,
                           base::Unretained(this))),
            tracing::mojom::kChromeTraceEventLabel)) {
      return false;
    }
    // Wait for OnEndTracingComplete() to quit the message loop.
    message_loop_runner_ = new content::MessageLoopRunner;
    message_loop_runner_->Run();

    return true;
  }

 private:
  friend struct base::DefaultSingletonTraits<InProcessTraceController>;

  void OnEnableTracingComplete() {
    message_loop_runner_->Quit();
  }

  void OnTracingComplete() { message_loop_runner_->Quit(); }

  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  base::OneShotTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(InProcessTraceController);
};

}  // namespace

namespace tracing {

bool BeginTracing(const std::string& category_patterns) {
  return InProcessTraceController::GetInstance()->BeginTracing(
      base::trace_event::TraceConfig(category_patterns, ""),
      tracing::StartTracingDoneCallback());
}

bool BeginTracingWithTraceConfig(
    const base::trace_event::TraceConfig& trace_config) {
  return InProcessTraceController::GetInstance()->BeginTracing(
      trace_config, tracing::StartTracingDoneCallback());
}

bool BeginTracingWithTraceConfig(
    const base::trace_event::TraceConfig& trace_config,
    tracing::StartTracingDoneCallback start_tracing_done_callback) {
  return InProcessTraceController::GetInstance()->BeginTracing(
      trace_config, start_tracing_done_callback);
}

bool EndTracing(std::string* json_trace_output) {
  return InProcessTraceController::GetInstance()->EndTracing(json_trace_output);
}

}  // namespace tracing
