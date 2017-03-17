Issue #27936: The round() function accepted a second None argument
for some types but not for others.  Fixed the inconsistency by
accepting None for all numeric types.