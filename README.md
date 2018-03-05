# MSX SDCC tests

Some tests I'm doing while learning to program in MSX using SDCC

## color-text

Print color text in screen 7.

I started writing this to implement it in a text editor, but I finally
rejected it because it was too slow. I might reconsider it in the future.

Try it online:
* [color-text on WebMSX](http://webmsx.org/?PRESETS=DOS2&DISKA_FILES_URL=https://github.com/fr3nd/msx-sdcc-tests/releases/download/v1.0/text.zip)

### How it works

* In text mode, it copies the full character table to memory.
* Switches to screen 7.
* Puts the character table in a non visible area of the screen.
* Every time a char is printed, it's copied from the hidden video memory to
  the specified position.
