# TT2MID
Tiertex (GB/GBC) to MIDI converter

This tool converts music from Game Boy and Game Boy Color games using Tiertex's sound engine to MIDI format.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex).

Examples:
* TT2MID "Toy Story (E) [!].gb" 20
* TT2MID "Bug's Life, A (E) [C][!].gbc" 5
* TT2MID "FIFA Soccer '97 (UE) [S][!].gb" A

This tool was created by referencing the publicly released audio source code for many games by Mark Ortiz. The sequence format is a lot like Mark Cooksey's, but less complex.

## Supported games:
* A Bug's Life
* FIFA: Road to World Cup '98
* FIFA '97
* FIFA 2000
* Hercules
* The Hunchback of Notre Dame: Topsy Turvy Games
* Madden '97
* Madden NFL 2000
* Men in Black: The Series
* Mulan
* NBA Live '96
* NHL 2000
* Olympic Summer Games: Atlanta 1996
* Pocahontas
* Small Soldiers
* Toy Story
* Toy Story 2
* Triple Play 2001
* World Cup '98

## To do:
  * Support for other versions of the sound engine (SNES, MegaDrive)
  * GBS file support
