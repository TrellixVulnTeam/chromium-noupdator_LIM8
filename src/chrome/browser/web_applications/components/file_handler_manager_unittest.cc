// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/components/file_handler_manager.h"

#include <set>
#include <string>
#include <vector>

#include "chrome/browser/web_applications/test/test_file_handler_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_app {

TEST(FileHandlerUtilsTest, GetFileExtensionsFromFileHandlers) {
  // Construct FileHandlerInfo vector with multiple file extensions.
  const std::vector<std::string> test_file_extensions = {"txt", "xls", "doc"};
  apps::FileHandlerInfo fhi1;
  apps::FileHandlerInfo fhi2;
  fhi1.extensions.insert(test_file_extensions[0]);
  fhi1.extensions.insert(test_file_extensions[1]);
  fhi2.extensions.insert(test_file_extensions[2]);
  std::vector<apps::FileHandlerInfo> file_handlers = {fhi1, fhi2};
  std::set<std::string> file_extensions =
      GetFileExtensionsFromFileHandlers(file_handlers);

  EXPECT_EQ(file_extensions.size(), test_file_extensions.size());
  for (const auto& test_file_extension : test_file_extensions) {
    EXPECT_TRUE(file_extensions.find(test_file_extension) !=
                file_extensions.end());
  }
}

}  // namespace web_app
