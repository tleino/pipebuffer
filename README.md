# pipebuffer

*pipebuffer* provides low-level configurable pipeline buffering for
Unix-like shells that can tell whether it is empty or full.

Originally written in 30-minutes for an embedded device as an example
for solving a particular problem for a friend, but for some reason it
is still used ;)

## Dependencies

None.

## Configure

Edit pipebuffer.c and edit variables to taste:

* BLOCK
* BUFFER
* WDELAY
* EMPTY_SIGNAL_FILE
* FULL_SIGNAL_FILE

## Compile

Edit pipebuffer.c 

	$ cc pipebuffer.c -opipebuffer

Or with debug:

	$ cc -g -DDEBUG pipebuffer.c -opipebuffer
