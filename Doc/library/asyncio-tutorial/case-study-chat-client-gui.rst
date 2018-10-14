Asyncio Case Study: Chat Application with GUI client
====================================================

TODO

Notes:

- server code remains identical to prior case study
- focus is on making a nice client
- The focus area here is: can you use asyncio if there is another
  blocking "loop" in the main thread? (common with GUIs and games)
  How do you do that?
- Mention any special considerations
- Show and discuss strategies for passing data between main thread
  (GUI) and the asyncio thread (IO).
- We can demonstrate the above with tkinter, allowing the
  case study to depend only on the stdlib
- Towards the end, mention how the design might change if
  the client was a browser instead of a desktop client.
  (can refer to the 3rd party websocket library, or aiohttp)
