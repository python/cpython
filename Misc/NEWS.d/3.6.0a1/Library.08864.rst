Issue #25590: In the Readline completer, only call getattr() once per
attribute.  Also complete names of attributes such as properties and slots
which are listed by dir() but not yet created on an instance.