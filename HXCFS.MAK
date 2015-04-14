PROJ = HXCFS
PROJFILE = HXCFS.MAK
DEBUG = 0

PWBRMAKE  = pwbrmake
NMAKEBSC1  = set
NMAKEBSC2  = nmake
CC  = cl
CFLAGS_G  = /W2 /BATCH /I FAT32
CFLAGS_D  = /Od /Zi
CFLAGS_R  = /Ot /Ol /Og /Oe /Gs
MAPFILE_D  = NUL
MAPFILE_R  = NUL
LFLAGS_G  =  /NOI /STACK:4096  /BATCH
LFLAGS_D  = /CO /FAR /PACKC
LFLAGS_R  =  /EXE /FAR /PACKC
LINKER	= link
ILINK  = ilink
LRF  = echo > NUL
LLIBS_G  = GRAPHICS.LIB

OBJS  = FECTRL.obj GUIUTILS.obj PC_HW.obj CRC.obj FAT32\FAT_ACC.obj\
	FAT32\FAT_FILE.obj FAT32\FAT_MISC.obj FAT32\FAT_STR.obj\
	FAT32\FATTABLE.obj FAT32\FATWRITE.obj FAT32\FATCACHE.obj

all: $(PROJ).exe

.SUFFIXES:
.SUFFIXES: .obj .c

FECTRL.obj : FECTRL.C keysfunc.h guiutils.h cfg_file.h hxcfeda.h pc_hw.h\
	conf.h

GUIUTILS.obj : GUIUTILS.C guiutils.h conf.h pc_hw.h

PC_HW.obj : PC_HW.C keysfunc.h keysdefs.h pc_hw.h crc.h

CRC.obj : CRC.C crc.h

FAT32\FAT_ACC.obj : FAT32\FAT_ACC.C FAT32\fat_defs.h FAT32\fat_acc.h\
	FAT32\fattable.h FAT32\fatwrite.h FAT32\fat_str.h FAT32\fat_misc.h\
	FAT32\fattypes.h FAT32\fat_opts.h

FAT32\FAT_FILE.obj : FAT32\FAT_FILE.C FAT32\fat_defs.h FAT32\fat_acc.h\
	FAT32\fattable.h FAT32\fatwrite.h FAT32\fat_misc.h FAT32\fat_str.h\
	FAT32\fat_file.h FAT32\fatcache.h FAT32\fattypes.h FAT32\fat_opts.h

FAT32\FAT_MISC.obj : FAT32\FAT_MISC.C FAT32\fat_misc.h FAT32\fat_defs.h\
	FAT32\fat_opts.h FAT32\fattypes.h

FAT32\FAT_STR.obj : FAT32\FAT_STR.C FAT32\fat_str.h

FAT32\FATTABLE.obj : FAT32\FATTABLE.C FAT32\fat_defs.h FAT32\fat_acc.h\
	FAT32\fattable.h FAT32\fattypes.h FAT32\fat_opts.h FAT32\fat_misc.h

FAT32\FATWRITE.obj : FAT32\FATWRITE.C FAT32\fat_defs.h FAT32\fat_acc.h\
	FAT32\fattable.h FAT32\fatwrite.h FAT32\fat_str.h FAT32\fat_misc.h\
	FAT32\fattypes.h FAT32\fat_opts.h

FAT32\FATCACHE.obj : FAT32\FATCACHE.C FAT32\fatcache.h FAT32\fat_file.h\
	FAT32\fat_opts.h FAT32\fat_acc.h FAT32\fat_defs.h FAT32\fattypes.h


$(PROJ).bsc : 

$(PROJ).exe : $(OBJS)
!IF $(DEBUG)
	$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_D)
$(LLIBS_G: = +^
) +
$(LLIBS_D: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_D);
<<
!ELSE
	$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_R)
$(LLIBS_G: = +^
) +
$(LLIBS_R: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_R);
<<
!ENDIF
	$(LINKER) @$(PROJ).lrf


.c.obj :
!IF $(DEBUG)
	$(CC) /c $(CFLAGS_G) $(CFLAGS_D) /Fo$@ $<
!ELSE
	$(CC) /c $(CFLAGS_G) $(CFLAGS_R) /Fo$@ $<
!ENDIF


run: $(PROJ).exe
	$(PROJ).exe $(RUNFLAGS)

debug: $(PROJ).exe
	CV $(CVFLAGS) $(PROJ).exe $(RUNFLAGS)
