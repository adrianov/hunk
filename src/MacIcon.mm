// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MacIcon.hpp"

#include <QPixmap>

#include <mach-o/dyld.h>

#import <AppKit/AppKit.h>

namespace {

NSString *appPath()
{
    NSString *path = [[NSBundle mainBundle] bundlePath];
    if ([path.pathExtension isEqualToString:@"app"])
        return path;
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0)
        return nil;
    path = [[NSString stringWithUTF8String:buf] stringByResolvingSymlinksInPath];
    path = [[[path stringByDeletingLastPathComponent] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
    return [path.pathExtension isEqualToString:@"app"] ? path : nil;
}

NSImage *sharpIcon(NSImage *source)
{
    const NSInteger side = 1024;
    source.size = NSMakeSize(side, side);
    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                                     pixelsWide:side
                                                                     pixelsHigh:side
                                                                   bitsPerSample:8
                                                                 samplesPerPixel:4
                                                                        hasAlpha:YES
                                                                        isPlanar:NO
                                                                  colorSpaceName:NSDeviceRGBColorSpace
                                                                     bytesPerRow:0
                                                                    bitsPerPixel:0];
    rep.size = NSMakeSize(side, side);
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:rep]];
    [source drawInRect:NSMakeRect(0, 0, side, side)];
    [NSGraphicsContext restoreGraphicsState];
    NSImage *image = [[[NSImage alloc] initWithSize:rep.size] autorelease];
    [image addRepresentation:rep];
    [rep release];
    return image;
}

} // namespace

void applyMacIcon()
{
    NSString *path = appPath();
    if (path)
        [NSApp setApplicationIconImage:sharpIcon([[NSWorkspace sharedWorkspace] iconForFile:path])];
}

QPixmap macAppIcon()
{
    NSData *data = [[NSApp applicationIconImage] TIFFRepresentation];
    QPixmap pixmap;
    if (data)
        pixmap.loadFromData(reinterpret_cast<const uchar *>(data.bytes), static_cast<uint>(data.length));
    return pixmap;
}
