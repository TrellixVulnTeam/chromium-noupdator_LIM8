// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.weblayer;

import android.content.Context;
import android.os.RemoteException;

import org.chromium.weblayer_private.aidl.APICallException;
import org.chromium.weblayer_private.aidl.IBrowserFragmentController;
import org.chromium.weblayer_private.aidl.IProfile;
import org.chromium.weblayer_private.aidl.ObjectWrapper;

/**
 * Profile holds state (typically on disk) needed for browsing. Create a
 * Profile via WebLayer.
 */
public final class Profile {
    private IProfile mImpl;

    Profile(IProfile impl) {
        mImpl = impl;
    }

    @Override
    protected void finalize() {
        // TODO(sky): figure out right assertion here if mImpl is non-null.
    }

    public void destroy() {
        try {
            mImpl.destroy();
        } catch (RemoteException e) {
            throw new APICallException(e);
        }
    }

    public void clearBrowsingData() {
        try {
            mImpl.clearBrowsingData();
        } catch (RemoteException e) {
            throw new APICallException(e);
        }
    }

    public BrowserFragmentController createBrowserFragmentController(Context context) {
        try {
            BrowserFragment fragment = new BrowserFragment();
            IBrowserFragmentController browserFragmentImpl =
                    mImpl.createBrowserFragmentController(fragment.asIRemoteFragmentClient(),
                            ObjectWrapper.wrap(WebLayer.createRemoteContext(context)));
            fragment.setRemoteFragment(browserFragmentImpl.getRemoteFragment());
            return new BrowserFragmentController(browserFragmentImpl, fragment);
        } catch (RemoteException e) {
            throw new APICallException(e);
        }
    }
}
