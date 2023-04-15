/* MyAppDelegate */

#import <Cocoa/Cocoa.h>

@interface MyAppDelegate : NSObject
{
    BOOL        initial_action_done;
    BOOL        should_terminate;
}
- (id)init;
- (IBAction)showPreferences:(id)sender;
- (BOOL)shouldShowUI;
- (BOOL)shouldTerminate;
- (void)testFileTypeBinding;
@end
