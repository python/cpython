-- This AppleScript creates Finder aliases for all the
-- dynamically-loaded modules that "live in" in a single
-- shared library.
-- It needs a scriptable finder, and it may need some common
-- scripting additions (i.e. stuff that *I* happen to have:-)
-- 
-- If you compare it to MkPluginAliases.py it also serves
-- as a comparison between python and AppleScript:-)
--
-- Jack Jansen, CWI, August 1995

-- G is a list of {target, original} tuples

set G to {{"mactcp.slb", "mactcpmodules.slb"}}
set G to (G & {{"macdnr.slb", "mactcpmodules.slb"}})
set G to (G & {{"AE.slb", "toolboxmodules.slb"}})
set G to (G & {{"Ctl.slb", "toolboxmodules.slb"}})
set G to (G & {{"Dlg.slb", "toolboxmodules.slb"}})
set G to (G & {{"Evt.slb", "toolboxmodules.slb"}})
set G to (G & {{"Menu.slb", "toolboxmodules.slb"}})
set G to (G & {{"List.slb", "toolboxmodules.slb"}})
set G to (G & {{"Qd.slb", "toolboxmodules.slb"}})
set G to (G & {{"Res.slb", "toolboxmodules.slb"}})
set G to (G & {{"Snd.slb", "toolboxmodules.slb"}})
set G to (G & {{"Win.slb", "toolboxmodules.slb"}})
set G to (G & {{"imgcolormap.slb", "imgmodules.slb"}})
set G to (G & {{"imgformat.slb", "imgmodules.slb"}})
set G to (G & {{"imggif.slb", "imgmodules.slb"}})
set G to (G & {{"imgjpeg.slb", "imgmodules.slb"}})
set G to (G & {{"imgop.slb", "imgmodules.slb"}})
set G to (G & {{"imgpgm.slb", "imgmodules.slb"}})
set G to (G & {{"imgppm.slb", "imgmodules.slb"}})
set G to (G & {{"imgtiff.slb", "imgmodules.slb"}})

-- Find the plugins directory
set Dir to choose folder with prompt "Where is the PlugIns directory?"

-- List everything there
set AllFiles to list folder Dir

-- Remove .slb aliases and remember .slb files
display dialog "About to remove old .slb aliases"
set LibFiles to {}
repeat with F in AllFiles
	if F ends with ".slb" then
		set fileRef to ((Dir as text) & F) as alias
		set Info to info for fileRef
		if alias of Info then
			tell application "Finder"
				move fileRef to trash
			end tell
		else
			set LibFiles to (LibFiles & F)
		end if
	end if
end repeat

-- Open the folder, so we can talk to the finder about it
tell application "Finder"
	set FinderName to open (Dir as alias)
end tell

-- The "real" mainloop: create the aliases
display dialog "About to create new .slb aliases"
repeat with Goal in G
	set Dst to item 1 of Goal
	set Src to item 2 of Goal
	if LibFiles contains Src then
		tell application "Finder"
			set DstAlias to make alias to (((Dir as text) & Src) as alias)
			set name of DstAlias to Dst
		end tell
	else
		-- The original isn't there
		display dialog "Skipping alias " & Dst & " to " & Src
	end if
end repeat

display dialog "All done!"
