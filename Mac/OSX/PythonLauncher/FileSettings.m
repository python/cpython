//
//  FileSettings.m
//  PythonLauncher
//
//  Created by Jack Jansen on Sun Jul 21 2002.
//  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
//

#import "FileSettings.h"

@implementation FileSettings
+ (id)getDefaultsForFileType: (NSString *)filetype
{
    static FileSettings *default_py, *default_pyw, *default_pyc;
    FileSettings **curdefault;
    
    if ([filetype isEqualToString: @"Python Script"]) {
        curdefault = &default_py;
    } else if ([filetype isEqualToString: @"Python GUI Script"]) {
        curdefault = &default_pyw;
    } else if ([filetype isEqualToString: @"Python Bytecode Document"]) {
        curdefault = &default_pyc;
    } else {
        NSLog(@"Funny File Type: %@\n", filetype);
        curdefault = &default_py;
        filetype = @"Python Script";
    }
    if (!*curdefault) {
        *curdefault = [[FileSettings new] init];
        [*curdefault factorySettingsForFileType: filetype];
        [*curdefault updateFromUserDefaults: filetype];
    }
    return *curdefault;
}

+ (id)newSettingsForFileType: (NSString *)filetype
{
    FileSettings *cur;
    
    cur = [[FileSettings new] init];
    [cur initWithFileSettings: [FileSettings getDefaultsForFileType: filetype]];
    return cur;
}

- (id)init
{
    self = [super init];
    return [self factorySettingsForFileType: @"Python Script"];
}

- (id)factorySettingsForFileType: (NSString *)filetype
{
    debug = NO;
    verbose = NO;
    inspect = NO;
    optimize = NO;
    nosite = NO;
    tabs = NO;
    others = @"";
    if ([filetype isEqualToString: @"Python Script"] ||
        [filetype isEqualToString: @"Python Bytecode Document"]) {
        interpreter = @"/usr/local/bin/python";
        with_terminal = YES;
   }  else if ([filetype isEqualToString: @"Python GUI Script"]) {
        interpreter = @"/Applications/Python.app/Contents/MacOS/python";
        with_terminal = NO;
    } else {
        NSLog(@"Funny File Type: %@\n", filetype);
    }
    origsource = NULL;
    return self;
}

- (id)initWithFileSettings: (FileSettings *)source
{
    interpreter = [source->interpreter retain];
    debug = source->debug;
    verbose = source->verbose;
    inspect = source->inspect;
    optimize = source->optimize;
    nosite = source->nosite;
    tabs = source->tabs;
    others = [source->others retain];
    with_terminal = source->with_terminal;
    
    origsource = [source retain];
    return self;
}

- (void)saveDefaults
{
    [origsource updateFromSource: self];
}

- (void)updateFromSource: (id <FileSettingsSource>)source
{
    interpreter = [[source interpreter] retain];
    debug = [source debug];
    verbose = [source verbose];
    inspect = [source inspect];
    optimize = [source optimize];
    nosite = [source nosite];
    tabs = [source tabs];
    others = [[source others] retain];
    with_terminal = [source with_terminal];
    if (!origsource) {
        NSUserDefaults *defaults;
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
            interpreter, @"interpreter",
            [NSNumber numberWithBool: debug], @"debug",
            [NSNumber numberWithBool: verbose], @"verbose",
            [NSNumber numberWithBool: inspect], @"inspect",
            [NSNumber numberWithBool: optimize], @"optimize",
            [NSNumber numberWithBool: nosite], @"nosite",
            [NSNumber numberWithBool: nosite], @"nosite",
            others, @"others",
            [NSNumber numberWithBool: with_terminal], @"with_terminal",
            nil];
        defaults = [NSUserDefaults standardUserDefaults];
        [defaults setObject: dict forKey: prefskey];
    }
}

- (void)updateFromUserDefaults: (NSString *)filetype
{
    NSUserDefaults *defaults;
    NSDictionary *dict;
    id value;
    
    prefskey = [filetype retain];
    defaults = [NSUserDefaults standardUserDefaults];
    dict = [defaults dictionaryForKey: filetype];
    if (!dict)
        return;
    value = [dict objectForKey: @"interpreter"];
    if (value) interpreter = [value retain];
    value = [dict objectForKey: @"debug"];
    if (value) debug = [value boolValue];
    value = [dict objectForKey: @"verbose"];
    if (value) verbose = [value boolValue];
    value = [dict objectForKey: @"inspect"];
    if (value) inspect = [value boolValue];
    value = [dict objectForKey: @"optimize"];
    if (value) optimize = [value boolValue];
    value = [dict objectForKey: @"nosite"];
    if (value) nosite = [value boolValue];
    value = [dict objectForKey: @"nosite"];
    if (value) tabs = [value boolValue];
    value = [dict objectForKey: @"others"];
    if (value) others = [value retain];
    value = [dict objectForKey: @"with_terminal"];
    if (value) with_terminal = [value boolValue];
}

- (NSString *)commandLineForScript: (NSString *)script
{
    return [NSString stringWithFormat:
        @"\"%@\"%s%s%s%s%s%s %@ \"%@\" %s",
        interpreter,
        debug?" -d":"",
        verbose?" -v":"",
        inspect?" -i":"",
        optimize?" -O":"",
        nosite?" -S":"",
        tabs?" -t":"",
        others,
        script,
        with_terminal? "" : " &"];
}

// FileSettingsSource protocol 
- (NSString *) interpreter { return interpreter;};
- (BOOL) debug { return debug;};
- (BOOL) verbose { return verbose;};
- (BOOL) inspect { return inspect;};
- (BOOL) optimize { return optimize;};
- (BOOL) nosite { return nosite;};
- (BOOL) tabs { return tabs;};
- (NSString *) others { return others;};
- (BOOL) with_terminal { return with_terminal;};

@end
