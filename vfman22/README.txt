Changed since version 2.1 release (2013-01-25)
- Read the actual structure of the d88 image format, making it more robust against
  accidental interleaving (not used by Epson formatters).
- Restructured the code splitting it into more source files.
Changed since version 2.0 release (2010-03-21)
- added a 'vferase' program, just a stripped vfwrite, which removes a file from the image.
- added a remaining size line, both blocks and directory entries, to the vfread directory mode.
- added read-only, system and archive bit filters and indicators to vfread directory mode.
- added a 'vdf2d88' program, to convert the old vfloppy format of only the sector data of track 4 and up
  to d88 format.
- generated the header in vformat instead of copying it, making the d88header.bin file superfluous.
- fixed an off-by-one error causing the last block and extend to be unusable.