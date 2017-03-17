Issue #26864: In urllib.request, change the proxy bypass host checking
against no_proxy to be case-insensitive, and to not match unrelated host
names that happen to have a bypassed hostname as a suffix.  Patch by Xiang
Zhang.