
# bmpico

Quick and dirty converter to turn 8-bit Windows bitmaps into 4-bit Windows icons with transparency.

## Usage

./icoconv [-t index] \<bmp\_file\_in...\> \<ico\_file\_out\>

 -t index : Use the color at [index] on the palette as transparent.

Multiple input bitmaps will be muxed into a single icon file.

