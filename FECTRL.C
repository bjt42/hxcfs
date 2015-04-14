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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <graph.h>

#include "keysfunc.h"
#include "guiutils.h"
#include "cfg_file.h"
#include "hxcfeda.h"
#include "pc_hw.h"
#include "conf.h"

#include <fat_opts.h>
#include <fat_misc.h>
#include <fat_defs.h>
#include <fat_file.h>

//#define DBGMODE 1

unsigned char sector[512];

static unsigned char ticker_offset=0;
static unsigned char ticker_chars[4]={0x2D,0x5C,0x7C,0x2F};

static unsigned short y_pos;
static unsigned long last_setlbabase;
static unsigned long cluster;
static unsigned char cfgfile_header[512];

static unsigned char currentPath[4*256]={"\\"};
static unsigned char sdfecfg_file[2048];
static char filter[17];

static unsigned char slotnumber;
static char selectorpos;
static unsigned char read_entry;
static unsigned char filtermode;

static disk_in_drive disks_slot_a[NUMBER_OF_SLOT];
static disk_in_drive disks_slot_b[NUMBER_OF_SLOT];
static DirectoryEntry DirectoryEntry_tab[32];

static struct fs_dir_list_status file_list_status;
static struct fs_dir_list_status file_list_status_tab[512];
static struct fat_dir_entry sfEntry;
static struct fs_dir_ent dir_entry;
extern struct fatfs _fs;

extern unsigned short SCREEN_YRESOL;
extern unsigned char NUMBER_OF_FILE_ON_DISPLAY;

int fl_fswrite(unsigned char* buffer,int size,int start_sector,void* f);
void init_buffer(void);
unsigned char get_char(void);

void sleep(int duration)
{
	clock_t prevTime=clock();
	while((clock()-prevTime)<(CLOCKS_PER_SEC*duration));
}

void lockup()
{
	sleep(2);
	quit();
}

int setlbabase(unsigned long lba)
{
	int ret;

	direct_access_cmd_sector* dacs;
	direct_access_status_sector* dass;

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- setlbabase E --");
	#endif

	dass=(direct_access_status_sector*)sector;
	dacs=(direct_access_cmd_sector*)sector;

	memset(&sector,0,512);

	sprintf(dacs->DAHEADERSIGNATURE,"HxCFEDA");
	dacs->cmd_code=1;
	dacs->parameter_0=(lba>>0)&0xFF;
	dacs->parameter_1=(lba>>8)&0xFF;
	dacs->parameter_2=(lba>>16)&0xFF;
	dacs->parameter_3=(lba>>24)&0xFF;
	dacs->parameter_4=0xA5;

	ret=writesector(0,(unsigned char*)&sector);
	if(!ret)
	{
		hxc_printf_box(0,"setlbabase: Write error");
		lockup();
	}

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- setlbabase L --");
	#endif

	return 0;
}

int test_floppy_if()
{
	direct_access_status_sector* dass=(direct_access_status_sector*)sector;

	last_setlbabase=2;
	do
	{
		setlbabase(last_setlbabase);
		memset(sector,0,512);
		if(!readsector(0,sector))
		{
			hxc_printf_box(0,"test_floppy_if: Sector %d read error",last_setlbabase);
			lockup();
		}

		#ifdef DBGMODE
			hxc_printf(0,0,0,"%.8X=%.8X?",last_setlbabase,L_INDIAN(dass->lba_base));
		#endif

		if(last_setlbabase!=L_INDIAN(dass->lba_base))
		{
			hxc_printf_box(0,"test_floppy_if: LBA change test failed");
			lockup();
		}

		last_setlbabase--;

	} while(last_setlbabase);

	return 0;
}

int media_init()
{
	unsigned char ret;
	direct_access_status_sector* dass;

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- media_init E --");
	#endif

	last_setlbabase=0xFFFFF000;
	ret=readsector(0,(unsigned char*)&sector);

	if(ret)
	{
		dass=(direct_access_status_sector*)sector;
		if(!strcmp(dass->DAHEADERSIGNATURE,"HxCFEDA"))
		{
			_settextcolor(ALTBGCOLOR);
			_setbkcolor(ALTFGCOLOR);
			hxc_printf(0,0,SCREEN_YRESOL-8,"Firmware %s",dass->FIRMWAREVERSION);
			_settextcolor(FGCOLOR);
			_setbkcolor(BGCOLOR);

			test_floppy_if();

			dass=(direct_access_status_sector*)sector;
			last_setlbabase=0;
			setlbabase(last_setlbabase);

			#ifdef DBGMODE
				hxc_printf(0,0,0,"-- media_init L --");
			#endif

			return 1;
		}

		hxc_printf_box(0,"HxC Floppy Emulator not found!");
		lockup();

		#ifdef DBGMODE
			hxc_printf(0,0,0,"-- media_init L --");
		#endif

		return 0;
	}

	hxc_printf_box(0,"media_init: Floppy access error");
	lockup();

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- media_init L --");
	#endif

	return 0;
}

int media_read(unsigned long sector,unsigned char* buffer)
{
	direct_access_status_sector* dass=(direct_access_status_sector*)buffer;

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- media_read E --");
	#endif

	_settextcolor(ALTFGCOLOR);
	_setbkcolor(ALTBGCOLOR);
	hxc_printf(0,8*79,0,"%c",ticker_chars[0x3&ticker_offset++]);
	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);

	do
	{
		if((sector-last_setlbabase)>=8)
		{
			setlbabase(sector);
		}

		if(!readsector(0,buffer))
		{
			hxc_printf_box(0,"media_read: Sector %d read error",(sector-last_setlbabase)+1);
		}
		last_setlbabase=L_INDIAN(dass->lba_base);

	} while((sector-L_INDIAN(dass->lba_base))>=8);

	if(!readsector((unsigned char)((sector-last_setlbabase)+1),buffer))
	{
		hxc_printf_box(0,"media_read: Sector %d read error",(sector-last_setlbabase)+1);
		lockup();
	}

	_settextcolor(ALTFGCOLOR);
	_setbkcolor(ALTBGCOLOR);
	hxc_printf(0,8*79,0," ");
	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- media_read L --");
	#endif

	return 1;
}

int media_write(unsigned long sector,unsigned char* buffer)
{
	direct_access_status_sector* dass;

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- media_write E --");
	#endif

	_settextcolor(ALTFGCOLOR);
	_setbkcolor(ALTBGCOLOR);
	hxc_printf(0,8*79,0,"%c",ticker_chars[0x3&ticker_offset++]);
	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);

	if((sector-last_setlbabase)>=8)
	{
		last_setlbabase=sector;
		setlbabase(sector);
	}

	if(!writesector((unsigned char)((sector-last_setlbabase)+1),buffer))
	{
		hxc_printf_box(0,"media_write: Write error");
		lockup();
	}

	_settextcolor(ALTFGCOLOR);
	_setbkcolor(ALTBGCOLOR);
	hxc_printf(0,8*79,0," ");
	_settextcolor(FGCOLOR);
	_setbkcolor(BGCOLOR);

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- media_write L --");
	#endif

	return 1;
}

void printslotstatus(unsigned char slotnumber,disk_in_drive* disks_a,disk_in_drive* disks_b)
{
	char tmp_str[17];

	hxc_printf(0,0,SLOT_Y_POS,"Slot %.2d",slotnumber);

	hxc_printf(0,0,SLOT_Y_POS+8,"Drive A:                 ");
	if(disks_a->DirEnt.size)
	{
		memcpy(tmp_str,disks_a->DirEnt.longName,16);
		tmp_str[16]=0;
		hxc_printf(0,0,SLOT_Y_POS+8,"Drive A: %s",tmp_str);
	}

	hxc_printf(0,0,SLOT_Y_POS+16,"Drive B:                 ");
	if(disks_b->DirEnt.size)
	{
		memcpy(tmp_str,disks_b->DirEnt.longName,16);
		tmp_str[16]=0;
		hxc_printf(0,0,SLOT_Y_POS+16,"Drive B: %s",tmp_str);
	}
}

char read_cfg_file(unsigned char* sdfecfg_file)
{
	char ret;
	unsigned char number_of_slot;
	unsigned short i;
	cfgfile* cfgfile_ptr;
	FL_FILE* file;

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- read_cfg_file E --");
	#endif

	memset((void*)&disks_slot_a,0,sizeof(disk_in_drive)*NUMBER_OF_SLOT);
	memset((void*)&disks_slot_b,0,sizeof(disk_in_drive)*NUMBER_OF_SLOT);

	ret=0;
	file=fl_fopen("/HXCSDFE.CFG","r");
	if(file)
	{
		cfgfile_ptr=(cfgfile*)cfgfile_header;

		fl_fread(cfgfile_header,1,512,file);
		number_of_slot=cfgfile_ptr->number_of_slot;

		fl_fseek(file,1024,SEEK_SET);

		fl_fread(sdfecfg_file,1,512,file);
		i=1;
		do
		{
			if(!(i&3))
			{
				fl_fread(sdfecfg_file,1,512,file);
			}

			memcpy(&disks_slot_a[i],&sdfecfg_file[(i&3)*128],sizeof(disk_in_drive));
			memcpy(&disks_slot_b[i],&sdfecfg_file[((i&3)*128)+64],sizeof(disk_in_drive));

			i++;

		} while(i<number_of_slot);

		fl_fclose(file);
	}
	else
	{
		ret=1;
	}

	if(ret)
	{
		hxc_printf_box(0,"read_cfg_file: Failed to access HXCSDFE.CFG");
	}

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- read_cfg_file L --");
	#endif

	return ret;
}

char save_cfg_file(unsigned char* sdfecfg_file)
{
	unsigned char number_of_slot,slot_index;
	unsigned char i,sect_nb,ret;
	cfgfile* cfgfile_ptr;
	unsigned short floppyselectorindex;
	FL_FILE* file;

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- save_cfg_file E --");
	#endif

	ret=0;
	file=fl_fopen("/HXCSDFE.CFG","r");
	if(file)
	{
		number_of_slot=1;
		slot_index=1;
		i=1;

		floppyselectorindex=128;	// First slot offset
		memset(sdfecfg_file,0,512);	// Clear the sector
		sect_nb=2;			// Slots Sector offset

		do
		{
			if(disks_slot_a[i].DirEnt.size) 		// Valid slot found
			{
				// Copy it to the actual file sector
				memcpy(&sdfecfg_file[floppyselectorindex],&disks_slot_a[i],sizeof(disk_in_drive));
				memcpy(&sdfecfg_file[floppyselectorindex+64],&disks_slot_b[i],sizeof(disk_in_drive));

				// Next slot...
				number_of_slot++;
				floppyselectorindex=(floppyselectorindex+128)&0x1FF;

				if(!(number_of_slot&0x3))                // Need to change to the next sector
				{
					// Save the sector
					if(fl_fswrite((unsigned char*)sdfecfg_file,1,sect_nb,file)!=1)
					{
						hxc_printf_box(0,"save_cfg_file: Failed to write file");
						ret=1;
					}
					// Next sector
					sect_nb++;
					memset(sdfecfg_file,0,512);	// Clear the next sector
				}
			}

			i++;

		} while(i<NUMBER_OF_SLOT);

		if(number_of_slot&0x3)
		{
			if(fl_fswrite((unsigned char*)sdfecfg_file,1,sect_nb,file)!=1)
			{
				hxc_printf_box(0,"save_cfg_file: Failed to write file");
				ret=1;
			}
		}

		if(slot_index>=number_of_slot)
		{
			slot_index=number_of_slot-1;
		}

		fl_fseek(file,0,SEEK_SET);

		// Update the file header
		cfgfile_ptr=(cfgfile*)cfgfile_header;
		cfgfile_ptr->number_of_slot=number_of_slot;
		cfgfile_ptr->slot_index=slot_index;

		if(fl_fswrite((unsigned char*)cfgfile_header,1,0,file)!=1)
		{
			hxc_printf_box(0,"save_cfg_file: Failed to write file");
			ret=1;
		}
	}
	else
	{
		hxc_printf_box(0,"save_cfg_file: Failed to create file");
		ret=1;
	}

	// Close file
	fl_fclose(file);

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- save_cfg_file L --");
	#endif

	return ret;
}

void clear_list(unsigned char add)
{
	unsigned char y_pos,i;

	y_pos=FILELIST_Y_POS;
	for(i=0;i<NUMBER_OF_FILE_ON_DISPLAY+add;i++)
	{
		clear_line(y_pos,0);
		y_pos=y_pos+8;
	}
}

void next_slot()
{
	slotnumber++;
	if(slotnumber>(NUMBER_OF_SLOT-1)) slotnumber=1;
	printslotstatus(slotnumber,(disk_in_drive*)&disks_slot_a[slotnumber],(disk_in_drive*)&disks_slot_b[slotnumber]);
}

void prev_slot()
{
	slotnumber--;
	if(!slotnumber) slotnumber=NUMBER_OF_SLOT-1;
	printslotstatus(slotnumber,(disk_in_drive*)&disks_slot_a[slotnumber],(disk_in_drive*)&disks_slot_b[slotnumber]);
}

void displayFolder()
{
	int i;
	hxc_printf(0,SCREEN_XRESOL/2,CURDIR_Y_POS,"Current directory:");

	for(i=SCREEN_XRESOL/2;i<SCREEN_XRESOL;i=i+8) hxc_printf(0,i,CURDIR_Y_POS+8," ");

	if(strlen(currentPath)<32)
		hxc_printf(0,SCREEN_XRESOL/2,CURDIR_Y_POS+8,"%s",currentPath);
	else
        hxc_printf(0,SCREEN_XRESOL/2,CURDIR_Y_POS+8,"...%s    ",&currentPath[strlen(currentPath)-32]);
}

void displaySearch()
{
	if(filtermode)
		hxc_printf(0,SCREEN_XRESOL/2,CURDIR_Y_POS+16,"Search: [%s]",filter);
	else
		hxc_printf(0,SCREEN_XRESOL/2,CURDIR_Y_POS+16,"                            ");
}

void enter_sub_dir(disk_in_drive* disk_ptr)
{
	int currentPathLength;
	unsigned char folder[128+1];
	unsigned char c;
	int i;
	int old_index;

	old_index=strlen(currentPath);

	if((disk_ptr->DirEnt.longName[0]==(unsigned char)'.')&&(disk_ptr->DirEnt.longName[1]==(unsigned char)'.'))
	{
		currentPathLength=strlen(currentPath)-1;
		do
		{
			currentPath[currentPathLength]=0;
			currentPathLength--;
		}
		while (currentPath[currentPathLength]!=(unsigned char)'/');
	}
	else
	{
		if(disk_ptr->DirEnt.longName[0]!=(unsigned char)'.')
		{
			for(i=0;i<128;i++)
			{
				c=disk_ptr->DirEnt.longName[i];
				if((c>=(32+0))&&(c<=127))
				{
					folder[i]=c;
				}
				else
				{
					folder[i]=0;
					i=128;
				}
			}

			currentPathLength=strlen(currentPath);

			if(currentPath[currentPathLength-1]!='/')
			strcat(currentPath,"/");

			strcat(currentPath,folder);
		}
	}

	displayFolder();

	filtermode=0;
	displaySearch();

	selectorpos=0;

	if(!fl_list_opendir(currentPath,&file_list_status))
	{
		currentPath[old_index]=0;
		fl_list_opendir(currentPath,&file_list_status);
		displayFolder();
	}
	for(i=0;i<512;i++)
	{
		memcpy(&file_list_status_tab[i],&file_list_status,sizeof(struct fs_dir_list_status));
	}
 	clear_list(0);
	read_entry=1;
}

void show_all_slots(void)
{
	#define ALLSLOTS_Y_POS 8
	char tmp_str[17];
	int i;

	for(i=1;i<NUMBER_OF_SLOT;i++)
	{
		if(disks_slot_a[i].DirEnt.size||disks_slot_b[i].DirEnt.size)
		{
			memcpy(tmp_str,&disks_slot_a[i].DirEnt.longName,16);
			tmp_str[16]=0;
			hxc_printf(0,0,ALLSLOTS_Y_POS+(i*8),"Slot %.2d - A: %s",i,tmp_str);

			memcpy(tmp_str,&disks_slot_b[i].DirEnt.longName,16);
			tmp_str[16]=0;
			hxc_printf(0,40*8,ALLSLOTS_Y_POS+(i*8),"B: %s",tmp_str);
		}
		else
		{
			hxc_printf(0,0,ALLSLOTS_Y_POS+(i*8),"Slot %.2d - A:",i);
			hxc_printf(0,40*8,ALLSLOTS_Y_POS+(i*8),"B:",i);
		}
	}

	hxc_printf(1,0,ALLSLOTS_Y_POS+(NUMBER_OF_SLOT+1)*8,"--- Press SPACE to continue ---");
	while(wait_function_key()!=FCT_OK);
}

int main(int argc,char* argv[])
{
	unsigned short i,page_number;
	unsigned char key,entrytype,bootdev;
	unsigned char last_file,displayentry,c;
	disk_in_drive* disk_ptr;
	cfgfile* cfgfile_ptr;
	char tempLowerName[FATFS_MAX_LONG_FILENAME];

	init_display();

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- Init display Done --");
	#endif

	bootdev=0;

	if(argc>1)
	{
		bootdev=(*argv[1])-'0';
		if(bootdev>3)
		{
			bootdev=0;
		}
	}
	init_pc_fdc(bootdev);

	#ifdef DBGMODE
		hxc_printf(0,0,0,"-- init_pc_fdc Done --");
	#endif

	if(media_init())
	{
		#ifdef DBGMODE
			hxc_printf(0,0,0,"-- media_init done --");
		#endif

		// Initialise File IO Library
		fl_init();

		#ifdef DBGMODE
			hxc_printf(0,0,0,"-- fl_init done --");
		#endif

		/* Attach media access functions to library */
		if(fl_attach_media(media_read,media_write)!=FAT_INIT_OK)
		{
			hxc_printf_box(0,"main: Media attach error");
			lockup();
		}

		#ifdef DBGMODE
			hxc_printf(0,0,0,"-- fl_attach_media done --");
		#endif

		hxc_printf_box(0,"Reading HXCSDFE.CFG...");

		read_cfg_file(sdfecfg_file);

		#ifdef DBGMODE
			hxc_printf(0,0,0,"-- read_cfg_file done --");
		#endif

		strcpy(currentPath,"/");
		displayFolder();

		slotnumber=1;
		printslotstatus(slotnumber,(disk_in_drive*)&disks_slot_a[slotnumber],(disk_in_drive*)&disks_slot_b[slotnumber]);

		read_entry=0;
		selectorpos=0;
		cluster=fatfs_get_root_cluster(&_fs);
		page_number=0;
		last_file=0;

		filtermode=0;
		displaySearch();

		fl_list_opendir(currentPath,&file_list_status);
		for(i=0;i<512;i++)
		{
			memcpy(&file_list_status_tab[i],&file_list_status,sizeof(struct fs_dir_list_status));
		}
		clear_list(0);

		for(;;)
		{
			y_pos=FILELIST_Y_POS;
			for(;;)
			{
				i=0;
				do
				{
					memset(&DirectoryEntry_tab[i],0,sizeof(DirectoryEntry));
					i++;

				} while((i<NUMBER_OF_FILE_ON_DISPLAY));

				i=0;
				y_pos=FILELIST_Y_POS;
				last_file=0x00;
				do
				{
					displayentry=0xFF;
					if(fl_list_readdir(&file_list_status,&dir_entry))
					{
						if(filtermode)
						{
							memcpy(tempLowerName,dir_entry.filename,FATFS_MAX_LONG_FILENAME);
							strlwr(tempLowerName);

							if(!strstr(tempLowerName,filter))
							{
								displayentry=0x00;
							}
						}

						if(displayentry)
						{
							if(dir_entry.is_dir)
							{
								entrytype=10;
								hxc_printf(0,0,y_pos," [%s]",dir_entry.filename);
								DirectoryEntry_tab[i].attributes=0x10;
							}
							else
							{
								entrytype=12;
								hxc_printf(0,0,y_pos," %s",dir_entry.filename);
								DirectoryEntry_tab[i].attributes=0x00;
							}

							y_pos=y_pos+8;
							dir_entry.filename[127]=0;
							sprintf(DirectoryEntry_tab[i].longName,"%s",dir_entry.filename);
							dir_entry.filename[11]=0;
							sprintf(DirectoryEntry_tab[i].name,"%s",dir_entry.filename);

							DirectoryEntry_tab[i].firstCluster=ENDIAN_32BIT(dir_entry.cluster);
							DirectoryEntry_tab[i].size=ENDIAN_32BIT(dir_entry.size);
							i++;
						}
					}
					else
					{
						last_file=0xFF;
						i=NUMBER_OF_FILE_ON_DISPLAY;
					}

				} while(i<NUMBER_OF_FILE_ON_DISPLAY);

				memcpy(&file_list_status_tab[(page_number+1)&0x1FF],&file_list_status,sizeof(struct fs_dir_list_status));

				invert_line(FILELIST_Y_POS+(selectorpos*8));

				read_entry=0;

				do
				{
					key=wait_function_key();
					if(1)
					{
						switch(key)
						{
						case FCT_UP_KEY: // UP
							invert_line(FILELIST_Y_POS+(selectorpos*8));
							hxc_printf(0,0,FILELIST_Y_POS+(selectorpos*8)," ");

							selectorpos--;
							if(selectorpos<0)
							{
								selectorpos=NUMBER_OF_FILE_ON_DISPLAY-1;
								if(page_number) page_number--;
								clear_list(0);
								read_entry=1;
								memcpy(&file_list_status,&file_list_status_tab[page_number&0x1FF],sizeof(struct fs_dir_list_status));
							}
							else
							{
								invert_line(FILELIST_Y_POS+(selectorpos*8));
							}
							break;

						case FCT_DOWN_KEY: // Down
							invert_line(FILELIST_Y_POS+(selectorpos*8));
							hxc_printf(0,0,FILELIST_Y_POS+(selectorpos*8)," ");

							selectorpos++;
							if(selectorpos>=NUMBER_OF_FILE_ON_DISPLAY)
							{
								selectorpos=0;
								clear_list(0);
								read_entry=1;
								if(!last_file) page_number++;
								memcpy(&file_list_status,&file_list_status_tab[page_number&0x1FF],sizeof(struct fs_dir_list_status));
							}
							else
							{
								invert_line(FILELIST_Y_POS+(selectorpos*8));
							}
							break;

						case FCT_RIGHT_KEY: // Right
							clear_list(0);
							read_entry=1;
							if(!last_file) page_number++;
							memcpy(&file_list_status,&file_list_status_tab[page_number&0x1FF],sizeof(struct fs_dir_list_status));
							break;

						case FCT_LEFT_KEY:
							if(page_number) page_number--;
							memcpy(&file_list_status,&file_list_status_tab[page_number&0x1FF],sizeof(struct fs_dir_list_status));
							clear_list(0);
							read_entry=1;
							break;

						case FCT_NEXTSLOT:
							next_slot();
							break;

						case FCT_PREVSLOT:
							prev_slot();
							break;

						case FCT_ENTERSUBDIR:
							disk_ptr=(disk_in_drive*)&DirectoryEntry_tab[selectorpos];

							if(disk_ptr->DirEnt.attributes&0x10)
							{
								enter_sub_dir(disk_ptr);
							}
							break;

						case FCT_SELECT_FILE_DRIVEA:
							disk_ptr=(disk_in_drive*)&DirectoryEntry_tab[selectorpos];

							if(!(disk_ptr->DirEnt.attributes&0x10))
							{
								memcpy((void*)&disks_slot_a[slotnumber],(void*)&DirectoryEntry_tab[selectorpos],sizeof(disk_in_drive));
								printslotstatus(slotnumber,(disk_in_drive*)&disks_slot_a[slotnumber],(disk_in_drive*)&disks_slot_b[slotnumber]);
							}
							break;

						case FCT_SELECT_FILE_DRIVEB:
							disk_ptr=(disk_in_drive*)&DirectoryEntry_tab[selectorpos];

							if(!(disk_ptr->DirEnt.attributes&0x10))
							{
								memcpy((void*)&disks_slot_b[slotnumber],(void*)&DirectoryEntry_tab[selectorpos],sizeof(disk_in_drive));
								printslotstatus(slotnumber,(disk_in_drive*)&disks_slot_a[slotnumber],(disk_in_drive*)&disks_slot_b[slotnumber]);
							}
							break;

						case FCT_HELP:
							clear_list(5);

							i=0;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"Cursor Keys       : Browse the contents of the SD Card");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"HOME              : Go back to the top of the directory");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"ENTER             : Change to a sub-directory");
							i+=2;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"A                 : Insert the selected file to A: for the current slot");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"B                 : Insert the selected file to B: for the current slot");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"PAGE DOWN         : Select the next slot");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"PAGE UP           : Select the previous slot");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"DEL               : Clear the current slot");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"TAB               : Show the status of all slots");
							i+=2;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"F3                : Search for files in the current folder");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"F8                : Settings Menu");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"F10               : Save & Exit");
							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"ESC               : Exit without saving");
							i+=2;
							hxc_printf(1,0,HELP_Y_POS+(i*8),"--- Press SPACE to continue ---");

							while(wait_function_key()!=FCT_OK);

							clear_list(5);
							init_buffer();
							printslotstatus(slotnumber,(disk_in_drive*)&disks_slot_a[slotnumber],(disk_in_drive*)&disks_slot_b[slotnumber]);
							displayFolder();
							displaySearch();

							memcpy(&file_list_status,&file_list_status_tab[page_number&0x1FF],sizeof(struct fs_dir_list_status));
							clear_list(0);
							read_entry=1;
							break;

						case FCT_EMUCFG:
							clear_list(5);
							cfgfile_ptr=(cfgfile*)cfgfile_header;

							i=0;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"Track step sound:");
							hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%s ",cfgfile_ptr->step_sound?"on":"off");

							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"User interface sound:");
							hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%d   ",cfgfile_ptr->buzzer_duty_cycle);

							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"LCD backlight standby:");
							hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%d s",cfgfile_ptr->back_light_tmr);

							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"SD Card standby:");
							hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%d s",cfgfile_ptr->standby_tmr);

							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"DF1 drive:");
							hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%s ",cfgfile_ptr->enable_drive_b?"off":"on");

							i++;
							hxc_printf(0,0,HELP_Y_POS+(i*8),"Load AUTOBOOT.HFE at power up:");
							hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%s ",cfgfile_ptr->startup_mode&0x04?"on":"off");

							i=i+2;
							hxc_printf(1,0,HELP_Y_POS+(i*8),"--- Press SPACE to continue ---");

							i=0;
							invert_line(HELP_Y_POS+(i*8));
							do
							{
								c=wait_function_key();
								switch(c)
								{
									case FCT_UP_KEY:
										invert_line(HELP_Y_POS+(i*8));
										if(i>0) i--;
										invert_line(HELP_Y_POS+(i*8));
									break;
									case FCT_DOWN_KEY:
										invert_line(HELP_Y_POS+(i*8));
										if(i<5) i++;
										invert_line(HELP_Y_POS+(i*8));
									break;
									case FCT_LEFT_KEY:
										invert_line(HELP_Y_POS+(i*8));
										switch(i)
										{
											case 0:
												cfgfile_ptr->step_sound=~cfgfile_ptr->step_sound;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%s ",cfgfile_ptr->step_sound?"on":"off");
											break;

											case 1:
												if(cfgfile_ptr->buzzer_duty_cycle) cfgfile_ptr->buzzer_duty_cycle--;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%d  ",cfgfile_ptr->buzzer_duty_cycle);
												if(!cfgfile_ptr->buzzer_duty_cycle) cfgfile_ptr->ihm_sound=0x00;
											break;

											case 2:
												if(cfgfile_ptr->back_light_tmr) cfgfile_ptr->back_light_tmr--;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%d s ",cfgfile_ptr->back_light_tmr);
											break;

											case 3:
												if(cfgfile_ptr->standby_tmr) cfgfile_ptr->standby_tmr--;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%d s ",cfgfile_ptr->standby_tmr);
											break;

											case 4:
												cfgfile_ptr->enable_drive_b=~cfgfile_ptr->enable_drive_b;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%s ",cfgfile_ptr->enable_drive_b?"off":"on");
											break;

											case 5:
												cfgfile_ptr->startup_mode=cfgfile_ptr->startup_mode^0x04;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%s ",(cfgfile_ptr->startup_mode&0x4)?"on":"off");
											break;
										}
										invert_line(HELP_Y_POS+(i*8));
									break;
									case FCT_RIGHT_KEY:
										invert_line(HELP_Y_POS+(i*8));
										switch(i)
										{
											case 0:
												cfgfile_ptr->step_sound=~cfgfile_ptr->step_sound;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%s ",cfgfile_ptr->step_sound?"on":"off");
												break;

											case 1:
												if(cfgfile_ptr->buzzer_duty_cycle<0x80) cfgfile_ptr->buzzer_duty_cycle++;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%d  ",cfgfile_ptr->buzzer_duty_cycle);
												cfgfile_ptr->ihm_sound=0xFF;
											break;

											case 2:
												if(cfgfile_ptr->back_light_tmr<0xFF) cfgfile_ptr->back_light_tmr++;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%d s ",cfgfile_ptr->back_light_tmr);
											break;

											case 3:
												if(cfgfile_ptr->standby_tmr<0xFF) cfgfile_ptr->standby_tmr++;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%d s ",cfgfile_ptr->standby_tmr);
											break;

											case 4:
												cfgfile_ptr->enable_drive_b=~cfgfile_ptr->enable_drive_b;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%s ",cfgfile_ptr->enable_drive_b?"off":"on");
											break;

											case 5:
												cfgfile_ptr->startup_mode=cfgfile_ptr->startup_mode^0x04;
												hxc_printf(0,SCREEN_XRESOL/2,HELP_Y_POS+(i*8),"%s ",(cfgfile_ptr->startup_mode&0x4)?"on":"off");
											break;
										}
										invert_line(HELP_Y_POS+(i*8));
									break;
								}

							} while(c!=FCT_OK);

							clear_list(5);
							init_buffer();
							printslotstatus(slotnumber,(disk_in_drive*)&disks_slot_a[slotnumber],(disk_in_drive*)&disks_slot_b[slotnumber]);
							displayFolder();
							displaySearch();

							memcpy(&file_list_status,&file_list_status_tab[page_number&0x1FF],sizeof(struct fs_dir_list_status));
							clear_list(0);
							read_entry=1;
							break;

						case FCT_SHOWSLOTS:
							clear_list(5);
							show_all_slots();
							clear_list(5);

							init_buffer();
							printslotstatus(slotnumber,(disk_in_drive*)&disks_slot_a[slotnumber],(disk_in_drive*)&disks_slot_b[slotnumber]);
							displayFolder();
							displaySearch();

							memcpy(&file_list_status,&file_list_status_tab[page_number&0x1FF],sizeof(struct fs_dir_list_status));
							clear_list(0);
							read_entry=1;
							break;

						case FCT_CLEARSLOT:
							memset((void*)&disks_slot_a[slotnumber],0,sizeof(disk_in_drive));
							memset((void*)&disks_slot_b[slotnumber],0,sizeof(disk_in_drive));
							printslotstatus(slotnumber,(disk_in_drive*)&disks_slot_a[slotnumber],(disk_in_drive*)&disks_slot_b[slotnumber]);
							break;

						case FCT_SAVEREBOOT:
							hxc_printf_box(0,"Saving selection and exiting...");
							save_cfg_file(sdfecfg_file);
							restore_box();
							jumptotrack(0);
							quit();
							break;

						case FCT_REBOOT:
							jumptotrack(0);
							quit();
							break;

						case FCT_TOP:
							page_number=0;
							memcpy(&file_list_status,&file_list_status_tab[page_number&0x1FF],sizeof(struct fs_dir_list_status));
							clear_list(0);
							read_entry=1;
							break;

						case FCT_SEARCH:
							hxc_printf(0,SCREEN_XRESOL/2,CURDIR_Y_POS+16,"Search:                     ");
							i=0;
							do
							{
								c=get_char();
								if(c=='\b')
								{
									if(i)
									{
										i--;
										hxc_printf(0,SCREEN_XRESOL/2+(8*8)+(8*i),CURDIR_Y_POS+16," ");
									}
								}
								else if((c!='\r')&&(c!=0x1B))
								{
									filter[i]=c;
									hxc_printf(0,SCREEN_XRESOL/2+(8*8)+(8*i),CURDIR_Y_POS+16,"%c",c);
									i++;
								}

							} while((c!='\r')&&(c!=0x1B)&&(i<16));

							if((c==0x1B)||(i==0)||(filter[0]==' '))
							{
								filtermode=0;
								hxc_printf(0,SCREEN_XRESOL/2,CURDIR_Y_POS+16,"                            ");
							}
							else
							{
								filter[i]=0;
								filtermode=0xFF;

								strlwr(filter);
								hxc_printf(0,SCREEN_XRESOL/2+(8*8),CURDIR_Y_POS+16,"[%s]",filter);
							}

							selectorpos=0;
							page_number=0;
							memcpy(&file_list_status,&file_list_status_tab[0],sizeof(struct fs_dir_list_status));

							clear_list(0);
							read_entry=1;
							break;

						default:
							break;
						}
					}

				} while(!read_entry);
			}
		}
	}

	restore_display();
	return 0;
}
