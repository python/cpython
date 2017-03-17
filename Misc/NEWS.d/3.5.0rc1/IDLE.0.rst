- Issue #23672: Allow Idle to edit and run files with astral chars in name.
  Patch by Mohd Sanad Zaki Rizvi.

- Issue #24745: Idle editor default font. Switch from Courier to
  platform-sensitive TkFixedFont.  This should not affect current customized
  font selections.  If there is a problem, edit $HOME/.idlerc/config-main.cfg
  and remove 'fontxxx' entries from [Editor Window].  Patch by Mark Roseman.

- Issue #21192: Idle editor. When a file is run, put its name in the restart bar.
  Do not print false prompts. Original patch by Adnan Umer.

- Issue #13884: Idle menus. Remove tearoff lines. Patch by Roger Serwy.

