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

#include <bios.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

#include "keysfunc.h"
#include "keysdefs.h"
#include "pc_hw.h"
#include "guiutils.h"
#include "hxcfeda.h"
#include "crc.h"

static unsigned char currenttrack;
static unsigned char driveid;

extern void lockup(void);
extern unsigned char sector[512];
extern void sleep(int duration);

int jumptotrack(unsigned char t)
{
	direct_access_cmd_sector* dacs;
	direct_access_status_sector* dass;

	if(0==t)
	{
		dass=(direct_access_status_sector*)sector;
		dacs=(direct_access_cmd_sector*)sector;

		memset(&sector,0,512);

		sprintf(dacs->DAHEADERSIGNATURE,"HxCFEDA");
		dacs->cmd_code=2;

		if(!writesector(0,(unsigned char*)&sector))
		{
			 hxc_printf_box(0,"jumptotrack: Write error");
			 lockup();
		}
	}

	currenttrack=t;
	return 1;
}

unsigned char writesector(unsigned char sectornum,unsigned char* data)
{
	struct diskinfo_t diskInfo;

	unsigned char retries=0;
	unsigned char status;

	diskInfo.drive=driveid;
	diskInfo.head=0;
	diskInfo.track=currenttrack;
	diskInfo.sector=sectornum;
	diskInfo.nsectors=1;
	diskInfo.buffer=data;

	while((status=(_bios_disk(_DISK_WRITE,&diskInfo)>>8))&&(retries<2))
	{
		_bios_disk(_DISK_RESET,&diskInfo);
		++retries;
	}

	return status==0;
}

unsigned char readsector(unsigned char sectornum,unsigned char * data)
{
	struct diskinfo_t diskInfo;

	unsigned char retries=0;
	unsigned char status;

	diskInfo.drive=driveid;
	diskInfo.head=0;
	diskInfo.track=currenttrack;
	diskInfo.sector=sectornum;
	diskInfo.nsectors=1;
	diskInfo.buffer=data;

	while((status=(_bios_disk(_DISK_READ,&diskInfo)>>8))&&(retries<2))
	{
	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- readsector: retry --");
	#endif
		_bios_disk(_DISK_RESET,&diskInfo);
		++retries;
	}

	return status==0;
}

void init_pc_fdc(unsigned char id)
{
	driveid=id;
	jumptotrack(255);
}

unsigned char get_char(void)
{
	unsigned int thechar=0;

	while(!(thechar&0xff))
	{
	    thechar=_bios_keybrd(_KEYBRD_READ);
	}

	return thechar&0xff;
}

unsigned char wait_function_key(void)
{
	unsigned char key,i,key_code;
	unsigned char function_code=FCT_NO_FUNCTION;

	while(function_code==FCT_NO_FUNCTION)
	{
	    key=_bios_keybrd(_NKEYBRD_READ)>>8;

	    i=0;
	    do
	    {
		function_code=keysmap[i].function_code;
		key_code=keysmap[i].keyboard_code;
		i++;

	    } while((key_code!=key)&&(function_code!=FCT_NO_FUNCTION));
	}

	return function_code;
}

void quit()
{
    sleep(1);
    restore_display();
    exit(0);
}
