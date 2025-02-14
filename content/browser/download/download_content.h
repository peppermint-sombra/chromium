// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_CONTENT_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_CONTENT_H_

// The type of download based on mimetype.
// This is used by UMA and UKM metrics.
namespace content {

// NOTE: Keep in sync with DownloadContentType in
// tools/metrics/histograms/enums.xml.
enum class DownloadContent {
  UNRECOGNIZED = 0,
  TEXT = 1,
  IMAGE = 2,
  AUDIO = 3,
  VIDEO = 4,
  OCTET_STREAM = 5,
  PDF = 6,
  DOCUMENT = 7,
  SPREADSHEET = 8,
  PRESENTATION = 9,
  ARCHIVE = 10,
  EXECUTABLE = 11,
  DMG = 12,
  CRX = 13,
  WEB = 14,
  EBOOK = 15,
  FONT = 16,
  APK = 17,
  MAX = 18,
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_CONTENT_H_
