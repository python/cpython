//
//  main.m
//  watchOSTestbed
//

#import <WatchKit/WatchKit.h>
#import "AppDelegate.h"

int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    @autoreleasepool {
        appDelegateClassName = NSStringFromClass([AppDelegate class]);

        return WKApplicationMain(argc, argv, appDelegateClassName);
    }
}
