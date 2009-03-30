/*
 * Simple tool for setting an icon on a file.
 */
#import <Cocoa/Cocoa.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: seticon ICON TARGET");
		return 1;
	}

	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSString* iconPath = [NSString stringWithUTF8String:argv[1]];
	NSString* filePath = [NSString stringWithUTF8String:argv[2]];

	[NSApplication sharedApplication];

	[[NSWorkspace sharedWorkspace]
		setIcon: [[NSImage alloc] initWithContentsOfFile: iconPath]
		forFile: filePath
		options: 0];
	[pool release];
	return 0;
}
