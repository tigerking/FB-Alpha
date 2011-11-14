// FB Alpha 4 Enraya driver module
// Based on MAME driver by Tomasz Slanina

#include "tiles_generic.h"
#include "driver.h"
extern "C" {
#include "ay8910.h"
}

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *AllRam;
static UINT8 *RamEnd;
static UINT8 *DrvZ80ROM;
static UINT8 *DrvGfxROM;
static UINT8 *DrvZ80RAM;
static UINT8 *DrvVidRAM;
static UINT32  *Palette;
static UINT32  *DrvPalette;

static INT16 *pAY8910Buffer[3];

static UINT8 DrvRecalc;

static UINT8  DrvJoy1[8];
static UINT8  DrvJoy2[8];
static UINT8  DrvDips[1];
static UINT8  DrvInputs[2];
static UINT8  DrvReset;

static UINT8 *soundlatch;
static UINT8 *soundcontrol;

static struct BurnInputInfo Enraya4InputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvJoy2 + 1,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy2 + 6,	"p1 start"	},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy1 + 5,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy1 + 6,	"p1 fire 2"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvJoy2 + 7,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvJoy2 + 5,	"p2 start"	},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy1 + 0,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy1 + 3,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy1 + 4,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy1 + 7,	"p2 fire 2"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"		},
	{"Dip A",		BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
};

STDINPUTINFO(Enraya4)

static struct BurnDIPInfo Enraya4DIPList[]=
{
	{0x0d, 0xff, 0xff, 0xfd, NULL			},

	{0   , 0xfe, 0   ,    2, "Difficulty"		},
	{0x0d, 0x01, 0x01, 0x01, "Easy"			},
	{0x0d, 0x01, 0x01, 0x00, "Hard"			},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"		},
	{0x0d, 0x01, 0x02, 0x02, "Off"			},
	{0x0d, 0x01, 0x02, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Pieces"		},
	{0x0d, 0x01, 0x04, 0x04, "30"			},
	{0x0d, 0x01, 0x04, 0x00, "16"			},

	{0   , 0xfe, 0   ,    2, "Speed"		},
	{0x0d, 0x01, 0x08, 0x08, "Slow"			},
	{0x0d, 0x01, 0x08, 0x00, "Fast"			},

	{0   , 0xfe, 0   ,    4, "Coin B"		},
	{0x0d, 0x01, 0x30, 0x30, "1 Coin  1 Credits"	},
	{0x0d, 0x01, 0x30, 0x00, "2 Coins 3 Credits"	},
	{0x0d, 0x01, 0x30, 0x10, "1 Coin  3 Credits"	},
	{0x0d, 0x01, 0x30, 0x20, "1 Coin  4 Credits"	},

	{0   , 0xfe, 0   ,    4, "Coin A"		},
	{0x0d, 0x01, 0xc0, 0x40, "2 Coins 1 Credits"	},
	{0x0d, 0x01, 0xc0, 0xc0, "1 Coin  1 Credits"	},
	{0x0d, 0x01, 0xc0, 0x00, "2 Coins 3 Credits"	},
	{0x0d, 0x01, 0xc0, 0x80, "1 Coin  2 Credits"	},
};

STDDIPINFO(Enraya4)

void __fastcall enraya4_write(UINT16 address, UINT8 data)
{
	if ((address & 0xf000) == 0xd000) {
		DrvVidRAM[((address & 0x3ff) << 1) + 0] = data;
		DrvVidRAM[((address & 0x3ff) << 1) + 1] = (address >> 10) & 3;

		return;
	}
}

static void sound_control(UINT8 data)
{
	if (*soundcontrol & 0x04 && ~data & 0x04) {
		AY8910Write(0, ~*soundcontrol & 1, *soundlatch);
	}

	*soundcontrol = data;
}

void __fastcall enraya4_out_port(UINT16 port, UINT8 data)
{
	switch (port & 0xff)
	{
		case 0x23:
			*soundlatch = data;
		break;

		case 0x33:
			sound_control(data);
		break;
	}
}

UINT8 __fastcall enraya4_in_port(UINT16 port)
{
	static UINT8 nRet = 0; 

	switch (port & 0xff)
	{
		case 0x00:
			return DrvDips[0];

		case 0x01:
			return DrvInputs[0];
	
		case 0x02:
			return DrvInputs[1];
	}

	return nRet;
}

static INT32 DrvDoReset()
{
	DrvReset = 0;
	memset (AllRam, 0, RamEnd - AllRam);

	ZetOpen(0);
	ZetReset();
	ZetClose();

	AY8910Reset(0);

	return 0;
}

static INT32 MemIndex()
{
	UINT8 *Next; Next = AllMem;

	DrvZ80ROM		= Next; Next += 0x00c000;

	DrvGfxROM		= Next; Next += 0x010000;

	Palette			= (UINT32*)Next; Next += 0x0008 * sizeof(UINT32);
	DrvPalette		= (UINT32*)Next; Next += 0x0008 * sizeof(UINT32);

	AllRam			= Next;

	DrvZ80RAM		= Next; Next += 0x001000;
	DrvVidRAM		= Next; Next += 0x000800;

	soundlatch		= Next; Next += 0x000001;
	soundcontrol		= Next; Next += 0x000001;

	pAY8910Buffer[0]	= (INT16*)Next; Next += nBurnSoundLen * sizeof(INT16);
	pAY8910Buffer[1]	= (INT16*)Next; Next += nBurnSoundLen * sizeof(INT16);
	pAY8910Buffer[2]	= (INT16*)Next; Next += nBurnSoundLen * sizeof(INT16);

	RamEnd			= Next;
	MemEnd			= Next;

	return 0;
}

static void DrvPaletteInit()
{
	for (INT32 i = 0; i < 8; i++) {
		Palette[i]  = (i & 1) ? 0xff0000 : 0;
		Palette[i] |= (i & 2) ? 0x00ff00 : 0;
		Palette[i] |= (i & 4) ? 0x0000ff : 0;
	}
}

static INT32 DrvGfxDecode()
{
	INT32 Plane[3] = { 0x10000, 0x20000, 0x00000 };
	INT32 XOffs[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	INT32 YOffs[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

	UINT8 *tmp = (UINT8*)malloc(0x6000);
	if (tmp == NULL) {
		return 1;
	}

	memcpy (tmp, DrvGfxROM, 0x6000);

	GfxDecode(0x0400, 3, 8, 8, Plane, XOffs, YOffs, 0x040, tmp, DrvGfxROM);

	if (tmp) {
		free (tmp);
		tmp = NULL;
	}

	return 0;
}

static INT32 DrvInit()
{
	AllMem = NULL;
	MemIndex();
	INT32 nLen = MemEnd - (UINT8 *)0;
	if ((AllMem = (UINT8 *)malloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	{
		BurnLoadRom(DrvZ80ROM + 0x00000, 0, 1);
		BurnLoadRom(DrvZ80ROM + 0x08000, 1, 1);

		BurnLoadRom(DrvGfxROM + 0x00000, 2, 1);
		BurnLoadRom(DrvGfxROM + 0x02000, 3, 1);
		BurnLoadRom(DrvGfxROM + 0x04000, 4, 1);

		DrvPaletteInit();
		DrvGfxDecode();
	}

	ZetInit(1);
	ZetOpen(0);
	ZetMapArea(0x0000, 0xbfff, 0, DrvZ80ROM);
	ZetMapArea(0x0000, 0xbfff, 2, DrvZ80ROM);
	ZetMapArea(0xc000, 0xcfff, 0, DrvZ80RAM);
	ZetMapArea(0xc000, 0xcfff, 1, DrvZ80RAM);
	ZetMapArea(0xc000, 0xcfff, 2, DrvZ80RAM);
//	ZetMapArea(0xd000, 0xdfff, 1, DrvVidRAM);
	ZetSetOutHandler(enraya4_out_port);
	ZetSetInHandler(enraya4_in_port);
	ZetSetWriteHandler(enraya4_write);
	ZetMemEnd();
	ZetClose();

	AY8910Init(0, 2000000, nBurnSoundRate, NULL, NULL, NULL, NULL);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}

static INT32 DrvExit()
{
	GenericTilesExit();
	ZetExit();
	AY8910Exit(0);

	if (AllMem) {
		free (AllMem);
		AllMem = NULL;
	}

	return 0;
}

static INT32 DrvDraw()
{
	if (DrvRecalc) {
		for (INT32 i = 0; i < 8; i++) {
			INT32 d = Palette[i];
			DrvPalette[i] = BurnHighCol(d >> 16, (d >> 8) & 0xff, d & 0xff, 0);
		}
	}

	for (INT32 offs = 0x40; offs < 0x3c0; offs++) {
		INT32 sx = (offs & 0x1f) << 3;
		INT32 sy = (offs >> 5) << 3;

		INT32 code = DrvVidRAM[(offs << 1) + 0] | (DrvVidRAM[(offs << 1) + 1] << 8);

		Render8x8Tile(pTransDraw, code, sx, sy - 16, 0, 0, 0, DrvGfxROM);
	}

	BurnTransferCopy(DrvPalette);

	return 0;
}


static INT32 DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	{
		memset (DrvInputs, 0xff, 2);
		for (INT32 i = 0; i < 8; i++) {
			DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
			DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
		}
	}

	INT32 nInterleave = 4;
	INT32 nCyclesTotal = 4000000 / 60;
	INT32 nCyclesDone  = 0;

	ZetOpen(0);

	for (INT32 i = 0; i < nInterleave; i++)
	{
		INT32 nSegment = nCyclesTotal / nInterleave;

		nCyclesDone += ZetRun(nSegment);

		ZetSetIRQLine(0, ZET_IRQSTATUS_AUTO);
	}

	ZetClose();

	if (pBurnSoundOut) {
		INT32 nSample;
		AY8910Update(0, &pAY8910Buffer[0], nBurnSoundLen);
		for (INT32 n = 0; n < nBurnSoundLen; n++) {
			nSample  = pAY8910Buffer[0][n];
			nSample += pAY8910Buffer[1][n];
			nSample += pAY8910Buffer[2][n];

			nSample /= 4;

			if (nSample < -32768) {
				nSample = -32768;
			} else {
				if (nSample > 32767) {
					nSample = 32767;
				}
			}

			pBurnSoundOut[(n << 1) + 0] = nSample;
			pBurnSoundOut[(n << 1) + 1] = nSample;
		}
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static INT32 DrvScan(INT32 nAction,INT32 *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin = 0x029702;
	}

	if (nAction & ACB_VOLATILE) {		
		memset(&ba, 0, sizeof(ba));

		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd - AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);

		ZetScan(nAction);
		AY8910Scan(nAction, pnMin);
	}

	return 0;
}


// 4 En Raya

static struct BurnRomInfo enraya4RomDesc[] = {
	{ "5.bin",   0x8000, 0xcf1cd151, BRF_ESS | BRF_PRG }, //  0 Z80 Code
	{ "4.bin",   0x4000, 0xf9ec1be7, BRF_ESS | BRF_PRG }, //  1

	{ "1.bin",   0x2000, 0x87f92552, BRF_GRA },	      //  2 Graphics
	{ "2.bin",   0x2000, 0x2b0a3793, BRF_GRA },	      //  3
	{ "3.bin",   0x2000, 0xf6940836, BRF_GRA },	      //  4

	{ "1.bpr",   0x0020, 0xdcbd2352, BRF_GRA },	      //  5 Prom - not used
};

STD_ROM_PICK(enraya4)
STD_ROM_FN(enraya4)

struct BurnDriver BurnDrvEnraya4 = {
	"4enraya", NULL, NULL, NULL, "1990",
	"4 En Raya\0", NULL, "IDSA", "misc",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, enraya4RomInfo, enraya4RomName, NULL, NULL, Enraya4InputInfo, Enraya4DIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x8,
	256, 224, 4, 3
};

