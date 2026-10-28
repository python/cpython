//
//  SceneDelegate.m
//  iOSTestbed
//

#import "SceneDelegate.h"

@interface SceneDelegate ()

@end

@implementation SceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    self.window.rootViewController = [[UIViewController alloc] init];
    [self.window makeKeyAndVisible];
}

@end
