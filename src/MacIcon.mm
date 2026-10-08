// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MacIcon.hpp"

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

} // namespace

void applyMacIcon()
{
    NSString *path = appPath();
    if (path)
        [NSApp setApplicationIconImage:[[NSWorkspace sharedWorkspace] iconForFile:path]];
}
