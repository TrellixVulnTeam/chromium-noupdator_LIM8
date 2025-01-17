// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_EXTENSIONS_BOOKMARK_APP_REGISTRY_CONTROLLER_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_EXTENSIONS_BOOKMARK_APP_REGISTRY_CONTROLLER_H_

#include "chrome/browser/web_applications/components/app_registry_controller.h"

class Profile;

namespace extensions {

class Extension;

class BookmarkAppRegistryController : public web_app::AppRegistryController {
 public:
  explicit BookmarkAppRegistryController(Profile* profile);
  ~BookmarkAppRegistryController() override;

  // AppRegistryController:
  void Init(base::OnceClosure callback) override;
  void SetAppLaunchContainer(
      const web_app::AppId& app_id,
      web_app::LaunchContainer launch_container) override;
  web_app::WebAppSyncBridge* AsWebAppSyncBridge() override;

 private:
  const Extension* GetExtension(const web_app::AppId& app_id) const;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_EXTENSIONS_BOOKMARK_APP_REGISTRY_CONTROLLER_H_
