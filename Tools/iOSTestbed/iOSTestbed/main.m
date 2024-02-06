//
//  main.m
//  iOSTestbed
//
//  Created by Russell Keith-Magee on 23/11/2023.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
    }

    // iOS doesn't like uncaught signals.
    signal(SIGPIPE, SIG_IGN);
    signal(SIGXFSZ, SIG_IGN);

    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
