// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Utilities that are used in multiple tests.

function MockWindow(width, height, sizer) {
  this.innerWidth = width;
  this.innerHeight = height;
  this.addEventListener = function(e, f) {
    if (e == 'scroll') {
      this.scrollCallback = f;
    }
    if (e == 'resize') {
      this.resizeCallback = f;
    }
  };
  this.setSize = function(width, height) {
    this.innerWidth = width;
    this.innerHeight = height;
    this.resizeCallback();
  };
  this.scrollTo = function(x, y) {
    if (sizer) {
      x = Math.min(x, parseInt(sizer.style.width) - width);
      y = Math.min(y, parseInt(sizer.style.height) - height);
    }
    this.pageXOffset = Math.max(0, x);
    this.pageYOffset = Math.max(0, y);
    this.scrollCallback();
  };
  this.setTimeout = function(callback, time) {
    this.timerCallback = callback;
    return 'timerId';
  };
  this.clearTimeout = function(timerId) {
    this.timerCallback = null;
  };
  this.runTimeout = function() {
    if (this.timerCallback) {
      this.timerCallback();
    }
  };
  if (sizer) {
    sizer.resizeCallback_ = function() {
      this.scrollTo(this.pageXOffset, this.pageYOffset);
    }.bind(this);
  }
  this.pageXOffset = 0;
  this.pageYOffset = 0;
  this.scrollCallback = null;
  this.resizeCallback = null;
  this.timerCallback = null;
}

function MockSizer() {
  var sizer = this;
  this.style = {
    width_: '0px',
    height_: '0px',
    get height() {
      return this.height_;
    },
    set height(height) {
      this.height_ = height;
      if (sizer.resizeCallback_) {
        sizer.resizeCallback_();
      }
    },
    get width() {
      return this.width_;
    },
    set width(width) {
      this.width_ = width;
      if (sizer.resizeCallback_) {
        sizer.resizeCallback_();
      }
    },
  };
}

function MockViewportChangedCallback() {
  this.wasCalled = false;
  this.callback = function() {
    this.wasCalled = true;
  }.bind(this);
  this.reset = function() {
    this.wasCalled = false;
  };
}

function MockDocumentDimensions(width, height, layoutOptions) {
  this.width = width || 0;
  this.height = height ? height : 0;
  this.layoutOptions = layoutOptions;
  this.pageDimensions = [];
  this.addPage = function(w, h) {
    var y = 0;
    if (this.pageDimensions.length != 0) {
      y = this.pageDimensions[this.pageDimensions.length - 1].y +
          this.pageDimensions[this.pageDimensions.length - 1].height;
    }
    this.width = Math.max(this.width, w);
    this.height += h;
    this.pageDimensions.push({x: 0, y: y, width: w, height: h});
  };
  this.addPageForTwoUpView = function(x, y, w, h) {
    this.width = Math.max(this.width, 2 * w);
    this.height = Math.max(this.height, y + h);
    this.pageDimensions.push({x: x, y: y, width: w, height: h});
  };
  this.reset = function() {
    this.width = 0;
    this.height = 0;
    this.pageDimensions = [];
  };
}

function animationFrame() {
  return new Promise(resolve => requestAnimationFrame(resolve));
}

function contentElement() {
  return document.elementFromPoint(innerWidth / 2, innerHeight / 2);
}

async function testAsync(f) {
  try {
    await f();
    chrome.test.succeed();
  } catch (e) {
    chrome.test.fail(e);
  }
}

/**
 * @return {!HTMLElement} An element containing a dom-repeat of bookmarks, for
 *     testing the bookmarks outside of the toolbar.
 */
function createBookmarksForTest() {
  const module = document.createElement('dom-module');
  module.id = 'test-bookmarks';
  module.innerHTML = `
      <template>
        <template is="dom-repeat" items="[[bookmarks]]">
          <viewer-bookmark bookmark="[[item]]" depth="0"></viewer-bookmark>
        </template>
      </template>
  `;
  document.body.appendChild(module);
  Polymer({
    is: 'test-bookmarks',
    properties: {
      bookmarks: Array,
    },
  });
  return document.createElement('test-bookmarks');
}
