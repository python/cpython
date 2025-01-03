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

        return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    }
}
