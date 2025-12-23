#if 0
gcc -Wall -pedantic raw2ansi.c -o raw2ansi -DDO_MAIN -s && exit 0
gcc -Wall -pedantic ${0} -o ${0%.*} -DDO_MAIN && ./${0%.*} && exit 0
cc -DCOMPILER=cc ${0} -o ${0%.*} -DDO_MAIN && ./${0%.*} && exit 0
g++ ${0} -o ${0%.*} -DDO_MAIN && ./${0%.*} && exit 0
egcs ${0} -o ${0%.*} -DDO_MAIN && ./${0%.*} && exit 0
c99 ${0} -o ${0%.*} -DDO_MAIN && ./${0%.*} && exit 0
c89 ${0} -o ${0%.*} -DDO_MAIN && ./${0%.*} && exit 0
echo "ERROR: could not find any C or C++ compiler" ; exit 0
#endif
// gcc -Wall -pedantic raw2ansi.c -o raw2ansi -DDO_MAIN
/*
raw2ansi.c

Copyright (c) 2023-2025 NoisyB


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
#ifdef  DO_HEADER
#ifndef RAW2ANSI_H
#define RAW2ANSI_H


#define ESC_HIDE_CURSOR "\033[?25l"
#define ESC_RESTORE_CURSOR "\033[?25h"
#define ESC_CLEAR_SCREEN "\033[2J"
#define ESC_RESET "\033[0m"
#define ESC_GOTOXY "\033[%d;%dH"
//#define ESC_GOTOXY "\033[%d;%df"
//  \033[8;H;Wt will resize the window to WxH characters
//                supported by some terminals (incl. xterm after allowing window ops)
#define ESC_RESIZE "\033[8;%d;%dt"
#define ESC_DECSWL "\033#5"  // resets a line to normal single width and single height display mode
#define ESC_DECDWL "\033#6"  // set double width line
#define ESC_DECDHL_T "\033#3"  // set double height line top half
#define ESC_DECDHL_B "\033#4"  // set double height line bottom half


extern void terminal_get_size (int *rows, int *cols);
extern int terminal_get_cursor_pos (int *row, int *col);
extern void terminal_echo_off (void);
extern void terminal_echo_on (void);
extern void terminal_widescreen_off (void);
extern void terminal_widescreen_on (void);

extern void raw2ansi_init (void);
extern void raw2ansi_deinit (void);

#define  TYPE_RGBA     3
#define  TYPE_256COLOR 2
#define  TYPE_8COLOR   1
#define  TYPE_ASCII    0

#define  FONT_NO_UTF8  0
#define  FONT_UTF8     1
extern void raw2ansi_fast (unsigned char *s, int w, int h,
                           int frame_skip,  // skip frames to speed things up
                           int use_lines,   // ?
                           int interlace,   // update only every n'th line
                           int type,        // format/colors
                           int font,        // (no) utf8
                           char *subtitle
                           );
extern void raw2ansi (unsigned char *s, int w, int h, int type, int font);

#endif // RAW2ANSI_H
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>           // gettimeofday()
#include <termios.h>
#define DO_HEADER
#include "raw2ansi.c"
#undef DO_HEADER



/*
https://man7.org/linux/man-pages/man2/ioctl_console.2.html
https://man7.org/linux/man-pages/man2/ioctl_tty.2.html
https://linux.die.net/man/4/tty_ioctl
http://aa-project.sourceforge.net/aalib/
http://aa-project.sourceforge.net/tune/
https://en.wikipedia.org/wiki/ANSI_escape_code
https://en.wikipedia.org/wiki/ANSI_escape_code#Description
https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
https://gist.github.com/XVilka/8346728
https://github.com/termstandard/colors
https://terminalguide.namepad.de/seq/
*/


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


static int
misc_getfps (int height)
{
  static int last_line = 0, fps = 0, fcount = 0;
  int line = 0;

  struct timeval now;
  gettimeofday (&now, NULL);
  line = now.tv_usec * 0.000001 * height;
  if (line > height)
    line = height - 1;

  fcount++;
  if (last_line > line)
    {
      fps = fcount;
      fcount = 0;
    }
  last_line = line;
  return fps;
}


int
terminal_set_size (int rows, int cols)
{
#ifdef TIOCSWINSZ
  struct winsize win = {.ws_row = rows, .ws_col = cols };
  return ioctl (STDIN_FILENO, TIOCSWINSZ, &win);
#else
  // requires "Allow Window Ops" in xterm
  fprintf (stdout, ESC_RESIZE, rows, cols);
  return 0;
#endif
}


void
terminal_get_size (int *rows, int *cols)
//terminal_get_size (int fd, int *rows, int *cols, int *px_width, int *px_height)
{
#ifdef TIOCGWINSZ
  struct winsize win = { 0, 0, 0, 0 };
  if (ioctl (STDIN_FILENO, TIOCGWINSZ, &win) == 0)
    {
      if (cols)
        *cols = win.ws_col;
      if (rows)
        *rows = win.ws_row;
      // some terminals will return 0 for pixels
    }
#else
#define DEFAULT_WIDTH 80
#define DEFAULT_HEIGHT 25
  if (cols)
    *cols = DEFAULT_WIDTH;
  if (rows)
    *rows = DEFAULT_HEIGHT;
#endif
}


int
terminal_get_cursor_pos (int *row, int *col)
{
  fputs ("\033[6n", stdout);
  return fscanf (stdin, "\033[%d;%dR", row, col);
}


// Convert RGB24 to xterm-256 8-bit value
// https://codegolf.stackexchange.com/questions/156918/rgb-to-xterm-color-converter
static inline int
rgb_to_x256_func (int c, int x, int i)
{
  return abs (c -= i > 215 ? 8 + (i - 216) * 10 : x * 40 + !!x * 55);
}
static int
rgb_to_x256 (int r, int g, int b)
{
  int l, m, t, i;
  for (i = l = 240; ~i--;
       t = rgb_to_x256_func (r, i / 36, i) + 
           rgb_to_x256_func (g, i / 6 % 6, i) +
           rgb_to_x256_func (b, i % 6, i), t < l ? l = t, m = i : 1);
  return (m + 16) & 0xff;
}


static int oldtty_set = 0;
static struct termios oldtty;


void
terminal_echo_off (void)
{
  struct termios newtty;
  if (tcgetattr (STDIN_FILENO, &oldtty) == -1)
    {
      fputs ("ERROR: Could not get TTY parameters\n", stderr);
      return;
    }
  oldtty_set = 1;

  newtty = oldtty;
  newtty.c_lflag &= ~(ICANON | ECHO);
  newtty.c_lflag |= ISIG;
  newtty.c_cc[VMIN] = 1;        // if VMIN != 0, read() calls
  newtty.c_cc[VTIME] = 0;       //  block (wait for input)

  tcsetattr (STDIN_FILENO, TCSANOW, &newtty);
}


void
terminal_echo_on (void)
{
  if (oldtty_set)
    {
      tcsetattr (STDIN_FILENO, TCSAFLUSH, &oldtty);
      oldtty_set = 0;
    }
}


static int terminal_widescreen = 0;


void
terminal_widescreen_off (void)
{
  terminal_widescreen = 0;
}


void
terminal_widescreen_on (void)
{
  terminal_widescreen = 1;
}


void
raw2ansi_init (void)
//raw2ansi_init (int frame_skip, int use_lines, int interlace, int type, int font)
{
  fputs (ESC_HIDE_CURSOR, stdout);
//  fputs (ESC_CLEAR_SCREEN, stdout);
//  fprintf (stdout, ESC_GOTOXY, 0, 0);
}


void
raw2ansi_deinit (void)
{
  fputs (ESC_RESET, stdout);
  fputs (ESC_RESTORE_CURSOR, stdout);
}


static unsigned char current_color_bg[4] = { 0, 0, 0, 0 };
static unsigned char current_color_fg[4] = { 0, 0, 0, 0 };


static inline int
current_color_cmp (unsigned char color1[4], unsigned char color2[4])
{
//  return ((unsigned int) &color1) == ((unsigned int) &color2);
// 0b11000000 == 192
// 0b11100000 == 224
// 0b11110000 == 240
// 0b11111100 == 252
  // bgra
//  return ((color1[0] & 240) == (color2[0] & 240) &&
//          (color1[1] & 240) == (color2[1] & 240) && (color1[2] & 252) == (color2[2] & 252))
  return !((color1[0] & 224) + (color1[1] & 224) + (color1[2] & 240) -
           (color2[0] & 224) - (color2[1] & 224) - (color2[2] & 240));
}


static void
current_color_set (unsigned char d[4], unsigned char color[4])
{
//  d[0] = color[0];
//  d[1] = color[1];
//  d[2] = color[2];
//  d[3] = color[3];
  *d++ = *color++;
  *d++ = *color++;
  *d = *color;
}


static inline int
memcmp_fast (unsigned char *a, unsigned char *b, int len)
{
  if (len > 3)
    if ((*(int *)a) != (*(int *)b))
      return 1;
  if (len > 1)
    {
      register unsigned o = 1;
      do {
          if ((*(a + o)) != (*(b + o)))
            return 1;
          o += 16;
//          o += 32;
        } while (o < len);
//      return memcmp (a, b, len);
    }
  return 0;
}


void
raw2ansi_fast (unsigned char *s, int w, int h, int frame_skip, int use_lines,
               int interlace, int type, int font, char *subtitle)
{
  static unsigned char *old_frame = NULL;
  static int old_w;
  static int old_h;
  register int x = 0, y;

//  int use_lines = 1;
  static int dropped_lines = 0;
  static int old_dropped_lines = 0;

//  int frame_skip = 1;
  static int skipped_frames = 0;
  static int frame_skip_value = 0;

//  int interlace = 0;
  int interlace_max = 2;
  static int interlace_value = 0;

  if (frame_skip_value < frame_skip)
    {
      frame_skip_value++;
      skipped_frames++;
      return;
    }
  else
    frame_skip_value = 0;

  if (old_frame && (old_w != w || old_h != h))
    {
      free (old_frame);
      old_frame = 0;
    }

  if (!old_frame)
    {
      if ((old_frame = (unsigned char *) malloc (w * h * 4)) == NULL)
        {
//          fprintf (stderr, "raw2ansi->malloc()\n");
          return;
        }
      memset (old_frame, 0, w * h * 4);
      old_w = w;
      old_h = h;
    }

  if (interlace == 1)
    {
      interlace_value++;
      if (interlace_value >= interlace_max)
        interlace_value = 0;
    }
  int line = 0;
  for (y = 0; y < h; y += 2)
    {
      if (interlace == 1)
        {
          if (((y - interlace_value) % interlace_max) == 0)
            use_lines = 1;
          else
            {
              use_lines = 0;
              dropped_lines += 2;
              line++;
continue;

            }
        }

      if (                      //y == 0 ||
           use_lines &&
           (!memcmp_fast (&old_frame[y * w * 4],       &s[y * w * 4],       w * 4) &&
            !memcmp_fast (&old_frame[(y + 1) * w * 4], &s[(y + 1) * w * 4], w * 4)))
        {
// DEBUG
          dropped_lines += 2;
//          use_lines = 0;
      fprintf (stdout, "\033[%d;1H", line + 1);
line++;
continue;
        }

      fprintf (stdout, "\033[%d;1H", line + 1);
//fprintf (stdout, "%d ", line + 1);

      if (terminal_widescreen)
        fprintf (stdout, ESC_DECDWL);  // set double width line


//      if (use_lines)
      for (x = 0; x < w; x++)
        {
          register int i = (y * w + x) * 4;
          int use_color = 0;
          if (x == 0 ||
              current_color_cmp (current_color_bg, &s[i]) == 0 ||
              current_color_cmp (current_color_fg, &s[i + w * 4]) == 0)
            {
              current_color_set (current_color_bg, &s[i]);
              current_color_set (current_color_fg, &s[i + w * 4]);
              use_color = 1;
            }

          if (use_color == 1)
            {
              unsigned char *bg = &s[i];
              unsigned char *fg = &s[i + w * 4];
#define R 0
#define G 1
#define B 2
              if (font == FONT_UTF8)
                {
                  if (type == TYPE_RGBA)
                    fprintf (stdout, "\033[48;2;%d;%d;%dm\033[38;2;%d;%d;%dm",
                             fg[R], fg[G], fg[B], bg[R], bg[G], bg[B]);
                  else if (type == TYPE_256COLOR)
                    fprintf (stdout, "\033[48;5;%dm\033[38;5;%dm",
                             rgb_to_x256 (fg[R], fg[G], fg[B]), rgb_to_x256 (bg[R], bg[G], bg[B]));

                }
              else
                {
                  if (type == TYPE_RGBA)
                    fprintf (stdout, "\033[48;2;%d;%d;%dm",
                             (fg[R] + bg[R]) >> 1, (fg[G] + bg[G]) >> 1,
                             (fg[B] + bg[B]) >> 1);
                  else if (type == TYPE_256COLOR)
                    fprintf (stdout, "\033[48;5;%dm\033[38;5;%dm",
                             rgb_to_x256 (fg[R], fg[G], fg[B]), rgb_to_x256 (bg[R], bg[G], bg[B]));
                }
            }

          if (font == FONT_UTF8)
            fputs ("\xe2\x96\x80"       // UTF8 bytes of U+2580 (upper half block)
//            fputs ("\xe2\x96\x84"       // UTF8 bytes of U+2584 (lower half block)
                   , stdout);
          else
            fputc (' ', stdout);
        }

      if (terminal_widescreen)
        fprintf (stdout, ESC_DECSWL);  // resets a line to normal single width and single height display mode
//      fprintf (stdout, ESC_RESET);
//      fputc ('\n', stdout);
//fprintf (stdout, "\033[%d;1H", y);

      line++;
    }

  fprintf (stdout, ESC_RESET "\n");

  fprintf (stderr, "%d %d dropped lines: %d (per frame: %d) ", w, h, dropped_lines,
           dropped_lines - old_dropped_lines);
  old_dropped_lines = dropped_lines;
  fprintf (stderr, "fps: %d     \n", misc_getfps (h));

  memcpy (old_frame, s, w * h * 4);
}


void
raw2ansi (unsigned char *s, int w, int h, int type, int font)
{
  raw2ansi_fast (s, w, h, 0, 0, 0, type, font, NULL);
}


#ifdef  DO_MAIN
/*
  trap misc_ansi_winch SIGWINCH # INT TERM EXIT
         SIGWINCH       -        Ign     Window resize signal (4.3BSD, Sun)
         SIGKILL      P1990      Term    Kill signal
         SIGQUIT      P1990      Core    Quit from keyboard
         SIGINT       P1990      Term    Interrupt from keyboard
         SIGTERM      P1990      Term    Termination signal
         typedef void (*sighandler_t)(int);
        sighandler_t signal(int signum, sighandler_t handler);
*/
static void
signal_handler (int signum)
{
  (void) signum;
  fprintf (stdout, ESC_RESET);
  fprintf (stdout, ESC_RESTORE_CURSOR);
//  fprintf (stderr, "%d\n", signum);
  exit (0);
}


int
main (int argc, char **argv)
{
  int w = 0, h = 0, type = TYPE_RGBA, font = FONT_NO_UTF8;

  if (isatty (STDIN_FILENO))
    {
      fprintf (stderr, "W H [UTF8=0]");
      exit (0);
    }
  w = getargv_i (1, 40);
  h = getargv_i (2, 40);
  int utf8 = getargv_i (3, 0);
  if (utf8 == 1)
    font = FONT_UTF8;
 
//  setbuf (stdout, NULL);

  unsigned data_len = w * h * 4;
  unsigned char *data = (unsigned char *) malloc (data_len);

  if (!data)
    {
      fprintf (stderr, "ERROR: malloc()\n");
      exit (1);
    }

  fprintf (stdout, ESC_HIDE_CURSOR);

  signal (SIGINT, signal_handler);

  while (fread (data, 1, data_len, stdin) == data_len)
    {
      fprintf (stdout, ESC_GOTOXY, 0, 0);
      raw2ansi (data, w, h, type, font);
    }
  fprintf (stdout, ESC_RESTORE_CURSOR);

  return 0;
}
#endif
#endif // DO_HEADER
