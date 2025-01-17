// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Nullable;

import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.BottomSheetContent;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.ContentPriority;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

/** A simple sheet content to test with. This only displays two empty white views. */
public class TestBottomSheetContent implements BottomSheetContent {
    /** Empty view that represents the toolbar. */
    private View mToolbarView;

    /** Empty view that represents the content. */
    private View mContentView;

    /** This content's priority. */
    private @ContentPriority int mPriority;

    /** Whether this content is browser specific. */
    private boolean mHasCustomLifecycle;

    /** Whether this content's peek state is enabled. */
    private boolean mPeekStateEnabled;

    /**
     * @param context A context to inflate views with.
     * @param priority The content's priority.
     * @param hasCustomLifecycle Whether the content is browser specific.
     * @param peekStateEnabled Whether the content's peek state is enabled.
     */
    public TestBottomSheetContent(Context context, @ContentPriority int priority,
            boolean hasCustomLifecycle, boolean peekStateEnabled) {
        mPriority = priority;
        mHasCustomLifecycle = hasCustomLifecycle;
        mPeekStateEnabled = peekStateEnabled;
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            mToolbarView = new View(context);
            ViewGroup.LayoutParams params =
                    new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 100);
            mToolbarView.setLayoutParams(params);
            mToolbarView.setBackground(new ColorDrawable(Color.WHITE));

            mContentView = new View(context);
            params = new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContentView.setLayoutParams(params);
            mToolbarView.setBackground(new ColorDrawable(Color.WHITE));
        });
    }

    /**
     * @param context A context to inflate views with.
     * @param priority The content's priority.
     * @param hasCustomLifecycle Whether the content is browser specific.
     */
    public TestBottomSheetContent(
            Context context, @ContentPriority int priority, boolean hasCustomLifecycle) {
        this(context, priority, hasCustomLifecycle, true);
    }

    /**
     * @param context A context to inflate views with.
     * @param peekStateEnabled Whether the content's peek state is enabled.
     */
    public TestBottomSheetContent(Context context, boolean peekStateEnabled) {
        this(/*TestBottomSheetContent(*/ context, ContentPriority.LOW, false, peekStateEnabled);
    }

    @Override
    public View getContentView() {
        return mContentView;
    }

    @Nullable
    @Override
    public View getToolbarView() {
        return mToolbarView;
    }

    @Override
    public int getVerticalScrollOffset() {
        return 0;
    }

    @Override
    public void destroy() {}

    @Override
    public int getPriority() {
        return mPriority;
    }

    @Override
    public boolean swipeToDismissEnabled() {
        return false;
    }

    @Override
    public boolean isPeekStateEnabled() {
        return mPeekStateEnabled;
    }

    @Override
    public boolean hasCustomLifecycle() {
        return mHasCustomLifecycle;
    }

    @Override
    public boolean setContentSizeListener(@Nullable BottomSheet.ContentSizeListener listener) {
        return false;
    }

    @Override
    public int getSheetContentDescriptionStringId() {
        return android.R.string.copy;
    }

    @Override
    public int getSheetHalfHeightAccessibilityStringId() {
        return android.R.string.copy;
    }

    @Override
    public int getSheetFullHeightAccessibilityStringId() {
        return android.R.string.copy;
    }

    @Override
    public int getSheetClosedAccessibilityStringId() {
        return android.R.string.copy;
    }
}
