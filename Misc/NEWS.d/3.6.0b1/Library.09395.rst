Issue #22233: Break email header lines *only* on the RFC specified CR and LF
characters, not on arbitrary unicode line breaks.  This also fixes a bug in
HTTP header parsing.