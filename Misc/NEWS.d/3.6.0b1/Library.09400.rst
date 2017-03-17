Issue #22450: urllib now includes an "Accept: */*" header among the
default headers.  This makes the results of REST API requests more
consistent and predictable especially when proxy servers are involved.