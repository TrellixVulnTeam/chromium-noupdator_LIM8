// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_CERT_VERIFY_PROC_BUILTIN_H_
#define NET_CERT_CERT_VERIFY_PROC_BUILTIN_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "net/base/net_export.h"

namespace net {

class CertNetFetcher;
class CertVerifyProc;
class SystemTrustStore;

// Will be used to create the system trust store. Implementations must be
// thread-safe - CreateSystemTrustStore may be invoked concurrently on worker
// threads.
class NET_EXPORT SystemTrustStoreProvider {
 public:
  virtual ~SystemTrustStoreProvider() {}

  // Returns a SystemTrustStoreProvider that will create the default
  // SystemTrustStore for SSL (using CreateSslSystemTrustStore()).
  static std::unique_ptr<SystemTrustStoreProvider> CreateDefaultForSSL();

  // Create and return a SystemTrustStore to be used during certificate
  // verification.
  // This function may be invoked concurrently on worker threads and must be
  // thread-safe. However, the returned SystemTrustStore will only be used on
  // a single thread.
  virtual std::unique_ptr<SystemTrustStore> CreateSystemTrustStore() = 0;
};

// TODO(crbug.com/649017): This is not how other cert_verify_proc_*.h are
// implemented -- they expose the type in the header. Use a consistent style
// here too.
NET_EXPORT scoped_refptr<CertVerifyProc> CreateCertVerifyProcBuiltin(
    scoped_refptr<CertNetFetcher> net_fetcher,
    std::unique_ptr<SystemTrustStoreProvider> system_trust_store_provider);

// Returns the time limit used by CertVerifyProcBuiltin. Intended for test use.
NET_EXPORT_PRIVATE base::TimeDelta
GetCertVerifyProcBuiltinTimeLimitForTesting();

}  // namespace net

#endif  // NET_CERT_CERT_VERIFY_PROC_BUILTIN_H_
