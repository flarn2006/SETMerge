SETMerge
========

Utility for merging level object lists for Sonic Adventure 1 and 2.

Typical usage pattern:

1. Create some objects [using Cheat Engine](https://github.com/flarn2006/SA2CheatTable)
2. Export the object list as a SET file (Show Spawned Objects->Save->Save as type: Native SET format)
3. `setmerge setXXXX_s.bin exported_file.bin setXXXX_s.bin` to append to original object list
4. Added objects will now be a "permanent" part of the game.
