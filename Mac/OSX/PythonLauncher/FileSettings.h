//
//  FileSettings.h
//  PythonLauncher
//
//  Created by Jack Jansen on Sun Jul 21 2002.
//  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol FileSettingsSource
- (NSString *) interpreter;
- (BOOL) debug;
- (BOOL) verbose;
- (BOOL) inspect;
- (BOOL) optimize;
- (BOOL) nosite;
- (BOOL) tabs;
- (NSString *) others;
- (BOOL) with_terminal;
@end

@interface FileSettings : NSObject <FileSettingsSource>
{
    NSString *interpreter;	// The pathname of the interpreter to use
    BOOL debug;			// -d option: debug parser
    BOOL verbose;		// -v option: verbose import
    BOOL inspect;		// -i option: interactive mode after script
    BOOL optimize;		// -O option: optimize bytecode
    BOOL nosite;		// -S option: don't import site.py
    BOOL tabs;			// -t option: warn about inconsistent tabs
    NSString *others;		// other options
    BOOL with_terminal;		// Run in terminal window

    FileSettings *origsource;
    NSString *prefskey;
}

+ (id)getDefaultsForFileType: (NSString *)filetype;
+ (id)newSettingsForFileType: (NSString *)filetype;

- (id)init;
- (id)initWithFileSettings: (FileSettings *)source;

- (void)updateFromSource: (id <FileSettingsSource>)source;
- (NSString *)commandLineForScript: (NSString *)script;

- (id)factorySettingsForFileType: (NSString *)filetype;
- (void)saveDefaults;
- (void)updateFromUserDefaults: (NSString *)filetype;


@end
