exportloop - version 1.1.2
Export Loop VB - version 1.1.1
Copyright © 2015 libertyernie

https://github.com/libertyernie/exportloop

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

----------------------------------------

Export Loop VB is a frontend to exportloop.exe, a C application that uses
vgmstream to convert compatible video game music formats into WAV files.
Supported formats include BRSTM, HPS, Ogg Vorbis, and many others.

exportloop.exe and its DLL files must be in the same directory as this app!

To add files, use the Add button or drag-and-drop them into the list.

This program can export up to three WAV files for each song:
* From the song start to the loop start
  * The WAV filename will end with the string "(beginning)"
* From the loop start to the end
  * The WAV filename will end with the string "(loop)"
* From the song start to the end (the entire song)
  * The WAV filename will be the same as the BRSTM filename (no suffix)

The output directory defaults to the current folder, but you can change it
using the Browser button. You can also open the folder in File Explorer with
the Open button.
