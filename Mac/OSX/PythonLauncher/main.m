//
//  main.m
//  PythonLauncher
//
//  Created by Jack Jansen on Fri Jul 19 2002.
//  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <unistd.h>

int main(int argc, const char *argv[])
{
	char *home = getenv("HOME");
	if (home) chdir(home);
    return NSApplicationMain(argc, argv);
}
