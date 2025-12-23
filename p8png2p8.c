#if 0
gcc  -Wall -pedantic -o p8png2p8 p8png2p8.c lodepng.c image2raw.c lexaloffle/pxa_compress_snippets.c lexaloffle/p8_compress.c -s -DUSE_LODEPNG -D_DO_MAIN -s && exit 0
gcc ${0} -o ${0%.*} && ./${0%.*} && exit 0
cc -DCOMPILER=cc ${0} -o ${0%.*} && ./${0%.*} && exit 0
g++ ${0} -o ${0%.*} && ./${0%.*} && exit 0
egcs ${0} -o ${0%.*} && ./${0%.*} && exit 0
c99 ${0} -o ${0%.*} && ./${0%.*} && exit 0
c89 ${0} -o ${0%.*} && ./${0%.*} && exit 0
echo "ERROR: could not find any C or C++ compiler" ; exit 0
#endif
// gcc -o p8png2p8 p8png2p8.c lodepng.c pxa_compress_snippets.c p8_compress.c -s -DUSE_LODEPNG
// gcc -o p8png2p8 p8png2p8.c lodepng.c pxa_compress_snippets.c p8_compress.c -s -DUSE_LIBPNG -I/usr/include/libpng16 -lpng16 
/*
      create image showing the p8 contents visually
      also create p8 file (p8png2p8)
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#define DO_HEADER
#include "image2raw.c"
#undef  DO_HEADER


/*
  https://www.lexaloffle.com/dl/docs/pico-8_manual.html
  https://github.com/pico-8/awesome-PICO-8
  https://pico-8.fandom.com/wiki/Memory#Memory_map
  https://pico-8.fandom.com/wiki/P8FileFormat
  https://pico-8.fandom.com/wiki/P8PNGFileFormat#New_Compressed_Format
  https://github.com/lvandeve/lodepng/blob/master/examples/example_decode.c
*/

 
extern int pico8_code_section_decompress(unsigned char *in_p, unsigned char *out_p, int max_len);


#define MIN(a,b) ((a)<(b)?(a):(b))
// 32800 == 160 * 205 == 0x8020
//#define P8_CODEDATA_SIZE 32800
#define P8_CODEDATA_SIZE sizeof (st_data_t)
#define P8_IMAGE_W 160
#define P8_IMAGE_H 205
// 160 * 205 * 4 == 131200
#define P8_IMAGE_SIZE (P8_IMAGE_W*P8_IMAGE_H*4)


typedef struct
{
/*
https://pico-8.fandom.com/wiki/P8PNGFileFormat
Bytes 0x0000-0x42ff are the spritesheet, map, flags, music, and sound effects data. 

Bytes 0x4300-0x7fff are the Lua code.
If the first four bytes (0x4300-0x4303) are a null (\x00) followed by
pxa, then the code is stored in the new (v0.2.0+) compressed format. 

If the first four bytes (0x4300-0x4303) are :c: followed by a null
(\x00), then the code is stored in the old (pre-v0.2.0) compressed
format.

The next two bytes (0x4304-0x4305) are the length of the decompressed code, stored MSB first.
The next two bytes (0x4306-0x4307) are the length of the compressed data + 8 for this 8-byte header, stored MSB first.
The remainder (0x4308-0x7fff) is the compressed data.

In all other cases, the code is stored as plaintext (ASCII), up to the first null byte (if any).
*/
  unsigned char gfx[0x2000]; // sprite_sheet 1+2
  unsigned char map[0x1000]; // p8map
  unsigned char gff[0x100]; // sprite flags, gfx_props
  unsigned char music[0x100]; // song
  unsigned char sfx[0x1100]; // sound effects
  unsigned char codedata[0x4300];  // codedata
  unsigned char version;
  unsigned char pad[0x1f];
} st_data_t;


typedef struct
{
  unsigned char magic[4];
  unsigned short decompressed;
  unsigned short compressed;
} st_codeheader_t;


typedef struct
{
  st_data_t *d;
  st_codeheader_t *h;
  unsigned char image[P8_IMAGE_SIZE];
  unsigned char codedata[P8_CODEDATA_SIZE]; // uncompressed
  unsigned char *lua_code;
} st_p8file_t;


static unsigned char p8pal[16][3] = {
{0x00,0x00,0x00},
{0x1D,0x2B,0x53},
{0x7E,0x25,0x53},
{0x00,0x87,0x51},

{0xAB,0x52,0x36},
{0x5F,0x57,0x4F},
{0xC2,0xC3,0xC7},
{0xFF,0xF1,0xE8},

{0xFF,0x00,0x4D},
{0xFF,0xA3,0x00},
{0xFF,0xEC,0x27},
{0x00,0xE4,0x36},

{0x29,0xAD,0xFF},
{0x83,0x76,0x9C},
{0xFF,0x77,0xA8},
{0xFF,0xCC,0xAA},
};


static unsigned char *
uchar_to_bits (unsigned char c)
{
  int i = 0;
  static unsigned char s[8];
  while (i < 8)
    {
      s[i] = (c&i ? '1' : '0');
      i++;
    }
  s[i] = 0;
  return s;
}


/*
static int
p8relabel (const char *code_s, const char *label_s, const char *output_s)
{
  unsigned char *code_image;
  unsigned char *label_image;
  unsigned width, height;
  unsigned pitch;

  code_image = png2raw (code_s, &width, &height, &pitch);
  label_image = png2raw (label_s, &width, &height, &pitch);

  for (int i = 0; i < P8_IMAGE_SIZE; i++)
    code_image[i] = (label_image[i] & 0xFC) | (code_image[i] & 0x03);
  free (label_image);

  raw2png (output_s, code_image, P8_IMAGE_W, P8_IMAGE_H, pitch);
  free (code_image);

  return 0;
}
*/


/*
static int
p8split (const char *cart_s, const char *code_s, const char *label_s)
{
  unsigned char *image;
  unsigned char *code_image;
  unsigned char *label_image;
  unsigned width, height;
  unsigned pitch;

  code_image = malloc (P8_IMAGE_SIZE);
  if (code_image == NULL)
    {
      fprintf (stderr, "ERROR: could not allocate memory\n");
      exit (1);
    }
  label_image = malloc (P8_IMAGE_SIZE);
  if (label_image == NULL)
    {
      free (code_image);
      fprintf (stderr, "ERROR: could not allocate memory\n");
      exit (1);
    }
  image = png2raw (cart_s, &width, &height, &pitch);

  for (int i = 0; i < P8_IMAGE_SIZE; i++)
    {
      unsigned char code = (image[i] & 0x03);
      if (i % 4 != 3)
        code_image[i] = code | code << 2 | code << 4 | code << 6;
      else
        code_image[i] = code | 0xFC;

      label_image[i] = (image[i] & 0xFC) | 0x03;
    }
  free (image);

  raw2png (code_s, code_image, P8_IMAGE_W, P8_IMAGE_H, pitch);
  free (code_image);

  raw2png (label_s, label_image, P8_IMAGE_W, P8_IMAGE_H, pitch);
  free (label_image);

  return 0;
}
*/


static unsigned char *
p8png2data (const char *p8png)
{
  int width, height, pitch;
  unsigned char *t = png2raw (p8png, &width, &height, &pitch);
  unsigned char * data = malloc (width * height * 4);

  if (data)
    for (int i = 0; i < width * height; i++)
      {
        data[i] =
          (t[i * 4 + 2] & 0x03) |
          (t[i * 4 + 1] & 0x03) << 2 |
          (t[i * 4 + 0] & 0x03) << 4 |
          (t[i * 4 + 3] & 0x03) << 6;
      }

  free (t);
  return data;
}


int
main (int argc, char **argv)
{
  int x = 0, y = 0, w = 0, h = 0;
  unsigned char data[0x10000 + 2];
  st_p8file_t p8;

//  if (isatty (STDOUT_FILENO))
  if (argc < 2)
    {
      fprintf (stderr, "FILE.P8.PNG");
      exit (0);
    }
  
  p8.d = (st_data_t *) p8png2data (argv[1]);
  if (!p8.d)
    {
      fprintf (stderr, "ERROR: malloc()\n");
      exit (0);
    }

  p8.h = (st_codeheader_t *) &p8.d->codedata;

  printf (
    "pico-8 cartridge // http://www.pico-8.com\n"
    "version %d\n", p8.d->version);

/*
Lua

The Lua section begins with the delimiter __lua__ on a line by itself. 
Subsequent lines up to the next delimiter (__gfx__) contain the cart's Lua
code as plaintext.

It is possible to use an external text editor to insert characters in the
Lua code section that the built-in editor does not support.  Uppercase ASCII
letters appear as lowercase in the built-in editor, and are converted to
lowercase if the cart is saved by PICO-8.  Non-ASCII characters may not
appear correctly in the built-in editor.

The built-in editor stores all letter characters as lowercase.  Glyphs are
stored as Unicode characters, encoded in UTF-8, with similar appearances. 
For example, shift-C in the built-in editor (cat glyph) is stored as the cat
emoji (????).
*/
  printf ("\n\n__lua__\n");
  pico8_code_section_decompress (p8.d->codedata, data, MIN (p8.h->decompressed, 0x10000));
  printf ((char *) data);
  printf (
    "\n--magic: %02x%02x%02x%02x \"%c%c%c%c\" decompressed: %d compressed: %d\n",
     p8.h->magic[0], p8.h->magic[1], p8.h->magic[2], p8.h->magic[3], 
#define ISPRINT(c) (isprint(c)?c:'.')
     ISPRINT (p8.h->magic[0]), ISPRINT (p8.h->magic[1]), ISPRINT (p8.h->magic[2]), ISPRINT (p8.h->magic[3]),
     p8.h->decompressed, p8.h->compressed);

/*
Spritesheet

The spritesheet section begins with the delimiter __gfx__.

The spritesheet is represented in the .p8 file as 128 lines of 128
hexadecimal digits.  Each line is a pixel row, and each hexadecimal digit is
a pixel color value, in pixel order.

This differs from the in-memory representation in that the 4-bit hexadecimal
digits appear in pixel order in the .p8 file, while pairs are swapped (least
significant nybble first) in memory.  This allows you to identify and draw
images using hex digits with a text editor, if you like.  If a manually
edited file's __gfx__ section includes characters outside of the hexadecimal
range, they're loaded as pixels of color 0.

A cart is allowed to use the bottommost 128 sprites as the bottommost 128x32
tiles of the map data.  That is, if the cart calls map() with coordinates in
that region, the data is read from the bottom of the spritesheet, and the
map editor can view this memory either way.

In the .p8 file, this data is always saved in the __gfx__ section, even if
the cart uses it as map data.  Note that this is encoded as a linear series
of 4-bit pixels, rather than a series of 8-bit bytes.  Tools reading this
section and converting to byte format should treat the first hex digit of
each pair as the bottom 4 bits and the second digit as the top 4 bits.
*/  
  printf ("\n\n__gfx__\n");
  w = 64;
//  h = sizeof (p8.d->gfx) / (w * 2);
  h = 128;
  for (y = 0; y < h; y++)
    {
  for (x = 0; x < w; x++)
    if ((y * w + x) < sizeof (p8.d->gfx))
    {
#if 0
      if (y > 0 && x == 0)
      printf ("\033[0m\n");
      printf ("\033[48;2;%d;%d;%dm \033[48;2;%d;%d;%dm ",
        p8pal[p8.d->gfx[y * w + x] & 0xf][0],
        p8pal[p8.d->gfx[y * w + x] & 0xf][1],
        p8pal[p8.d->gfx[y * w + x] & 0xf][2],
        p8pal[p8.d->gfx[y * w + x] >> 4][0],
        p8pal[p8.d->gfx[y * w + x] >> 4][1],
        p8pal[p8.d->gfx[y * w + x] >> 4][2]
        );
#else
//      printf ("\n");
      printf ("%x%x", p8.d->gfx[y * w + x] & 0xf, p8.d->gfx[y * w + x] >> 4);
#endif
    }
      printf ("\n");
    }
//  printf ("\033[0m");

/*
Sprite flags

The sprite flags section begins with the delimiter __gff__.

Flags are represented in the .p8 file as 2 lines of 256 hexadecimal digits
(128 bytes).  Each pair of digits represents the 8 flags (most significant
nybble first) for each of the 256 sprites, in sprite ID order.

In the graphics editor, the flags are arranged left to right from LSB to
MSB: red=1, orange=2, yellow=4, green=8, blue=16, purple=32, pink=64,
peach=128.
*/
  printf ("\n\n__gff__\n");
  h = 2;
//  w = sizeof (p8.d->gff) / (h * 2);
  w = 128;
  for (y = 0; y < h; y++)
  {
  for (x = 0; x < w; x++)
    {
//      if (y > 0 && x == 0)
//      printf ("\033[0m\n");
//      printf ("\033[48;5;%dm \033[48;5;%dm ", p8.d->gff[y * w + x] & 0xf, p8.d->gff[y * w + x] >> 4);
//      printf ("%s %s\n", uchar_to_bits (p8.d->gff[y * w + x] & 0xf), uchar_to_bits (p8.d->gff[y * w + x] >> 4));
      printf ("%x%x", p8.d->gff[y * w + x] & 0xf, p8.d->gff[y * w + x] >> 4);
    }
//  printf ("\033[0m");
printf ("\n");
}
/*
Label

If you have stored a screenshot for the cartridge label by pressing F7, the
cart is saved with the screenshot.  This is used as a cartridge "label"
image when you save the cart as a .p8.png file.  The label is remembered in
this way if you load a .p8.png file and save it as a .p8 file.

The format for the __label__ data is identical to that of the spritesheet,
representing a square of 128 x 128 32-color pixels.  Pixels in the default
palette are encoded as hexadecimal digits from 0 to f, and those in the
alternate palette use g to v.  If a manually edited file's __label__ section
includes characters outside of the range from 0 to 9 or a to v, they're
loaded as pixels of color 0.
*/
  printf ("\n\n__label__\n");
//  printf ("\033[0m");

/*
Map

The map section begins with the delimiter __map__.

Map data is stored in the .p8 file as 32 lines of 256 hexadecimal digits
(128 bytes).  Each pair of digits (most significant nybble first) is the
sprite ID for a tile on the map, ordered left to right, top to bottom, for
the first 32 rows of the map.

The map area is 128 tiles wide by 64 tiles high.  Map memory describes the
top 32 rows.  If the cart author draws tiles in the bottom 32 rows, this is
stored in the bottom of the __gfx__ section.  (See above.)
*/
  printf ("\n\n__map__\n");
  w = 128;
//  h = sizeof (p8.d->map) / w;
  h = 32;
  for (y = 0; y < h; y++)
  {
  for (x = 0; x < w; x++)
    {
//      if (y > 0 && x == 0)
//        printf ("\033[0m\n");
//      printf ("\033[48;5;%dm ", p8.d->map[y * w + x]);
      printf ("%x%x", p8.d->map[y * w + x] & 0xf, p8.d->map[y * w + x] >> 4);
    }
//  printf ("\033[0m");
  printf ("\n");
} 
/*
Sound effects
The sound effects section begins with the delimiter __sfx__.

Sound data is stored in the .p8 file as 64 lines of 168 hexadecimal digits
(84 bytes, most significant nybble first), one line per sound effect (0-63).

The byte values (hex digit pairs, MSB) are as follows:

byte 0: The editor mode and filter switches. See Here for more info.
byte 1: The note duration, in multiples of 1/128 second.
byte 2: Loop range start, as a note number (0-63).
byte 3: Loop range end, as a note number (0-63).
bytes 4-84: 32 notes
Each note is represented by 20 bits = 5 nybbles = 5 hex digits.  (Two notes
use five bytes.) The nybbles are:

nybble 0-1: pitch (0-63): c-0 to d#-5, chromatic scale

nybble 2: waveform (0-F): 0 sine, 1 triangle, 2 sawtooth, 3 long square, 4
short square, 5 ringing, 6 noise, 7 ringing sine; 8-F are the custom
waveforms corresponding to sound effects 0 through 7 (PICO-8 0.1.11 "version
11" and later)

nybble 3: volume (0-7)

nybble 4: effect (0-7): 0 none, 1 slide, 2 vibrato, 3 drop, 4 fade_in, 5
fade_out, 6 arp fast, 7 arp slow; arpeggio commands loop over groups of four
notes at speed 2 (fast) and 4 (slow)

Note that this is very different from the in-memory layout for sound data.
*/
  printf ("\n\n__sfx__\n");
  w = 84;
  h = 64;
  for (y = 0; y < h; y++)
    {
  for (x = 0; x < w; x++)
    {
//      if (y > 0 && x == 0)
//      printf ("%s %s\n", uchar_to_bits (p8.d->sfx[y * w + x] & 0xf), uchar_to_bits (p8.d->sfx[y * w + x] >> 4));
      printf ("%x%x", p8.d->sfx[y * w + x] & 0xf, p8.d->sfx[y * w + x] >> 4);
    }
//  printf ("\033[0m");
      printf ("\n");
    }
/*
Music patterns
The sound effects section begins with the delimiter __music__.

Music patterns are represented in the .p8 file as 64 lines, one for each
pattern.  Each line consists of a hex-encoded flags byte (two digits), a
space, and four hex-encoded one-byte (MSB-first) sound effect IDs.

The flags byte has three flags on the lower bits (the higher five bits are unused):

0: begin pattern loop
1: end pattern loop
2: stop at end of pattern

The sound effect ID is in the range 0-63.  If a channel is silent for a music
pattern, its number is 64 + the channel number (0x41, 0x42, 0x43, or 0x44).
*/
  printf ("\n\n__music__\n");
  w = 5;
  h = 64;
  for (y = 0; y < h; y++)
    {
  for (x = 0; x < w; x++)
    {
//      if (y > 0 && x == 0)
//      printf ("%s %s\n", uchar_to_bits (p8.d->music[y * w + x] & 0xf), uchar_to_bits (p8.d->music[y * w + x] >> 4));
      printf ("%x%x", p8.d->sfx[y * w + x] & 0xf, p8.d->sfx[y * w + x] >> 4);
    }
//  printf ("\033[0m");
      printf ("\n");
    }

  return 0;
}
