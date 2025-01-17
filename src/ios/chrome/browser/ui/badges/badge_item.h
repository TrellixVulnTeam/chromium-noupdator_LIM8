// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BADGES_BADGE_ITEM_H_
#define IOS_CHROME_BROWSER_UI_BADGES_BADGE_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/badges/badge_type.h"

// States for the InfobarBadge.
typedef NS_ENUM(NSInteger, BadgeState) {
  // The badge is not accepted.
  BadgeStateNone = 0,
  // The Infobar Badge is accepted. e.g. The Infobar was accepted/confirmed, and
  // the Infobar action has taken place.
  BadgeStateAccepted,
};

// Holds properties and values the UI needs to configure a badge button.
@protocol BadgeItem

// The type of the badge.
- (BadgeType)badgeType;
// Whether the badge should be displayed in the fullScreenBadge position. If
// YES, it will be displayed in both FullScreen and non FullScreen.
- (BOOL)isFullScreen;
// Some badges may not be tappable if there is no action associated with it.
@property(nonatomic, assign, readonly, getter=isTappable) BOOL tappable;
// The BadgeState of the badge.
@property(nonatomic, assign) BadgeState badgeState;

@end

#endif  // IOS_CHROME_BROWSER_UI_BADGES_BADGE_ITEM_H_
