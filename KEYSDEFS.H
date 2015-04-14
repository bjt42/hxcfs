  /* SCAN.H - Constants for keyboard scan codes. They can be
   * processed by:
   *	AH returned by BIOS function 16h, services 00h, 01h, 10h, 11h
   *	High byte returned by DOS _bios_keybrd( _KEYBRD_READY );
   *				  _bios_keybrd( _NKEYBRD_READY );
   *				  _bios_keybrd( _KEYBRD_READ );
   *				  _bios_keybrd( _NKEYBRD_READ );
   *	OS/2 KBDKEYINFO.fsScan (set by KbdCharIn or KbdPeek)
   */

  enum SCAN
  {

      /* Scan numbers follow keyboard layout of original PC. */

      /* Top row of keys */
      scanESC = 0x01,
      scan1, scan2, scan3, scan4, scan5, scan6, scan7, scan8, scan9, scan0,
      scanMINUS, scanEQUAL, scanBKSP,

      /* Second row of keys */
      scanTAB = 0x0F,
      scanQ, scanW, scanE, scanR, scanT, scanY, scanU, scanI, scanO, scanP,
      scanLBRACKET, scanRBRACKET, scanENTER,

      /* Third row of keys */
      scanCTRL = 0x1D,
      scanA, scanS, scanD, scanF, scanG, scanH, scanJ, scanK,
      scanSEMICOLON, scanFQUOTE, scanBQUOTE,

      /* Fourth row of keys */
      scanLSHIFT = 0x2A, scanBACKSLASH,
      scanZ, scanX, scanC, scanV, scanB, scanN, scanM,
      scanCOMMA, scanPERIOD, scanSLASH, scanRSHIFT, scanPRTSC,

      /* Fifth row of keys */
      scanALT = 0x38, scanSPACE, scanCAPS,

      /* Function keys */
      scanF1 = 0x3B, scanF2, scanF3, scanF4, scanF5,
      scanF6,	     scanF7, scanF8, scanF9, scanF10,

      /* Numeric keypad */
      scanNUM = 0x45,  scanSCROLL,
      scanHOME, scanUP,     scanPGUP,  scanGMINUS,
      scanLEFT, scanCENTER, scanRIGHT, scanGPLUS,
      scanEND,	scanDOWN,   scanPGDN,
      scanINS,	scanDEL,

      /* 84-key and 101-key keyboards only */
      scanSYSREQ = 0x54,

      /* 101-key keyboard only */
      scanF11 = 0x57, scanF12,

      /* Introductory values to test whether scan is for function key */
      scanINTRO  = 0,	      // For keys common to all keyboards
      scanINTRO2 = 0xE0       // For keypad and other keys unique to
			      //   101-key keyboard
  };

/*
//
// Copyright (C) 2009-2014 Jean-François DEL NERO
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
typedef struct keyboard_funct_mapper_
{
	unsigned char function_code;
	unsigned char keyboard_code;
} keyboard_funct_mapper;

keyboard_funct_mapper keysmap[]=
{
	{FCT_UP_KEY,scanUP},
	{FCT_DOWN_KEY,scanDOWN},
	{FCT_LEFT_KEY,scanLEFT},
	{FCT_RIGHT_KEY,scanRIGHT},
	{FCT_SELECT_FILE_DRIVEA,scanA},
	{FCT_SELECT_FILE_DRIVEB,scanB},
	{FCT_NEXTSLOT,scanPGUP},
	{FCT_PREVSLOT,scanPGDN},
	{FCT_REBOOT,scanESC},
	{FCT_CLEARSLOT,scanDEL},
	{FCT_HELP,scanF1},
	{FCT_SAVEREBOOT,scanF10},
	{FCT_OK,scanSPACE},
	{FCT_SHOWSLOTS,scanTAB},
	{FCT_SEARCH,scanF3},
	{FCT_TOP,scanHOME},
	{FCT_EMUCFG,scanF8},
	{FCT_NO_FUNCTION,0x00}
};
