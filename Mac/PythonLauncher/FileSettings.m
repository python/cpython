//
//  FileSettings.m
//  PythonLauncher
//
//  Created by Jack Jansen on Sun Jul 21 2002.
//  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
//

#import "FileSettings.h"

@implementation FileSettings

+ (id)getFactorySettingsForFileType: (NSString *)filetype
{
    static FileSettings *fsdefault_py, *fsdefault_pyw, *fsdefault_pyc;
    FileSettings **curdefault;
    
    if ([filetype isEqualToString: @"Python Script"]) {
        curdefault = &fsdefault_py;
    } else if ([filetype isEqualToString: @"Python GUI Script"]) {
        curdefault = &fsdefault_pyw;
    } else if ([filetype isEqualToString: @"Python Bytecode Document"]) {
        curdefault = &fsdefault_pyc;
    } else {
        NSLog(@"Funny File Type: %@\n", filetype);
        curdefault = &fsdefault_py;
        filetype = @"Python Script";
    }
    if (!*curdefault) {
        *curdefault = [[FileSettings new] initForFSDefaultFileType: filetype];
    }
    return *curdefault;
}

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
        *curdefault = [[FileSettings new] initForDefaultFileType: filetype];
    }
    return *curdefault;
}

+ (id)newSettingsForFileType: (NSString *)filetype
{
    FileSettings *cur;
    
    cur = [FileSettings new];
    [cur initForFileType: filetype];
    return [cur retain];
}

- (id)initWithFileSettings: (FileSettings *)source
{
    self = [super init];
    if (!self) return self;
    
    interpreter = [source->interpreter retain];
    honourhashbang = source->honourhashbang;
    debug = source->debug;
    verbose = source->verbose;
    inspect = source->inspect;
    optimize = source->optimize;
    nosite = source->nosite;
    tabs = source->tabs;
    others = [source->others retain];
    scriptargs = [source->scriptargs retain];
    with_terminal = source->with_terminal;
    prefskey = source->prefskey;
    if (prefskey) [prefskey retain];
    
    return self;
}

- (id)initForFileType: (NSString *)filetype
{
    FileSettings *defaults;
    
    defaults = [FileSettings getDefaultsForFileType: filetype];
    self = [self initWithFileSettings: defaults];
    origsource = [defaults retain];
    return self;
}

//- (id)init
//{
//    self = [self initForFileType: @"Python Script"];
//    return self;
//}

- (id)initForFSDefaultFileType: (NSString *)filetype
{
    int i;
    NSString *filename;
    NSDictionary *dict;
    static NSDictionary *factorySettings;
    
    self = [super init];
    if (!self) return self;
    
    if (factorySettings == NULL) {
        NSBundle *bdl = [NSBundle mainBundle];
        NSString *path = [ bdl pathForResource: @"factorySettings"
                ofType: @"plist"];
        factorySettings = [[NSDictionary dictionaryWithContentsOfFile:
            path] retain];
        if (factorySettings == NULL) {
            NSLog(@"Missing %@", path);
            return NULL;
        }
    }
    dict = [factorySettings objectForKey: filetype];
    if (dict == NULL) {
        NSLog(@"factorySettings.plist misses file type \"%@\"", filetype);
        interpreter = [@"no default found" retain];
        return NULL;
    }
    [self applyValuesFromDict: dict];
    interpreters = [dict objectForKey: @"interpreter_list"];
    interpreter = NULL;
    for (i=0; i < [interpreters count]; i++) {
        filename = [interpreters objectAtIndex: i];
        filename = [filename stringByExpandingTildeInPath];
        if ([[NSFileManager defaultManager] fileExistsAtPath: filename]) {
            interpreter = [filename retain];
            break;
        }
    }
    if (interpreter == NULL)
        interpreter = [@"no default found" retain];
    origsource = NULL;
    return self;
}

- (void)applyUserDefaults: (NSString *)filetype
{
    NSUserDefaults *defaults;
    NSDictionary *dict;
    
    defaults = [NSUserDefaults standardUserDefaults];
    dict = [defaults dictionaryForKey: filetype];
    if (!dict)
        return;
    [self applyValuesFromDict: dict];
}
    
- (id)initForDefaultFileType: (NSString *)filetype
{
    FileSettings *fsdefaults;
    
    fsdefaults = [FileSettings getFactorySettingsForFileType: filetype];
    self = [self initWithFileSettings: fsdefaults];
    if (!self) return self;
    interpreters = [fsdefaults->interpreters retain];
    scriptargs = [@"" retain];
    [self applyUserDefaults: filetype];
    prefskey = [filetype retain];
    return self;
}

- (void)reset
{
    if (origsource) {
        [self updateFromSource: origsource];
    } else {
        FileSettings *fsdefaults;
        fsdefaults = [FileSettings getFactorySettingsForFileType: prefskey];
        [self updateFromSource: fsdefaults];
    }
}

- (void)updateFromSource: (id <FileSettingsSource>)source
{
    interpreter = [[source interpreter] retain];
    honourhashbang = [source honourhashbang];
    debug = [source debug];
    verbose = [source verbose];
    inspect = [source inspect];
    optimize = [source optimize];
    nosite = [source nosite];
    tabs = [source tabs];
    others = [[source others] retain];
    scriptargs = [[source scriptargs] retain];
    with_terminal = [source with_terminal];
    // And if this is a user defaults object we also save the
    // values
    if (!origsource) {
        NSUserDefaults *defaults;
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
            interpreter, @"interpreter",
            [NSNumber numberWithBool: honourhashbang], @"honourhashbang",
            [NSNumber numberWithBool: debug], @"debug",
            [NSNumber numberWithBool: verbose], @"verbose",
            [NSNumber numberWithBool: inspect], @"inspect",
            [NSNumber numberWithBool: optimize], @"optimize",
            [NSNumber numberWithBool: nosite], @"nosite",
            [NSNumber numberWithBool: tabs], @"tabs",
            others, @"others",
            scriptargs, @"scriptargs",
            [NSNumber numberWithBool: with_terminal], @"with_terminal",
            nil];
        defaults = [NSUserDefaults standardUserDefaults];
        [defaults setObject: dict forKey: prefskey];
    }
}

- (void)applyValuesFromDict: (NSDictionary *)dict
{
    id value;
    
    value = [dict objectForKey: @"interpreter"];
    if (value) interpreter = [value retain];
    value = [dict objectForKey: @"honourhashbang"];
    if (value) honourhashbang = [value boolValue];
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
    value = [dict objectForKey: @"tabs"];
    if (value) tabs = [value boolValue];
    value = [dict objectForKey: @"others"];
    if (value) others = [value retain];
    value = [dict objectForKey: @"scriptargs"];
    if (value) scriptargs = [value retain];
    value = [dict objectForKey: @"with_terminal"];
    if (value) with_terminal = [value boolValue];
}

- (NSString*)_replaceSingleQuotes: (NSString*)string
{
	/* Replace all single-quotes by '"'"', that way shellquoting will
	 * be correct when the result value is delimited  using single quotes.
	 */
	NSArray* components = [string componentsSeparatedByString:@"'"];

	return [components componentsJoinedByString:@"'\"'\"'"];
}

- (NSString *)commandLineForScript: (NSString *)script
{
    NSString *cur_interp = NULL;
    NSString* script_dir = NULL;
    char hashbangbuf[1024];
    FILE *fp;
    char *p;

    script_dir = [script substringToIndex:
	    [script length]-[[script lastPathComponent] length]];
    
    if (honourhashbang &&
       (fp=fopen([script cString], "r")) &&
       fgets(hashbangbuf, sizeof(hashbangbuf), fp) &&
       strncmp(hashbangbuf, "#!", 2) == 0 &&
       (p=strchr(hashbangbuf, '\n'))) {
            *p = '\0';
            p = hashbangbuf + 2;
            while (*p == ' ') p++;
            cur_interp = [NSString stringWithCString: p];
    }
    if (!cur_interp)
        cur_interp = interpreter;
        
    return [NSString stringWithFormat:
        @"cd '%@' && '%@'%s%s%s%s%s%s %@ '%@' %@ %s",
    	[self _replaceSingleQuotes:script_dir],
        [self _replaceSingleQuotes:cur_interp],
        debug?" -d":"",
        verbose?" -v":"",
        inspect?" -i":"",
        optimize?" -O":"",
        nosite?" -S":"",
        tabs?" -t":"",
        others,
        [self _replaceSingleQuotes:script],
        scriptargs ? scriptargs : @"",
        with_terminal? "&& echo Exit status: $? && exit 1" : " &"];
}

- (NSArray *) interpreters { return interpreters;};

// FileSettingsSource protocol 
- (NSString *) interpreter { return interpreter;};
- (BOOL) honourhashbang { return honourhashbang; };
- (BOOL) debug { return debug;};
- (BOOL) verbose { return verbose;};
- (BOOL) inspect { return inspect;};
- (BOOL) optimize { return optimize;};
- (BOOL) nosite { return nosite;};
- (BOOL) tabs { return tabs;};
- (NSString *) others { return others;};
- (NSString *) scriptargs { return scriptargs;};
- (BOOL) with_terminal { return with_terminal;};

@end
