#import "MyAppDelegate.h"
#import "PreferencesWindowController.h"
#import <Carbon/Carbon.h>

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
    // if this call comes before applicationDidFinishLaunching: we 
    // should terminate immedeately after starting the script.
    if (!initial_action_done)
        should_terminate = YES;
    initial_action_done = YES;
    if( GetCurrentKeyModifiers() & optionKey )
        return YES;
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

@end
