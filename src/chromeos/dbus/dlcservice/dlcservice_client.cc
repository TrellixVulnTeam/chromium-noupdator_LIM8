// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/dlcservice/dlcservice_client.h"

#include <stdint.h>

#include <algorithm>
#include <deque>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/dbus/constants/dbus_switches.h"
#include "chromeos/dbus/dlcservice/fake_dlcservice_client.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

const char kOnInstallStatus[] = "OnInstallStatus";

DlcserviceClient* g_instance = nullptr;

class DlcserviceErrorResponseHandler {
 public:
  explicit DlcserviceErrorResponseHandler(dbus::ErrorResponse* err_response)
      : err(dlcservice::kErrorInternal) {
    if (err_response && dbus::MessageReader(err_response).PopString(&err_msg) &&
        DlcserviceErrorFromString(err_msg, &err)) {
      VLOG(1) << "Handling error response err=" << err
              << " err_msg=" << err_msg;
    } else {
      LOG(ERROR) << "Failed to set err based on error response "
                 << "defaulted to kErrorInternal.";
    }
  }

  ~DlcserviceErrorResponseHandler() = default;

  std::string get_err() { return err; }

  std::string get_err_msg() { return err_msg; }

 private:
  bool DlcserviceErrorFromString(const std::string& err_msg, std::string* err) {
    static const base::NoDestructor<std::unordered_set<std::string>> err_set({
        dlcservice::kErrorNone,
        dlcservice::kErrorInternal,
        dlcservice::kErrorBusy,
        dlcservice::kErrorNeedReboot,
        dlcservice::kErrorInvalidDlc,
    });
    static const std::pair<std::string, std::string> delims = {"dlcservice/",
                                                               ":"};

    if (!err) {
      LOG(ERROR) << "err passed is nullptr.";
      return false;
    }

    // Clear to empty out |err|.
    err->clear();

    // Verify dlcservice error code.
    if (err_msg.find(delims.first) == std::string::npos) {
      LOG(ERROR) << "Dlcservice did not send valid error message: " << err_msg;
      *err = dlcservice::kErrorInternal;
      return true;
    }

    // Extract the dlcservice error code.
    size_t padding = delims.first.size();
    size_t second_idx = err_msg.find(delims.second, padding);
    *err = err_msg.substr(padding, second_idx - padding);

    // Lookup the dlcservice error code and provide default on failure.
    auto itr = err_set->find(*err);
    *err = itr != err_set->end() ? *itr : dlcservice::kErrorInternal;
    return true;
  }

  // Holds the dlcservice specific error.
  std::string err;

  // Holds the entire error message from error response.
  std::string err_msg;

  DISALLOW_COPY_AND_ASSIGN(DlcserviceErrorResponseHandler);
};

}  // namespace

// The DlcserviceClient implementation used in production.
class DlcserviceClientImpl : public DlcserviceClient {
 public:
  DlcserviceClientImpl() : dlcservice_proxy_(nullptr) {}

  ~DlcserviceClientImpl() override = default;

  void Install(const dlcservice::DlcModuleList& dlc_module_list,
               InstallCallback callback) override {
    if (!service_available_) {
      pending_tasks_.emplace_back(base::BindOnce(
          &DlcserviceClientImpl::Install, weak_ptr_factory_.GetWeakPtr(),
          std::move(dlc_module_list), std::move(callback)));
      return;
    }

    if (install_callback_holder_.has_value()) {
      pending_installs_.emplace_back(base::BindOnce(
          &DlcserviceClientImpl::Install, weak_ptr_factory_.GetWeakPtr(),
          std::move(dlc_module_list), std::move(callback)));
      return;
    }

    dbus::MethodCall method_call(dlcservice::kDlcServiceInterface,
                                 dlcservice::kInstallMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendProtoAsArrayOfBytes(dlc_module_list);

    install_callback_holder_ = std::move(callback);

    VLOG(1) << "Requesting to install DLC(s).";
    dlcservice_proxy_->CallMethodWithErrorResponse(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&DlcserviceClientImpl::OnInstall,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void Uninstall(const std::string& dlc_id,
                 UninstallCallback callback) override {
    if (!service_available_) {
      pending_tasks_.emplace_back(base::BindOnce(
          &DlcserviceClientImpl::Uninstall, weak_ptr_factory_.GetWeakPtr(),
          dlc_id, std::move(callback)));
      return;
    }

    dbus::MethodCall method_call(dlcservice::kDlcServiceInterface,
                                 dlcservice::kUninstallMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(dlc_id);

    VLOG(1) << "Requesting to uninstall DLC=" << dlc_id;
    dlcservice_proxy_->CallMethodWithErrorResponse(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&DlcserviceClientImpl::OnUninstall,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void GetInstalled(GetInstalledCallback callback) override {
    if (!service_available_) {
      pending_tasks_.emplace_back(
          base::BindOnce(&DlcserviceClientImpl::GetInstalled,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
      return;
    }

    dbus::MethodCall method_call(dlcservice::kDlcServiceInterface,
                                 dlcservice::kGetInstalledMethod);

    VLOG(1) << "Requesting to get installed DLC(s).";
    dlcservice_proxy_->CallMethodWithErrorResponse(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&DlcserviceClientImpl::OnGetInstalled,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void Init(dbus::Bus* bus) {
    dlcservice_proxy_ = bus->GetObjectProxy(
        dlcservice::kDlcServiceServiceName,
        dbus::ObjectPath(dlcservice::kDlcServiceServicePath));
    // TODO(kimjae): Use from cros_system_api the const once sync'ed for
    // kOnInstallStatus as dlcservice::kOnInstallStatus and delete the
    // definition of kOnInstallStatus.
    dlcservice_proxy_->ConnectToSignal(
        dlcservice::kDlcServiceInterface, kOnInstallStatus,
        base::Bind(&DlcserviceClientImpl::OnInstallStatus,
                   weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&DlcserviceClientImpl::OnInstallStatusConnected,
                       weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnInstallStatus(dbus::Signal* signal) {
    if (!install_callback_holder_.has_value())
      return;

    dlcservice::InstallStatus install_status;
    if (!dbus::MessageReader(signal).PopArrayOfBytesAsProto(&install_status)) {
      LOG(ERROR) << "Failed to parse proto as install status.";
      return;
    }

    auto SendSignal = [this, &install_status]() {
      base::Optional<InstallCallback> install_callback;
      std::swap(install_callback, install_callback_holder_);
      std::move(install_callback.value())
          .Run(install_status.error_code(), install_status.dlc_module_list());
    };

    switch (install_status.status()) {
      case dlcservice::Status::COMPLETED:
        VLOG(1) << "DLC(s) install successful.";
        SendSignal();
        break;
      case dlcservice::Status::RUNNING:
        VLOG(2) << "Install in progress: " << install_status.progress();
        // Need to return here since we don't want to try starting another
        // pending install from the queue (would waste time checking).
        return;
      case dlcservice::Status::FAILED:
        LOG(ERROR) << "Failed to install with error code: "
                   << install_status.error_code();
        SendSignal();
        break;
      default:
        NOTREACHED();
    }

    // Try to run a pending install since we have complete/failed the current
    // install, but do not waste trying to run a pending install when the
    // current install is running at the moment.
    if (!pending_installs_.empty()) {
      std::move(pending_installs_.front()).Run();
      pending_installs_.pop_front();
    }
  }

  void OnInstallStatusConnected(const std::string& interface,
                                const std::string& signal,
                                bool success) {
    // When the connected to dlcservice daemon's |OnInstallStatus| signal we can
    // go ahead and mark the service as being available and not queue up tasks
    // that came in before dlcservice daemon was available.
    if (success) {
      service_available_ = true;
      std::vector<base::OnceClosure> callbacks;
      callbacks.swap(pending_tasks_);
      for (auto&& callback : callbacks) {
        std::move(callback).Run();
      }
    } else {
      LOG(ERROR) << "Failed to connect to install status signal.";
      pending_tasks_.clear();
    }
  }

  void OnInstall(dbus::Response* response, dbus::ErrorResponse* err_response) {
    if (response)
      return;

    base::Optional<InstallCallback> install_callback;
    std::swap(install_callback, install_callback_holder_);
    std::move(install_callback.value())
        .Run(DlcserviceErrorResponseHandler(err_response).get_err(),
             dlcservice::DlcModuleList());
  }

  void OnUninstall(UninstallCallback callback,
                   dbus::Response* response,
                   dbus::ErrorResponse* err_response) {
    if (response) {
      std::move(callback).Run(dlcservice::kErrorNone);
      return;
    }

    std::move(callback).Run(
        DlcserviceErrorResponseHandler(err_response).get_err());
  }

  void OnGetInstalled(GetInstalledCallback callback,
                      dbus::Response* response,
                      dbus::ErrorResponse* err_response) {
    dlcservice::DlcModuleList dlc_module_list;
    if (response && dbus::MessageReader(response).PopArrayOfBytesAsProto(
                        &dlc_module_list)) {
      std::move(callback).Run(dlcservice::kErrorNone, dlc_module_list);
      return;
    }

    std::move(callback).Run(
        DlcserviceErrorResponseHandler(err_response).get_err(),
        dlcservice::DlcModuleList());
  }

  dbus::ObjectProxy* dlcservice_proxy_;

  // True after dlcservice's D-Bus service has become available.
  bool service_available_ = false;

  // The cached callback to call on a finished |Install()|.
  base::Optional<InstallCallback> install_callback_holder_;

  // A list of postponed calls to dlcservice to install only used after
  // the dlcservice is already available.
  std::deque<base::OnceClosure> pending_installs_;

  // A list of postponed calls to dlcservice to be called after it becomes
  // available.
  std::vector<base::OnceClosure> pending_tasks_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<DlcserviceClientImpl> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(DlcserviceClientImpl);
};

DlcserviceClient::DlcserviceClient() {
  CHECK(!g_instance);
  g_instance = this;
}

DlcserviceClient::~DlcserviceClient() {
  CHECK_EQ(this, g_instance);
  g_instance = nullptr;
}

// static
void DlcserviceClient::Initialize(dbus::Bus* bus) {
  CHECK(bus);
  (new DlcserviceClientImpl())->Init(bus);
}

// static
void DlcserviceClient::InitializeFake() {
  new FakeDlcserviceClient();
}

// static
void DlcserviceClient::Shutdown() {
  CHECK(g_instance);
  delete g_instance;
}

// static
DlcserviceClient* DlcserviceClient::Get() {
  return g_instance;
}

}  // namespace chromeos
