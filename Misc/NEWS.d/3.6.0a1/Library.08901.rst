Issue #25447: fileinput now uses sys.stdin as-is if it does not have a
buffer attribute (restores backward compatibility).