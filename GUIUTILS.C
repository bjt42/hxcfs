/*
//
// Copyright (C) 2009-2014 Jean-François DEL NERO
// Copyright (C) 2015 bjt
//
// This file is part of the HxCFloppyEmulator file selector.
//
// HxCFloppyEmulator file selector may be used and distributed without restriction
// provided that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator file selector is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator file selector is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator file selector; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <graph.h>

#include "guiutils.h"
#include "conf.h"
#include "pc_hw.h"

static unsigned char screen_buffer[80*25];
static unsigned char screen_buffer_backup[80*3];
static unsigned char screen_line_attr[25];

static unsigned char* screen_buffer_aligned=screen_buffer;
static unsigned char* screen_buffer_backup_aligned=screen_buffer_backup;

unsigned short SCREEN_YRESOL;
unsigned char NUMBER_OF_FILE_ON_DISPLAY;

unsigned char BGCOLOR=1;
unsigned char FGCOLOR=15;
unsigned char ALTBGCOLOR=7;
unsigned char ALTFGCOLOR=0;

#define bmaptype void*
#define bitmap_font_bmp NULL
#define bitmap_font8x8_bmp NULL

void print_char(unsigned char* membuffer,bmaptype* font,unsigned short x,unsigned short y,unsigned char c)
{
	unsigned char x_pos=x/8;
	unsigned char y_pos=y/8;

	membuffer[(80*y_pos)+x_pos]=c;

	_settextposition(1+y_pos,1+x_pos);
	_outmem(&c,1);
}

void print_char8x8(unsigned char* membuffer,bmaptype* font,unsigned short x,unsigned short y,unsigned char c)
{
	print_char(membuffer,font,x,y,c);
}

void print_str(unsigned char* membuffer,char* buf,unsigned short x_pos,unsigned short y_pos,unsigned char font)
{
	unsigned char x=x_pos/8;
	unsigned char y=y_pos/8;
	unsigned int len=strlen(buf);

	memcpy(&membuffer[(80*y)+x],buf,len);

	_settextposition(1+y,1+x);
	_outmem(buf,len);
}

int hxc_printf(unsigned char mode,unsigned short x_pos,unsigned short y_pos,char* chaine,...)
{
	char temp_buffer[1024];

	va_list marker;
	va_start(marker,chaine);

	vsprintf(temp_buffer,chaine,marker);
	switch(mode)
	{
	case 0:
	print_str(screen_buffer_aligned,temp_buffer,x_pos,y_pos,8);
        break;
        case 1:
        print_str(screen_buffer_aligned,temp_buffer,(SCREEN_XRESOL-(strlen(temp_buffer)*8))/2,y_pos,8);
        break;
        case 2:
        print_str(screen_buffer_aligned,temp_buffer,(SCREEN_XRESOL-(strlen(temp_buffer)*8)),y_pos,8);
        break;
        case 4:
        print_str(screen_buffer_aligned,temp_buffer,x_pos,y_pos,16);
        break;
        case 5:
        print_str(screen_buffer_aligned,temp_buffer,(SCREEN_XRESOL-(strlen(temp_buffer)*16))/2,y_pos,16);
        break;
        case 6:
        print_str(screen_buffer_aligned,temp_buffer,(SCREEN_XRESOL-(strlen(temp_buffer)*16)),y_pos,16);
        break;
	}

	va_end(marker);
	return 0;
}

void clear_line(unsigned short y_pos,unsigned short val)
{
	unsigned char szLine[81];

	memset(szLine,val?val:0x20,80);
	szLine[80]='\0';

	print_str(screen_buffer_aligned,szLine,0,y_pos,8);
	screen_line_attr[y_pos/8]&=~1;
}

void h_line(unsigned short y_pos,unsigned short val)
{
	clear_line(y_pos,(val==0xFFFF)?0xC4:val);
}

void invert_line(unsigned short y_pos)
{
	unsigned char y=y_pos/8;
	unsigned char inverted;

	screen_line_attr[y]^=1;
	inverted=screen_line_attr[y]&0x1;

	_settextcolor(inverted?ALTFGCOLOR:FGCOLOR);
	_setbkcolor(inverted?FGCOLOR:BGCOLOR);

	_settextposition(1+y,1);
	_outmem(&screen_buffer_aligned[80*y],80);

	if(inverted)
	{
	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);
	}
}

void restore_box()
{
	int i;

	memcpy(&screen_buffer_aligned[80*9],screen_buffer_backup_aligned,80*3);

	for(i=9;i<12;++i)
	{
	_settextposition(1+i,1);
	_outmem(&screen_buffer_aligned[80*i],80);
	invert_line(8*i);
	invert_line(8*i);
	}
}

int hxc_printf_box(unsigned char mode,char * chaine,...)
{
	char temp_buffer[1024];
	int str_size;
	unsigned short i;
	va_list marker;

	memcpy(screen_buffer_backup_aligned,&screen_buffer_aligned[80*9],80*3);

	va_start(marker,chaine);

	vsprintf(temp_buffer,chaine,marker);

	str_size=strlen(temp_buffer) * 8;
	str_size=str_size+(4*8);

	_settextcolor(ALTFGCOLOR);
	_setbkcolor(ALTBGCOLOR);

	for(i=0;i<str_size;i=i+8)
	{
	print_char8x8(screen_buffer_aligned,bitmap_font8x8_bmp,((SCREEN_XRESOL-str_size)/2)+i,80-8,196);
	}
	print_char8x8(screen_buffer_aligned,bitmap_font8x8_bmp,((SCREEN_XRESOL-str_size)/2)+(i-8),80-8,191);
	print_char8x8(screen_buffer_aligned,bitmap_font8x8_bmp,((SCREEN_XRESOL-str_size)/2),80-8,218);

	for(i=0;i<str_size;i=i+8)
	{
        print_char8x8(screen_buffer_aligned,bitmap_font8x8_bmp,((SCREEN_XRESOL-str_size)/2)+i,80,' ');
	}

	print_str(screen_buffer_aligned,temp_buffer,((SCREEN_XRESOL-str_size)/2)+(2*8),80,8);
	print_char8x8(screen_buffer_aligned,bitmap_font8x8_bmp,((SCREEN_XRESOL-str_size)/2)+(i-8),80,179);
	print_char8x8(screen_buffer_aligned,bitmap_font8x8_bmp,((SCREEN_XRESOL-str_size)/2),80,179);

	for(i=0;i<str_size;i=i+8)
	{
	print_char8x8(screen_buffer_aligned,bitmap_font8x8_bmp,((SCREEN_XRESOL-str_size)/2)+i,80+8,196);
	}
	print_char8x8(screen_buffer_aligned,bitmap_font8x8_bmp,((SCREEN_XRESOL-str_size)/2)+(i-8),80+8,217);
	print_char8x8(screen_buffer_aligned,bitmap_font8x8_bmp,((SCREEN_XRESOL-str_size)/2),80+8,192);

	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);

	va_end(marker);
}

void init_buffer()
{
	int i;
	static unsigned char show_copyright=1;

	_settextcolor(ALTBGCOLOR);
	_setbkcolor(ALTFGCOLOR);
	hxc_printf(0,SCREEN_XRESOL-(8*(strlen(VERSIONCODE)+9)),SCREEN_YRESOL-(8*1),"Version %s",VERSIONCODE);
	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);

	h_line(SCREEN_YRESOL-48,0xFFFF);

	_settextcolor(ALTFGCOLOR);
	_setbkcolor(ALTBGCOLOR);
	clear_line(0,0);
	hxc_printf(1,0,0,"HxC Floppy Emulator Manager for DOS");
	clear_line(SCREEN_YRESOL-16,0);
	hxc_printf(1,0,SCREEN_YRESOL-16,">>> Press F1 for help <<<");
	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);

	if(show_copyright)
	{
	show_copyright=0;

	i=5;
	hxc_printf(1,0,HELP_Y_POS+(i*8), "HxC Floppy Emulator Manager for DOS");
	i++;
	hxc_printf(1,0,HELP_Y_POS+(i*8), "(c) 2009-2014 HxC2001 / Jean-Francois DEL NERO");
	i++;
	hxc_printf(1,0,HELP_Y_POS+(i*8), "(c) 2015 bjt");
	i++;
	hxc_printf(1,0,HELP_Y_POS+(i*8), "Check for updates at:");
	i++;
	hxc_printf(1,0,HELP_Y_POS+(i*8), "http://hxc2001.free.fr/floppy_drive_emulator/");
	i++;
	hxc_printf(1,0,HELP_Y_POS+(i*8), "Email: hxc2001@free.fr");
	i++;
	hxc_printf(1,0,HELP_Y_POS+(i*8), "v%s - %s",VERSIONCODE,DATECODE);
	}
}

int init_display()
{
	if (!_setvideomoderows(_TEXTC80,25))
	{
	if (!_setvideomoderows(_TEXTMONO,25))
	{
	    printf("init_display: _setvideomoderows failed\n");
	    exit(0);
	}
	else
	{
	    BGCOLOR=0;
	    FGCOLOR=7;
	    ALTBGCOLOR=7;
	    ALTFGCOLOR=0;
	}
	}

	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);

	_clearscreen(_GCLEARSCREEN);
	_displaycursor(_GCURSOROFF);

	SCREEN_YRESOL=200;
	NUMBER_OF_FILE_ON_DISPLAY=18;

	_settextcolor(ALTBGCOLOR);
	_setbkcolor(ALTFGCOLOR);
	clear_line(SCREEN_YRESOL-8,0);
	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);

	init_buffer();
}

void restore_display()
{
	_settextcolor(ALTBGCOLOR);
	_setbkcolor(ALTFGCOLOR);

	_displaycursor(_GCURSORON);
	_clearscreen(_GCLEARSCREEN);
}
