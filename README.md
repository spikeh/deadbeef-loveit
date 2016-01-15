# deadbeef-loveit
deadbeef-loveit is a plugin for the DeaDBeeF audio player that allows you to
mark tracks as "loved" by using the context menu or a defined hotkey. It does
this by adding a new, user configurable metadata entry to the selected track.

## Installation
After cloning this repository, compile the plugin:

`make`

Install to your home directory:

`mkdir -p ~/.local/lib64`

`install -v -m 644 ./loveit.so ~/.local/lib64/deadbeef/`

(For 32-bit systems, copy to `~/.local/lib/deadbeef/` instead.)

## Usage
![](http://i.imgur.com/wWNdhEJ.jpg)
