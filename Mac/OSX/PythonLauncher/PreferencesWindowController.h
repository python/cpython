/* PreferencesWindowController */

#import <Cocoa/Cocoa.h>

#import "FileSettings.h"

@interface PreferencesWindowController : NSWindowController <FileSettingsSource>
{
    IBOutlet NSPopUpButton *filetype;
    IBOutlet NSTextField *interpreter;
    IBOutlet NSButton *debug;
    IBOutlet NSButton *verbose;
    IBOutlet NSButton *inspect;
    IBOutlet NSButton *optimize;
    IBOutlet NSButton *nosite;
    IBOutlet NSButton *tabs;
    IBOutlet NSTextField *others;
    IBOutlet NSButton *with_terminal;
    IBOutlet NSTextField *commandline;

    FileSettings *settings;
}

+ getPreferencesWindow;

- (IBAction)do_reset:(id)sender;
- (IBAction)do_apply:(id)sender;
- (IBAction)do_filetype:(id)sender;

- (void)controlTextDidChange:(NSNotification *)aNotification;

@end
