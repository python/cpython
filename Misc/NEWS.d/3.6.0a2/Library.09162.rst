Issue #25738: Stop http.server.BaseHTTPRequestHandler.send_error() from
sending a message body for 205 Reset Content.  Also, don't send Content
header fields in responses that don't have a body.  Patch by Susumu
Koshiba.