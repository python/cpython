#import "PreferencesWindowController.h"

@implementation PreferencesWindowController

+ getPreferencesWindow
{
    static PreferencesWindowController *_singleton;
    
    if (!_singleton)
        _singleton = [[PreferencesWindowController alloc] init];
    [_singleton showWindow: _singleton];
    return _singleton;
}

- (id) init
{
    self = [self initWithWindowNibName: @"PreferenceWindow"];
    return self;
}

- (void)load_defaults
{
    NSString *title = [filetype titleOfSelectedItem];
    
    settings = [FileSettings getDefaultsForFileType: title];
}

- (void)update_display
{
//    [[self window] setTitle: script];
    
    [interpreter setStringValue: [settings interpreter]];
    [debug setState: [settings debug]];
    [verbose setState: [settings verbose]];
    [inspect setState: [settings inspect]];
    [optimize setState: [settings optimize]];
    [nosite setState: [settings nosite]];
    [tabs setState: [settings tabs]];
    [others setStringValue: [settings others]];
    [with_terminal setState: [settings with_terminal]];
    
    [commandline setStringValue: [settings commandLineForScript: @"<your script here>"]];
}

- (void) windowDidLoad
{
    [super windowDidLoad];
    [self load_defaults];
    [self update_display];
}

- (void)update_settings
{
    [settings updateFromSource: self];
}

- (IBAction)do_filetype:(id)sender
{
    [self load_defaults];
    [self update_display];
}

- (IBAction)do_reset:(id)sender
{
    [self load_defaults];
    [self update_display];
}

- (IBAction)do_apply:(id)sender
{
    [self update_settings];
    [self update_display];
}

// FileSettingsSource protocol 
- (NSString *) interpreter { return [interpreter stringValue];};
- (BOOL) debug { return [debug state];};
- (BOOL) verbose { return [verbose state];};
- (BOOL) inspect { return [inspect state];};
- (BOOL) optimize { return [optimize state];};
- (BOOL) nosite { return [nosite state];};
- (BOOL) tabs { return [tabs state];};
- (NSString *) others { return [others stringValue];};
- (BOOL) with_terminal { return [with_terminal state];};

// Delegates
- (void)controlTextDidChange:(NSNotification *)aNotification
{
    [self update_settings];
    [self update_display];
};


@end
