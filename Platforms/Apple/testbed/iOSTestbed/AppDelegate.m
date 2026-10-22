//
//  AppDelegate.m
//  iOSTestbed
//

#import "AppDelegate.h"
#import "SceneDelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    UISceneConfiguration *configuration = [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
    configuration.delegateClass = [SceneDelegate class];
    return configuration;
}

- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {
}

@end
