Issue #25328: smtpd's SMTPChannel now correctly raises a ValueError if both
decode_data and enable_SMTPUTF8 are set to true.