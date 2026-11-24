//
//  AppDelegate.m
//  watchOSTestbed
//

#import "AppDelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

// The testbed has no storyboard (Interface.plist), so WatchKit requires the
// delegate to provide a root interface controller class, or the app crashes
// on launch.
- (Class)applicationRootInterfaceControllerClass {
    return [WKInterfaceController class];
}

@end
