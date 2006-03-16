rem Check for a working sound-card - exit with 0 if OK, 1 otherwise.
set wmi = GetObject("winmgmts:")
set scs = wmi.InstancesOf("win32_sounddevice")
for each sc in scs
   set status = sc.Properties_("Status")
   wscript.Echo(sc.Properties_("Name") + "/" + status)
   if status = "OK" then
       wscript.Quit 0 rem normal exit
   end if
next
rem No sound card found - exit with status code of 1
wscript.Quit 1

