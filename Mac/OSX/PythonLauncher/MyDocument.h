//
//  MyDocument.h
//  PythonLauncher
//
//  Created by Jack Jansen on Fri Jul 19 2002.
//  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
//


#import <Cocoa/Cocoa.h>

#import "FileSettings.h"

@interface MyDocument : NSDocument <FileSettingsSource>
{
    IBOutlet NSTextField *interpreter;
    IBOutlet NSButton *honourhashbang;
    IBOutlet NSButton *debug;
    IBOutlet NSButton *verbose;
    IBOutlet NSButton *inspect;
    IBOutlet NSButton *optimize;
    IBOutlet NSButton *nosite;
    IBOutlet NSButton *tabs;
    IBOutlet NSTextField *others;
    IBOutlet NSButton *with_terminal;
    IBOutlet NSTextField *scriptargs;
    IBOutlet NSTextField *commandline;

    NSString *script;
    NSString *filetype;
    FileSettings *settings;
}

- (IBAction)do_run:(id)sender;
- (IBAction)do_cancel:(id)sender;
- (IBAction)do_reset:(id)sender;
- (IBAction)do_apply:(id)sender;

- (void)controlTextDidChange:(NSNotification *)aNotification;

@end
