// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_P2P_NETWORK_LIST_OBSERVER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_P2P_NETWORK_LIST_OBSERVER_H_

#include <vector>

namespace net {
class IPAddress;
struct NetworkInterface;
typedef std::vector<NetworkInterface> NetworkInterfaceList;
}  // namespace net

namespace blink {

// TODO(crbug.com/787254): Move this class out of the Blink exposed API when
// all users of it have been Onion souped. Also, move it away from std::vector.
class NetworkListObserver {
 public:
  virtual ~NetworkListObserver() {}

  virtual void OnNetworkListChanged(
      const net::NetworkInterfaceList& list,
      const net::IPAddress& default_ipv4_local_address,
      const net::IPAddress& default_ipv6_local_address) = 0;

 protected:
  NetworkListObserver() {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_P2P_NETWORK_LIST_OBSERVER_H_
