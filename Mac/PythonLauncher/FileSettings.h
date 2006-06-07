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
- (BOOL) honourhashbang;
- (BOOL) debug;
- (BOOL) verbose;
- (BOOL) inspect;
- (BOOL) optimize;
- (BOOL) nosite;
- (BOOL) tabs;
- (NSString *) others;
- (BOOL) with_terminal;
- (NSString *) scriptargs;
@end

@interface FileSettings : NSObject <FileSettingsSource>
{
    NSString *interpreter;	// The pathname of the interpreter to use
    NSArray *interpreters;	// List of known interpreters
    BOOL honourhashbang;	// #! line overrides interpreter
    BOOL debug;			// -d option: debug parser
    BOOL verbose;		// -v option: verbose import
    BOOL inspect;		// -i option: interactive mode after script
    BOOL optimize;		// -O option: optimize bytecode
    BOOL nosite;		// -S option: don't import site.py
    BOOL tabs;			// -t option: warn about inconsistent tabs
    NSString *others;		// other options
    NSString *scriptargs;	// script arguments (not for preferences)
    BOOL with_terminal;		// Run in terminal window

    FileSettings *origsource;
    NSString *prefskey;
}

+ (id)getDefaultsForFileType: (NSString *)filetype;
+ (id)getFactorySettingsForFileType: (NSString *)filetype;
+ (id)newSettingsForFileType: (NSString *)filetype;

//- (id)init;
- (id)initForFileType: (NSString *)filetype;
- (id)initForFSDefaultFileType: (NSString *)filetype;
- (id)initForDefaultFileType: (NSString *)filetype;
//- (id)initWithFileSettings: (FileSettings *)source;

- (void)updateFromSource: (id <FileSettingsSource>)source;
- (NSString *)commandLineForScript: (NSString *)script;

//- (void)applyFactorySettingsForFileType: (NSString *)filetype;
//- (void)saveDefaults;
//- (void)applyUserDefaults: (NSString *)filetype;
- (void)applyValuesFromDict: (NSDictionary *)dict;
- (void)reset;
- (NSArray *) interpreters;

@end
