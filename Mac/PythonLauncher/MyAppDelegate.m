#import "MyAppDelegate.h"
#import "PreferencesWindowController.h"
#import <Carbon/Carbon.h>
#import <ApplicationServices/ApplicationServices.h>

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
    // Test that the file mappings are correct
    [self testFileTypeBinding];
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
    // should terminate immediately after starting the script.
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

- (void)testFileTypeBinding
{
    NSURL *ourUrl;
    OSStatus err;
    FSRef appRef;
    NSURL *appUrl;
    static NSString *extensions[] = { @"py", @"pyw", @"pyc", NULL};
    NSString **ext_p;
    int i;

    if ([[NSUserDefaults standardUserDefaults] boolForKey: @"SkipFileBindingTest"])
        return;
    ourUrl = [NSURL fileURLWithPath: [[NSBundle mainBundle] bundlePath]];
    for( ext_p = extensions; *ext_p; ext_p++ ) {
        err = LSGetApplicationForInfo(
            kLSUnknownType,
            kLSUnknownCreator,
            (CFStringRef)*ext_p,
            kLSRolesViewer,
            &appRef,
            (CFURLRef *)&appUrl);
        if (err || ![appUrl isEqual: ourUrl] ) {
            i = NSRunAlertPanel(@"File type binding",
                @"PythonLauncher is not the default application for all " \
                  @"Python script types. You should fix this with the " \
                  @"Finder's \"Get Info\" command.\n\n" \
                  @"See \"Changing the application that opens a file\" in " \
                  @"Mac Help for details.",
                @"OK",
                @"Don't show this warning again",
                NULL);
            if ( i == 0 ) { // Don't show again
                [[NSUserDefaults standardUserDefaults]
                    setObject:@"YES" forKey:@"SkipFileBindingTest"];
            }
            return;
        }
    }
}

@end
