#import "MyAppDelegate.h"
#import "PreferencesWindowController.h"

@implementation MyAppDelegate

- (id)init
{
    self = [super init];
    initial_action_done = NO;
    should_terminate = NO;
    return self;
}

- (IBAction)showPreferences:(id)sender
{
    [PreferencesWindowController getPreferencesWindow];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    // If we were opened because of a file drag or doubleclick
    // we've set initial_action_done in shouldShowUI
    // Otherwise we open a preferences dialog.
    if (!initial_action_done) {
        initial_action_done = YES;
        [self showPreferences: self];
    }
}

- (BOOL)shouldShowUI
{
    // if this call comes before applicationDidFinishLaunching: we do not show a UI
    // for the file. Also, we should terminate immedeately after starting the script.
    if (initial_action_done)
        return YES;
    initial_action_done = YES;
    should_terminate = YES;
    return NO;
}

- (BOOL)shouldTerminate
{
    return should_terminate;
}

- (BOOL)applicationShouldOpenUntitledFile:(NSApplication *)sender
{
    return NO;
}


- (BOOL)application:(NSApplication *)sender xx_openFile:(NSString *)filename
{
    initial_action_done = YES;
    return YES;
}

- (BOOL)application:(id)sender xx_openFileWithoutUI:(NSString *)filename
{
    initial_action_done = YES;
    return YES;
}

@end
