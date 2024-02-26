//
//  main.m
//  iOSTestbed
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    @autoreleasepool {
        appDelegateClassName = NSStringFromClass([AppDelegate class]);

        // iOS doesn't like uncaught signals.
        signal(SIGPIPE, SIG_IGN);
        signal(SIGXFSZ, SIG_IGN);

        return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    }
}
