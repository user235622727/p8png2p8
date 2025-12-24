#if 0
gcc  -Wall -pedantic image2raw.c lodepng.c -o image2raw -DUSE_LODEPNG -DDO_MAIN -s && exit 0
gcc ${0} -o ${0%.*} && ./${0%.*} && exit 0
cc -DCOMPILER=cc ${0} -o ${0%.*} && ./${0%.*} && exit 0
g++ ${0} -o ${0%.*} && ./${0%.*} && exit 0
egcs ${0} -o ${0%.*} && ./${0%.*} && exit 0
c99 ${0} -o ${0%.*} && ./${0%.*} && exit 0
c89 ${0} -o ${0%.*} && ./${0%.*} && exit 0
echo "ERROR: could not find any C or C++ compiler" ; exit 0
#endif
#ifdef  DO_HEADER
/*
image2raw.c

Copyright (c) 2016 - 2025 NoisyB


This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef IMAGE2RAW_H
#define IMAGE2RAW_H
/*
  raw
    returns always RGBA
    bits == 32
    pitch == w*4
*/
extern unsigned char *tga2raw (const char *filename, int *w, int *h,
                               int *pitch);
extern unsigned char *xpm2raw (const char *filename, int *w, int *h,
                               int *pitch);
extern unsigned char *png2raw (const char *filename, int *w, int *h,
                               int *pitch);
extern unsigned char *jpg2raw (const char *filename, int *w, int *h,
                               int *pitch);


#endif // IMAGE2RAW_H
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#ifdef  USE_NANOJPEG
#include "nanojpeg.h"
#endif
#ifdef  USE_LODEPNG
#include "lodepng.h"
#endif
#ifdef  USE_LIBPNG
#include <png.h>
#endif


#if 0
static const char *
getenv_s (const char *name, const char *def)
{
  const char *t = getenv (name);
  return t ? t : def;
}


static int
getenv_i (const char *name, int def)
{
  const char *t = getenv (name);
  return t ? strtol (t, NULL, 10) : def;
}
#else
#define getenv_s(n,def) ((const char *)(getenv(n)?getenv(n):(def)))
#define getenv_i(n,def) (getenv(n)?strtol(getenv(n),NULL,10):(def))
#endif
#define getargv_s(i,def) ((const char *)(((i)<argc&&argv[i])?argv[i]:(def)))
#define getargv_i(i,def) (((i)<argc&&argv[i])?strtol(argv[i],NULL,10):(def))


struct TGAheader
{
  unsigned char infolen;        // length of info field
  unsigned char has_cmap;       // 1 if image has colormap, 0 otherwise
  unsigned char type;
//    0  -  No image data included.
//    1  -  Uncompressed, color-mapped images.
//    2  -  Uncompressed, RGB images.
//    3  -  Uncompressed, black and white images.
//    9  -  Runlength encoded color-mapped images.
//   10  -  Runlength encoded RGB images.
//   11  -  Compressed, black and white images.
//   32  -  Compressed color-mapped data, using Huffman, Delta, and
//          runlength encoding.
//   33  -  Compressed color-mapped data, using Huffman, Delta, and
//          runlength encoding.  4-pass quadtree-type process.

  unsigned char cmap_start[2];  // index of first colormap entry
  unsigned char cmap_len[2];    // number of entries in colormap
  unsigned char cmap_bits;      // bits per colormap entry

  unsigned char yorigin[2];     // image origin (ignored here)
  unsigned char xorigin[2];
  unsigned char width[2];       // image size
  unsigned char height[2];
  unsigned char pixel_bits;     // bits/pixel
  unsigned char flags;
// flags: 00vhaaaa
//   h  horizontal flip
//   v  vertical flip
//   a  alpha bits
};


// read/write unaligned little-endian 16-bit ints
#define LE16(p) ((p)[0] + ((p)[1] << 8))
//#define SETLE16(p, v) ((p)[0] = (v), (p)[1] = (v) >> 8)


unsigned char *
tga2raw (const char *filename, int *width, int *height, int *pitch)
{
  int w = 0, h = 0;
  struct TGAheader hdr;

  FILE *src = fopen (filename, "rb");
  if (!src)
    {
      fprintf (stderr, "ERROR: opening %s\n", filename);
      return NULL;
    }

  if (!fread (&hdr, sizeof (struct TGAheader), 1, src))
    {
      fprintf (stderr, "ERROR: reading TGA data");
      return NULL;
    }

#define TGA_TYPE_RGB 2
  if (hdr.type != TGA_TYPE_RGB)
    {
      fprintf (stderr, "ERROR: only TGA RGBA w/o RLE supported\n");
      return NULL;
    }

  int bpp = hdr.pixel_bits;

  if (bpp != 24 && bpp != 32)
    {
      fprintf (stderr, "ERROR: only 24bit RGB or 32bit RGBA supported\n");
      return NULL;
    }

//fprintf (stderr, "SHIT3]");
//fflush (stderr);
//exit (0);

  fseek (src, hdr.infolen, SEEK_CUR);   // skip info field
  
  w = LE16 (hdr.width);
  h = LE16 (hdr.height);


  unsigned char *s = (unsigned char *) malloc (w * h * 4);
  if (s == NULL)
    return NULL;
  if (bpp == 32)
    {
       fread (s, 1, w * h * 4, src);
    }
  else
    {
      int x = 0, y = 0;
      unsigned char *p = (unsigned char *) malloc (w * h * 3);
      if (p == NULL)
        {
          free (s);
          return NULL;
        }
      fread (p, 1, w * h * 3, src);

       // turn rgb to rgba
       // TODO: flip
       for (y = 0; y < h; y++)
         for (x = 0; x < w; x++)
           {
             int o = y * w + x;
             int d = y * w + x;
             s[d * 4] = p[o * 3];
             s[d * 4 + 1] = p[o * 3 + 1];
             s[d * 4 + 2] = p[o * 3 + 2];
             s[d * 4 + 3] = 0xff;
           }

      free (p);
    }
    
  *width = w;
  *height = h;
  *pitch = w * 4;
  return s;
}


/*
 * Supports the XPMv3 format, EXCEPT:
 * - hotspot coordinates are ignored
 * - only colour ('c') colour symbols are used
 * - rgb.txt is not used (for portability), so only RGB colours
 *   are recognized (#rrggbb etc) - only a few basic colour names are
 *   handled
 *
 * The result is an 8bpp indexed surface if possible, otherwise 32bpp.
 * The colourkey is correctly set if transparency is used.
 *
 * Besides the standard API, also provides
 *
 *     SDL_Surface *IMG_ReadXPMFromArray(char **xpm)
 *
 * that reads the image data from an XPM file included in the C source.
 *
 * TODO: include rgb.txt here. The full table (from solaris 2.6) only
 * requires about 13K in binary form.
 */

/* Hash table to look up colors from pixel strings */
#define STARTING_HASH_SIZE 256

struct hash_entry
{
  char *key;
  unsigned int color;
  struct hash_entry *next;
};

struct color_hash
{
  struct hash_entry **table;
  struct hash_entry *entries;   /* array of all entries */
  struct hash_entry *next_free;
  int size;
  int maxnum;
};

static int
hash_key (const char *key, int cpp, int size)
{
  int hash;

  hash = 0;
  while (cpp-- > 0)
    {
      hash = hash * 33 + *key++;
    }
  return hash & (size - 1);
}

static struct color_hash *
create_colorhash (int maxnum)
{
  int bytes, s;
  struct color_hash *hash;

  /* we know how many entries we need, so we can allocate
     everything here */
  hash = (struct color_hash *) malloc (sizeof *hash);
  if (!hash)
    return NULL;

  /* use power-of-2 sized hash table for decoding speed */
  for (s = STARTING_HASH_SIZE; s < maxnum; s <<= 1)
    ;
  hash->size = s;
  hash->maxnum = maxnum;
  bytes = hash->size * sizeof (struct hash_entry **);
  hash->entries = NULL;         /* in case malloc fails */
  hash->table = (struct hash_entry **) malloc (bytes);
  if (!hash->table)
    {
      free (hash);
      return NULL;
    }
  memset (hash->table, 0, bytes);
  hash->entries =
    (struct hash_entry *) malloc (maxnum * sizeof (struct hash_entry));
  if (!hash->entries)
    {
      free (hash->table);
      free (hash);
      return NULL;
    }
  hash->next_free = hash->entries;
  return hash;
}

static int
add_colorhash (struct color_hash *hash,
               char *key, int cpp, unsigned int color)
{
  int index = hash_key (key, cpp, hash->size);
  struct hash_entry *e = hash->next_free++;
  e->color = color;
  e->key = key;
  e->next = hash->table[index];
  hash->table[index] = e;
  return 1;
}

/* fast lookup that works if cpp == 1 */
#define QUICK_COLORHASH(hash, key) ((hash)->table[*(unsigned char *)(key)]->color)

static unsigned int
get_colorhash (struct color_hash *hash, const char *key, int cpp)
{
  struct hash_entry *entry = hash->table[hash_key (key, cpp, hash->size)];
  while (entry)
    {
      if (memcmp (key, entry->key, cpp) == 0)
        return entry->color;
      entry = entry->next;
    }
  return 0;                     /* garbage in - garbage out */
}

static void
free_colorhash (struct color_hash *hash)
{
  if (hash)
    {
      if (hash->table)
        free (hash->table);
      if (hash->entries)
        free (hash->entries);
      free (hash);
    }
}

static char *linebuf;
static int buflen;
static const char *error;

/*
 * Read next line from the source.
 * If len > 0, it's assumed to be at least len chars (for efficiency).
 * Return NULL and set error upon EOF or parse error.
 */
static char *
get_next_line (char ***lines, FILE *src, int len)
{
  char *linebufnew;

  if (lines)
    {
      return *(*lines)++;
    }
  else
    {
      char c;
      int n;
      do
        {
          if (fread (&c, 1, 1, src) <= 0)
            {
              error = "Premature end of data";
              return NULL;
            }
        }
      while (c != '"');
      if (len)
        {
          len += 4;             /* "\",\n\0" */
          if (len > buflen)
            {
              buflen = len;
              linebufnew = (char *) realloc (linebuf, buflen);
              if (!linebufnew)
                {
                  free (linebuf);
                  error = "Out of memory";
                  return NULL;
                }
              linebuf = linebufnew;
            }
          if (fread (linebuf, 1, len - 1, src) <= 0)
            {
              error = "Premature end of data";
              return NULL;
            }
          n = len - 2;
        }
      else
        {
          n = 0;
          do
            {
              if (n >= buflen - 1)
                {
                  if (buflen == 0)
                    buflen = 16;
                  buflen *= 2;
                  linebufnew = (char *) realloc (linebuf, buflen);
                  if (!linebufnew)
                    {
                      free (linebuf);
                      error = "Out of memory";
                      return NULL;
                    }
                  linebuf = linebufnew;
                }
              if (fread (linebuf + n, 1, 1, src) <= 0)
                {
                  error = "Premature end of data";
                  return NULL;
                }
            }
          while (linebuf[n++] != '"');
          n--;
        }
      linebuf[n] = '\0';
      return linebuf;
    }
}

#define SKIPSPACE(p) do { while (isspace((unsigned char)*(p))) ++(p); } while (0)
#define SKIPNONSPACE(p) do { while (!isspace((unsigned char)*(p)) && *p) ++(p); } while (0)


unsigned char *
xpm2raw (const char *filename, int *width, int *height, int *pitch)
{
// https://en.wikipedia.org/wiki/X_PixMap
// http://nerdbude.com/xpm.html
  unsigned char *image = NULL;
  int index;
  int x, y;
  int w, h, ncolors, cpp;
  int indexed;
  unsigned char *dst;
  struct color_hash *colors = NULL;
  char *keystrings = NULL, *nextkey;
  char *line;
  char ***xpmlines = NULL;
  int pixels_len;

  error = NULL;
  linebuf = NULL;
  buflen = 0;

  FILE * src = fopen (filename, "rb");

  line = get_next_line (xpmlines, src, 0);
  if (!line)
    goto done;
  /*
   * The header string of an XPMv3 image has the format
   *
   * <width> <height> <ncolors> <cpp> [ <hotspot_x> <hotspot_y> ]
   *
   * where the hotspot coords are intended for mouse cursors.
   * Right now we don't use the hotspots but it should be handled
   * one day.
   */
  if (sscanf (line, "%d %d %d %d", &w, &h, &ncolors, &cpp) != 4
      || w <= 0 || h <= 0 || ncolors <= 0 || cpp <= 0)
    {
      error = "Invalid format description";
      goto done;
    }

  keystrings = (char *) malloc (ncolors * cpp);
  if (!keystrings)
    {
      error = "Out of memory";
      goto done;
    }
  nextkey = keystrings;

  /* Create the new surface */
  indexed = 0;
  image = (unsigned char *) malloc (w * h * 4);
  if (!image)
    {
      /* Hmm, some SDL error (out of memory?) */
      goto done;
    }

  /* Read the colors */
  colors = create_colorhash (ncolors);
  if (!colors)
    {
      error = "Out of memory";
      goto done;
    }
  for (index = 0; index < ncolors; ++index)
    {
      char *p;
      line = get_next_line (xpmlines, src, 0);
      if (!line)
        goto done;

      p = line + cpp + 1;

      /* parse a colour definition */
      for (;;)
        {
          char nametype;
//          char *colname;
          unsigned int rgb = 0, pixel;

          SKIPSPACE (p);
          if (!*p)
            {
              error = "colour parse error";
              goto done;
            }
          nametype = *p;
          SKIPNONSPACE (p);
          SKIPSPACE (p);
//          colname = p;
          SKIPNONSPACE (p);
          if (nametype == 's')
            continue;           /* skip symbolic colour names */

//            if (!color_to_rgb(colname, p - colname, &rgb))
//                continue;

          memcpy (nextkey, line, cpp);
          pixel = rgb;
          add_colorhash (colors, nextkey, cpp, pixel);
          nextkey += cpp;
          break;
        }
    }

  /* Read the pixels */
  pixels_len = w * cpp;
  dst = (unsigned char *) image;
  for (y = 0; y < h; y++)
    {
      line = get_next_line (xpmlines, src, pixels_len);
      if (!line)
        goto done;

      if (indexed)
        {
          /* optimization for some common cases */
          if (cpp == 1)
            for (x = 0; x < w; x++)
              dst[x] = (unsigned char) QUICK_COLORHASH (colors, line + x);
          else
            for (x = 0; x < w; x++)
              dst[x] = (unsigned char) get_colorhash (colors, line + x * cpp, cpp);
        }
      else
        {
          for (x = 0; x < w; x++)
            ((unsigned int *) dst)[x] = get_colorhash (colors,
                                                       line + x * cpp, cpp);
        }
//      dst += image->pitch;
    }

done:
  if (error)
    {
      if (image)
        {
          free (image);
          image = NULL;
        }
      fprintf (stderr, "%s", error);
    }
  if (keystrings)
    free (keystrings);
  free_colorhash (colors);
  if (linebuf)
    free (linebuf);

  *width = w;
  *height = h;
  *pitch = w * 4;

  return image;
}


unsigned char *
png2raw (const char *filename, int *w, int *h, int *pitch)
{
#ifdef USE_LODEPNG
  unsigned char *s = NULL;
  unsigned error =
    lodepng_decode32_file (&s, (unsigned *) w, (unsigned *) h, filename);
  if (error)
    {
//      fprintf (stderr, "ERROR: %s: %s\n", filename, lodepng_error_text (error));
//      return NULL;
    }
  *pitch = *w * 4;
  return s;
#endif
#ifdef  USE_LIBPNG
  FILE *dfile = fopen (filename, "rb");

  if (!dfile)
    {
      perror ("fopen data file");
      return NULL;
    }

  char pnghead[8];

  if (fread (pnghead, 1, 8, dfile) != 8)
    {
      perror ("read data");
      return NULL;
    }

  if (png_sig_cmp (pnghead, 0, 8) != 0)
    {
      fprintf (stderr, "data file is not a PNG!\n");
      return NULL;
    }

  png_structp d_png =
    png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!d_png)
    {
      fprintf (stderr, "Can't create png_structp for data img\n");
      return NULL;
    }

  png_infop d_inf = png_create_info_struct (d_png);

  png_init_io (d_png, dfile);
  png_set_sig_bytes (d_png, 8);

  png_read_info (d_png, d_inf);

  uint32_t d_w = png_get_image_width (d_png, d_inf);
  uint32_t d_h = png_get_image_height (d_png, d_inf);

  if ((d_w != P8_IMAGE_W) || (d_h != P8_IMAGE_H))
    {
      fprintf (stderr, "Data image doesn't match PICO-8's dimensions.\n"
               "PICO-8 cartridge PNGs have a size of 160x205.\n");
      return NULL;
    }

  int d_col = png_get_color_type (d_png, d_inf);
  if (d_col != PNG_COLOR_TYPE_RGB_ALPHA)
    {
      fprintf (stderr, "Data image should be 8-bit RGBA.\n");
      return NULL;
    }

  switch (d_col)
    {
    case PNG_COLOR_TYPE_PALETTE:
      png_set_palette_to_rgb (d_png);
      if (png_get_valid (d_png, d_inf, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (d_png);
      png_set_add_alpha (d_png, 0xFF, PNG_FILLER_AFTER);
      break;
    case PNG_COLOR_TYPE_GRAY:
      png_set_gray_to_rgb (d_png);
      png_set_add_alpha (d_png, 0xFF, PNG_FILLER_AFTER);
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
      png_set_gray_to_rgb (d_png);
      break;
    case PNG_COLOR_TYPE_RGB:
      png_set_add_alpha (d_png, 0xFF, PNG_FILLER_AFTER);
      break;
    }

  int d_passes = png_set_interlace_handling (d_png);
  png_read_update_info (d_png, d_inf);

  png_bytep *d_rowptrs = malloc (sizeof (png_bytep) * d_h);
  for (int y = 0; y < d_h; y++)
    d_rowptrs[y] = malloc (png_get_rowbytes (d_png, d_inf));

  png_read_image (d_png, d_rowptrs);
  fclose (dfile);

  return (unsigned char *) d_rowptrs;
#endif
  return NULL;
}


unsigned char *
jpg2raw (const char *filename, int *width, int *height, int *pitch)
{
#ifdef  USE_NANOJPEG
// https://keyj.emphy.de/nanojpeg/
   int w = 0, h = 0;
   unsigned char * s = NULL;
   int size;
   unsigned char *buf;
   FILE *f;
   int x = 0, y = 0;

   f = fopen(filename, "rb");
   if (!f)
     {
       fprintf(stderr, "Error opening the input file.\n");
       return NULL;
     }
   fseek(f, 0, SEEK_END);
   size = (int) ftell(f);
   buf = (unsigned char *) malloc (size);
   fseek(f, 0, SEEK_SET);
   size = (int) fread(buf, 1, size, f);
   fclose(f);

   njInit();
   if (njDecode(buf, size))
     {
       free (buf);
       fprintf(stderr, "Error decoding the input file.\n");
       return NULL;
     }
   free (buf);

   w = njGetWidth();
   h = njGetHeight();
   s = njGetImage();
//    njGetImageSize()
   unsigned char * p = malloc (w * h * 4);
   if (!njIsColor())
     {
       // turn gray to rgba
       for (y = 0; y < h; y++)
         for (x = 0; x < w; x++)
           {
             int o = y * w + x;
             p[o * 4] = s[o * 3];
             p[o * 4 + 1] = s[o * 3];
             p[o * 4 + 2] = s[o * 3];
             p[o * 4 + 3] = 0xff;
           }
     }
   else
     {
       // turn rgb to rgba
       for (y = 0; y < h; y++)
         for (x = 0; x < w; x++)
           {
             int o = y * w + x;
             p[o * 4] = s[o * 3];
             p[o * 4 + 1] = s[o * 3 + 1];
             p[o * 4 + 2] = s[o * 3 + 2];
             p[o * 4 + 3] = 0xff;
           }
    }
  njDone();
//  free (s);
  *width = w;
  *height = h;
  *pitch = w * 4;
  return p;
#else
  return NULL;
#endif
}


#ifdef  DO_MAIN
int
main (int argc, char **argv)
{
  const char *filename = NULL;

  if (isatty (STDOUT_FILENO))
    {
      fprintf (stderr, "FILENAME");
      exit (0);
    }
  filename = getargv_s (1, NULL);

  int w = 0;
  int h = 0;
  int pitch = 0;
  unsigned char *p = png2raw (filename, &w, &h, &pitch);
  fwrite (p, 1, w * h * 4, stdout);

  return 0;
}
#endif
#endif // DO_HEADER
