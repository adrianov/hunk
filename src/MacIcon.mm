// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MacIcon.hpp"

#include <QImage>

#import <AppKit/AppKit.h>

void applyMacIcon()
{
    const QImage image(QStringLiteral(":/icons/hunk.png"));
    CGImageRef cg = image.toCGImage();
    if (!cg)
        return;
    NSImage *icon = [[NSImage alloc] initWithCGImage:cg size:NSZeroSize];
    CGImageRelease(cg);
    [NSApp setApplicationIconImage:icon];
#if !__has_feature(objc_arc)
    [icon release];
#endif
}
