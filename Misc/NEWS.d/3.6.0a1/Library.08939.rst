Issue #26402: Fix XML-RPC client to retry when the server shuts down a
persistent connection.  This was a regression related to the new
http.client.RemoteDisconnected exception in 3.5.0a4.