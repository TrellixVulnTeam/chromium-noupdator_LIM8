// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.profiles;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.chrome.browser.cookies.CookiesFetcher;
import org.chromium.content_public.browser.BrowserStartupController;

/**
 * Wrapper that allows passing a Profile reference around in the Java layer.
 */
public class Profile {
    /** Whether this wrapper corresponds to an off the record Profile. */
    private final boolean mIsOffTheRecord;

    /** Pointer to the Native-side ProfileAndroid. */
    private long mNativeProfileAndroid;

    private Profile(long nativeProfileAndroid) {
        mNativeProfileAndroid = nativeProfileAndroid;
        mIsOffTheRecord = ProfileJni.get().isOffTheRecord(mNativeProfileAndroid, Profile.this);
    }

    public static Profile getLastUsedProfile() {
        // TODO(crbug.com/704025): turn this into an assert once the bug is fixed
        if (!BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                        .isFullBrowserStarted()) {
            throw new IllegalStateException("Browser hasn't finished initialization yet!");
        }
        return (Profile) ProfileJni.get().getLastUsedProfile();
    }

    /**
     * Destroys the Profile.  Destruction is delayed until all associated
     * renderers have been killed, so the profile might not be destroyed upon returning from
     * this call.
     */
    public void destroyWhenAppropriate() {
        ProfileJni.get().destroyWhenAppropriate(mNativeProfileAndroid, Profile.this);
    }

    public Profile getOriginalProfile() {
        return (Profile) ProfileJni.get().getOriginalProfile(mNativeProfileAndroid, Profile.this);
    }

    public Profile getOffTheRecordProfile() {
        return (Profile) ProfileJni.get().getOffTheRecordProfile(
                mNativeProfileAndroid, Profile.this);
    }

    public boolean hasOffTheRecordProfile() {
        return ProfileJni.get().hasOffTheRecordProfile(mNativeProfileAndroid, Profile.this);
    }

    public ProfileKey getProfileKey() {
        return (ProfileKey) ProfileJni.get().getProfileKey(mNativeProfileAndroid, Profile.this);
    }

    public boolean isOffTheRecord() {
        return mIsOffTheRecord;
    }

    /**
     * @return Whether the profile is signed in to a child account.
     */
    public boolean isChild() {
        return ProfileJni.get().isChild(mNativeProfileAndroid, Profile.this);
    }

    /**
     * Wipes all data for this profile.
     */
    public void wipe() {
        ProfileJni.get().wipe(mNativeProfileAndroid, Profile.this);
    }

    /**
     * @return Whether or not the native side profile exists.
     */
    @VisibleForTesting
    public boolean isNativeInitialized() {
        return mNativeProfileAndroid != 0;
    }

    @CalledByNative
    private static Profile create(long nativeProfileAndroid) {
        return new Profile(nativeProfileAndroid);
    }

    @CalledByNative
    private void onNativeDestroyed() {
        mNativeProfileAndroid = 0;

        if (mIsOffTheRecord) {
            CookiesFetcher.deleteCookiesIfNecessary();
        }
    }

    @CalledByNative
    private long getNativePointer() {
        return mNativeProfileAndroid;
    }

    @NativeMethods
    interface Natives {
        Object getLastUsedProfile();
        void destroyWhenAppropriate(long nativeProfileAndroid, Profile caller);
        Object getOriginalProfile(long nativeProfileAndroid, Profile caller);
        Object getOffTheRecordProfile(long nativeProfileAndroid, Profile caller);
        boolean hasOffTheRecordProfile(long nativeProfileAndroid, Profile caller);
        boolean isOffTheRecord(long nativeProfileAndroid, Profile caller);
        boolean isChild(long nativeProfileAndroid, Profile caller);
        void wipe(long nativeProfileAndroid, Profile caller);
        Object getProfileKey(long nativeProfileAndroid, Profile caller);
    }
}
