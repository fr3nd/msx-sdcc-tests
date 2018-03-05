// vim:foldmethod=marker:foldlevel=0
#include <stdio.h>
#include "VDPgraph2.h"

#define SCREEN7_BACKBUF 0x0D400
#define SCREEN7_OFFSETX 0
#define SCREEN7_OFFSETY 1
#define SCREEN7_SIZEX 512
#define SCREEN7_SIZEY 212
#define CHAR_SIZEX 3
#define CHAR_SIZEY 8
#define BACKBUFFER_LENGTH 85
#define SCREENCOLS 80

// Colors
#define TRANSPARENT    0x00
#define BLACK          0x01
#define MEDIUM_GREEN   0x02
#define LIGHT_GREEN    0x03
#define DARK_BLUE      0x04
#define LIGHT_BLUE     0x05
#define DARK_RED       0x06
#define CYAN           0x07
#define MEDIUM_RED     0x08
#define LIGHT_RED      0x09
#define DARK_YELLOW    0x0A
#define LIGHT_YELLOW   0x0B
#define DARK_GREEN     0x0C
#define MAGENTA        0x0D
#define GRAY           0x0E
#define WHITE          0x0F

/* DOS calls */
#define TERM    #0x62

/* BIOS calls */
#define CHGET  #0x009F
#define CHPUT  #0x00A2
#define CALSLT #0x001C
#define EXPTBL #0xFCC1
#define POSIT  #0x00C6
#define INITXT #0x006C
#define CLS    #0x00C3
#define CHGCLR #0x0062
#define CHGMOD #0x005F
#define WRTVRM #0x004D
#define NRDVRM #0x0174
#define NWRVRM #0x0177
#define FILVRM #0x0056
#define DISSCR #0x0041
#define ENASCR #0x0044

/* Memory variables */
#define LINL40 0xF3AE
#define FORCLR 0xF3E9
#define BAKCLR 0xF3EA
#define BDRCLR 0xF3EB

/* Vram positions */
#define CHARTABLE 0x01000 // 80 cols

#define DOSCALL  call 5
#define BIOSCALL ld iy,(EXPTBL-1)\
call CALSLT

/*** global variables {{{ ***/

struct coord {
  int x, y;
};

char fgcolor;
char bgcolor;
char inverted;

struct coord cursor_pos;
unsigned char char_table[8*255];
char escape_sequence = 0;
char escape_sequence_2 = 0;
char escape_sequence_y = 0;

/*** end global variables }}} ***/

/*** functions {{{ ***/

char getchar(void) __naked {
  __asm
    push ix

    ld ix,CHGET
    BIOSCALL
    ld h, #0x00
    ld l,a ;reg to put a read character

    pop ix
    ret
  __endasm;
}

void initxt(char columns) __naked {
  columns;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld a,(ix)
    ld (LINL40),a

    ld ix,INITXT
    BIOSCALL

    pop ix
    ret
  __endasm;
}

void gotoxy(int x, int y) {
  cursor_pos.x = x;
  cursor_pos.y = y;
}

void cls(void) __naked {
  __asm
    push ix
    cp a
    ld ix,CLS
    BIOSCALL
    pop ix
    ret
  __endasm;
}

void exit(int code) __naked {
  code;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld b,(ix)
    ld c, TERM
    DOSCALL

    pop ix
    ret
  __endasm;
}
/*** end functions }}} ***/

/*** graphic functions {{{ ***/

void color(char fg, char bg, char bd) __naked {
  fg;
  bg;
  bd;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld a,0(ix)
    ld(#FORCLR),a
    ld a,1(ix)
    ld(#BAKCLR),a
    ld a,2(ix)
    ld(#BDRCLR),a

    ld ix,CHGCLR
    BIOSCALL

    pop ix
    ret
  __endasm;
}

void screen(char mode) __naked {
  mode;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld a,0(ix)
    ld ix,CHGMOD
    BIOSCALL

    pop ix
    ret
  __endasm;
}

void vpoke(unsigned int address, unsigned char value) __naked {
  address;
  value;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld l,0(ix)
    ld h,1(ix)
    ld a,2(ix)
    ld ix,NWRVRM
    BIOSCALL

    pop ix
    ret
  __endasm;
}

unsigned char vpeek(unsigned int address) __naked {
  address;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld l,0(ix)
    ld h,1(ix)
    ld ix,NRDVRM
    BIOSCALL

    ld h, #0x00
    ld l,a

    pop ix
    ret
  __endasm;
}

void fillvram(char data, int length, int address) __naked {
  data;
  length;
  address;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld a,0(ix)
    ld c,1(ix)
    ld b,2(ix)
    ld l,3(ix)
    ld h,4(ix)
    ld ix,FILVRM
    BIOSCALL

    pop ix
    ret
  __endasm;
}


void putchar(char c) {
  MMMtask vdptask;
  unsigned char n,m;


  if (escape_sequence == 1) {
    // If previous char was an escape sequence \33
    if (c == 'K') {
      // Delete everything from the cursor position to the end of line
      escape_sequence = 0;

      vdptask.X2 = SCREEN7_OFFSETX + cursor_pos.x*CHAR_SIZEX*2;
      vdptask.Y2 = SCREEN7_OFFSETY + cursor_pos.y*CHAR_SIZEY;
      vdptask.DX = SCREEN7_SIZEX - cursor_pos.x*CHAR_SIZEX*2;
      vdptask.DY = CHAR_SIZEY;
      vdptask.s0 = 0x11;
      vdptask.DI = 0;
      vdptask.LOP = opHMMV;
      fLMMM(&vdptask);

      cursor_pos.x = SCREENCOLS;
    } else if (c == 'x') {
      // Cursor
      // TODO
    } else if (c == 'm') {
      // Color
      // Based on ANSI but different
      // Colors from 0 to 15 (+32)
      // inverted 16+32
      escape_sequence_2 = 1;
    } else if (escape_sequence_2 == 1) {
      if (c-32 <= 15) {
        fgcolor = c-32;
      } else {
        if (inverted == 0) {
          inverted = 1;
        } else {
          inverted = 0;
        }
      }
      escape_sequence_2 = 0;
      escape_sequence = 0;
    } else if (c == 'y') {
      // Cursor
      // TODO
    } else if (c == 'H') {
      // Move to top left corner
      gotoxy(0, 0);
      escape_sequence = 0;
    } else if (c == 'Y') {
      // Move cursor to xy position
      escape_sequence_2 = 2;
      escape_sequence_y = 127;
    } else if (escape_sequence_2 == 2) {
      if (escape_sequence_y == 127) {
        escape_sequence_y = c;
      } else {
        gotoxy(c-32, escape_sequence_y-32);
        escape_sequence_2 = 0;
        escape_sequence = 0;
        escape_sequence_y = 0;
      }

    } else {
      escape_sequence = 0;
    }
  } else {
    // If previous char wasn't an escape sequence \33
    if (c >= 0x20) {
      n = c%BACKBUFFER_LENGTH;
      m = c/BACKBUFFER_LENGTH;

      // Set background
      vdptask.X2 = SCREEN7_OFFSETX + cursor_pos.x*CHAR_SIZEX*2;
      vdptask.Y2 = SCREEN7_OFFSETY + cursor_pos.y*CHAR_SIZEY;
      vdptask.DX = CHAR_SIZEX*2;
      vdptask.DY = CHAR_SIZEY;
      vdptask.s0 = (fgcolor<<4) + fgcolor;
      vdptask.DI = 0;
      vdptask.LOP = opHMMV;
      fLMMM(&vdptask);

      // Set char
      vdptask.X = n*CHAR_SIZEX*2;
      vdptask.Y = 212+m*CHAR_SIZEY;
      vdptask.DX = CHAR_SIZEX*2;
      vdptask.DY = CHAR_SIZEY;
      vdptask.X2 = SCREEN7_OFFSETX + cursor_pos.x*CHAR_SIZEX*2;
      vdptask.Y2 = SCREEN7_OFFSETY + cursor_pos.y*CHAR_SIZEY;
      vdptask.s0 = 0;
      vdptask.DI = 0;
      if (inverted == 0) {
        vdptask.LOP = LOGICAL_AND;
      } else {
        vdptask.LOP = LOGICAL_XOR;
      }
      fLMMM(&vdptask);

      cursor_pos.x++;
    } else if (c == '\n') {
      cursor_pos.y++;
    } else if (c == '\r') {
      cursor_pos.x = 0;
    } else if (c == '\33') {
      escape_sequence = 1;
    }
  }
}

void vputchar_vram(unsigned char c, unsigned int addr) {
  int x,y;
  unsigned char b, p1, p2, p3;

  for (y=0; y<CHAR_SIZEY; y++) {
    b = char_table[c*CHAR_SIZEY + y];
    for (x=0; x<CHAR_SIZEX; x++) {
      p1 = ((b >> (CHAR_SIZEX*2)-x*2+1) & 1U) * fgcolor;
      p2 = ((b >> (CHAR_SIZEX*2)-x*2) & 1U) * fgcolor;
      if (p1 == 0) p1 = bgcolor;
      if (p2 == 0) p2 = bgcolor;
      p3 = (p1 << 4) + p2;
      vpoke(addr + x + y*SCREEN7_SIZEX/2 ,p3);
    }
  }
}

void vcolorprint(char* str) {
  int x=0;

  while (str[x] != '\0') {
    putchar(str[x]);
    gotoxy(cursor_pos.x++,cursor_pos.y);
    x++;
  }
}

/*** end graphic functions }}} ***/


/*** main {{{ ***/

int main(char **argv, int argc) {
  int n, y, x;

  Save_VDP();		// Save VDP internals
  // Get the char table
  initxt(80);
  for (n = 0; n < 8*255; n++) {
    char_table[n] = vpeek(CHARTABLE + n);
  }

  // Set graphic mode
  color(WHITE, BLACK, BLACK);
  screen(7);
  SetFasterVDP();	// optimize VDP, sprites off
  SetPage(0);		// Set the main page 0
  SetBorderColor(0);	// Background + border

  // Set base colors
  fgcolor = WHITE;
  bgcolor = BLACK;
  inverted = 0;

  // Put all chars in backbufffer
  y = -1;
  x = 0;
  for (n = 0; n < 255; n++) {
    if (n%BACKBUFFER_LENGTH == 0) {
      y++;
      x=0;
    }
    vputchar_vram(n, SCREEN7_BACKBUF + x*CHAR_SIZEX + y*SCREEN7_SIZEX/2*CHAR_SIZEY);
    x++;
  }

  // Set coords to 0,0
  gotoxy(0, 0);


  printf("\33m%cM\33m%cS\33m%cX!\r\n", LIGHT_RED+32 , LIGHT_GREEN+32, LIGHT_BLUE+32);
  gotoxy(0,10);
  printf("\33m%cC\33m%cO\33m%cL\33m%cO\33m%cR\33m%cS\33m%c! \33m%cC\33m%cO\33m%cL\33m%cO\33m%cR\33m%cS\33m%c!\r\n", MEDIUM_GREEN+32, LIGHT_GREEN+32, DARK_BLUE+32, LIGHT_BLUE+32, DARK_RED+32, CYAN+32, MEDIUM_RED+32, LIGHT_RED+32, DARK_YELLOW+32, LIGHT_YELLOW+32, DARK_GREEN+32, MAGENTA+32, GRAY+32, WHITE+32);
  gotoxy(0,15);
  printf("\33m%c\33m0Look Ma, inverted text!", WHITE+32);
  getchar();


  initxt(80);

  return 0;
}

/*** main }}} ***/
