#include "tiles_generic.h"
#include "m6502_intf.h"
#include "msm6295.h"
#include "burn_ym2203.h"
#include "burn_ym3812.h"
#include "h6280_intf.h"

static UINT8 DrvInputPort0[8]       = {0, 0, 0, 0, 0, 0, 0, 0};
static UINT8 DrvInputPort1[8]       = {0, 0, 0, 0, 0, 0, 0, 0};
static UINT8 DrvInputPort2[8]       = {0, 0, 0, 0, 0, 0, 0, 0};
static UINT8 DrvDip[2]              = {0, 0};
static UINT8 DrvInput[3]            = {0x00, 0x00, 0x00};
static UINT8 DrvReset               = 0;

static UINT8 *Mem                   = NULL;
static UINT8 *MemEnd                = NULL;
static UINT8 *RamStart              = NULL;
static UINT8 *RamEnd                = NULL;
static UINT8 *Drv68KRom             = NULL;
static UINT8 *Drv68KRam             = NULL;
static UINT8 *DrvM6502Rom           = NULL;
static UINT8 *DrvM6502Ram           = NULL;
static UINT8 *DrvH6280Rom           = NULL;
static UINT8 *DrvH6280Ram           = NULL;
static UINT8 *DrvCharRam            = NULL;
static UINT8 *DrvCharCtrl0Ram       = NULL;
static UINT8 *DrvCharCtrl1Ram       = NULL;
static UINT8 *DrvCharColScrollRam   = NULL;
static UINT8 *DrvCharRowScrollRam   = NULL;
static UINT8 *DrvVideo1Ram          = NULL;
static UINT8 *DrvVideo1Ctrl0Ram     = NULL;
static UINT8 *DrvVideo1Ctrl1Ram     = NULL;
static UINT8 *DrvVideo1ColScrollRam = NULL;
static UINT8 *DrvVideo1RowScrollRam = NULL;
static UINT8 *DrvVideo2Ram          = NULL;
static UINT8 *DrvVideo2Ctrl0Ram     = NULL;
static UINT8 *DrvVideo2Ctrl1Ram     = NULL;
static UINT8 *DrvVideo2ColScrollRam = NULL;
static UINT8 *DrvVideo2RowScrollRam = NULL;
static UINT8 *DrvPaletteRam         = NULL;
static UINT8 *DrvPalette2Ram        = NULL;
static UINT8 *DrvSpriteRam          = NULL;
static UINT8 *DrvSpriteDMABufferRam = NULL;
static UINT8 *DrvSharedRam          = NULL;
static UINT8 *DrvChars              = NULL;
static UINT8 *DrvTiles1             = NULL;
static UINT8 *DrvTiles2             = NULL;
static UINT8 *DrvSprites            = NULL;
static UINT8 *DrvTempRom            = NULL;
static UINT32 *DrvPalette            = NULL;
static UINT16 *pCharLayerDraw       = NULL;
static UINT16 *pTile1LayerDraw      = NULL;
static UINT16 *pTile2LayerDraw      = NULL;

static INT32 i8751RetVal;
static UINT8 DrvVBlank;
static UINT8 DrvSoundLatch;
static UINT8 DrvFlipScreen;
static INT32 DrvPriority;
static INT32 DrvCharTilemapWidth;
static INT32 DrvCharTilemapHeight;
static INT32 DrvTile1TilemapWidth;
static INT32 DrvTile1TilemapHeight;
static INT32 DrvTile2TilemapWidth;
static INT32 DrvTile2TilemapHeight;

typedef void (*Dec0Render)();
static Dec0Render Dec0DrawFunction;
static void BaddudesDraw();
static void HbarrelDraw();
static void RobocopDraw();

static INT32 nCyclesDone[3], nCyclesTotal[3];

static INT32 Dec0Game = 0;

#define DEC0_GAME_BADDUDES	1
#define DEC0_GAME_HBARREL	2
#define DEC0_GAME_BIRDTRY	3

static struct BurnInputInfo Dec0InputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort2 + 4, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , DrvInputPort2 + 2, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort2 + 5, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , DrvInputPort2 + 3, "p2 start"  },

	{"Up"                , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 up"     },
	{"Down"              , BIT_DIGITAL  , DrvInputPort0 + 1, "p1 down"   },
	{"Left"              , BIT_DIGITAL  , DrvInputPort0 + 2, "p1 left"   },
	{"Right"             , BIT_DIGITAL  , DrvInputPort0 + 3, "p1 right"  },
	{"Fire 1"            , BIT_DIGITAL  , DrvInputPort0 + 4, "p1 fire 1" },
	{"Fire 2"            , BIT_DIGITAL  , DrvInputPort0 + 5, "p1 fire 2" },
	{"Fire 3"            , BIT_DIGITAL  , DrvInputPort0 + 6, "p1 fire 3" },
	{"Fire 4"            , BIT_DIGITAL  , DrvInputPort0 + 7, "p1 fire 4" },
	{"Fire 5"            , BIT_DIGITAL  , DrvInputPort2 + 0, "p1 fire 5" },
	
	{"Up (Cocktail)"     , BIT_DIGITAL  , DrvInputPort1 + 0, "p2 up"     },
	{"Down (Cocktail)"   , BIT_DIGITAL  , DrvInputPort1 + 1, "p2 down"   },
	{"Left (Cocktail)"   , BIT_DIGITAL  , DrvInputPort1 + 2, "p2 left"   },
	{"Right (Cocktail)"  , BIT_DIGITAL  , DrvInputPort1 + 3, "p2 right"  },
	{"Fire 1 (Cocktail)" , BIT_DIGITAL  , DrvInputPort1 + 4, "p2 fire 1" },
	{"Fire 2 (Cocktail)" , BIT_DIGITAL  , DrvInputPort1 + 5, "p2 fire 2" },
	{"Fire 3 (Cocktail)" , BIT_DIGITAL  , DrvInputPort1 + 6, "p2 fire 3" },
	{"Fire 4 (Cocktail)" , BIT_DIGITAL  , DrvInputPort1 + 7, "p2 fire 4" },
	{"Fire 5 (Cocktail)" , BIT_DIGITAL  , DrvInputPort2 + 1, "p2 fire 5" },

	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , DrvInputPort2 + 6, "service"   },
	{"Dip 1"             , BIT_DIPSWITCH, DrvDip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, DrvDip + 1       , "dip"       },
};

STDINPUTINFO(Dec0)

static inline void DrvClearOpposites(UINT8* nJoystickInputs)
{
	if ((*nJoystickInputs & 0x03) == 0x03) {
		*nJoystickInputs &= ~0x03;
	}
	if ((*nJoystickInputs & 0x0c) == 0x0c) {
		*nJoystickInputs &= ~0x0c;
	}
}

static inline void DrvMakeInputs()
{
	DrvInput[0] = DrvInput[1] = DrvInput[2] = 0x00;

	for (INT32 i = 0; i < 8; i++) {
		DrvInput[0] |= (DrvInputPort0[i] & 1) << i;
		DrvInput[1] |= (DrvInputPort1[i] & 1) << i;
		DrvInput[2] |= (DrvInputPort2[i] & 1) << i;
	}

	DrvClearOpposites(&DrvInput[0]);
	DrvClearOpposites(&DrvInput[1]);
}

static struct BurnDIPInfo BaddudesDIPList[]=
{
	// Default Values
	{0x18, 0xff, 0xff, 0xff, NULL                     },
	{0x19, 0xff, 0xff, 0xff, NULL                     },

	// Dip 1
	{0   , 0xfe, 0   , 4   , "Coin A"                 },
	{0x18, 0x01, 0x03, 0x00, "3 Coins 1 Play"         },
	{0x18, 0x01, 0x03, 0x01, "2 Coins 1 Play"         },
	{0x18, 0x01, 0x03, 0x03, "1 Coin  1 Play"         },
	{0x18, 0x01, 0x03, 0x02, "1 Coin  2 Plays"        },
	
	{0   , 0xfe, 0   , 4   , "Coin B"                 },
	{0x18, 0x01, 0x0c, 0x00, "3 Coins 1 Play"         },
	{0x18, 0x01, 0x0c, 0x04, "2 Coins 1 Play"         },
	{0x18, 0x01, 0x0c, 0x0c, "1 Coin  1 Play"         },
	{0x18, 0x01, 0x0c, 0x08, "1 Coin  2 Plays"        },
	
	{0   , 0xfe, 0   , 2   , "Service Mode"           },
	{0x18, 0x01, 0x10, 0x10, "Off"                    },
	{0x18, 0x01, 0x10, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x18, 0x01, 0x20, 0x00, "Off"                    },
	{0x18, 0x01, 0x20, 0x20, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x18, 0x01, 0x40, 0x40, "Off"                    },
	{0x18, 0x01, 0x40, 0x00, "On"                     },
	
	// Dip 2
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x19, 0x01, 0x03, 0x01, "1"                      },
	{0x19, 0x01, 0x03, 0x03, "3"                      },
	{0x19, 0x01, 0x03, 0x02, "5"                      },
	{0x19, 0x01, 0x03, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x19, 0x01, 0x0c, 0x08, "Easy"                   },
	{0x19, 0x01, 0x0c, 0x0c, "Normal"                 },
	{0x19, 0x01, 0x0c, 0x04, "Hard"                   },
	{0x19, 0x01, 0x0c, 0x00, "Hardest"                },
	
	{0   , 0xfe, 0   , 2   , "Allow continue"         },
	{0x19, 0x01, 0x10, 0x10, "No"                     },
	{0x19, 0x01, 0x10, 0x00, "Yes"                    },
};

STDDIPINFO(Baddudes)

static struct BurnDIPInfo HbarrelDIPList[]=
{
	// Default Values
	{0x18, 0xff, 0xff, 0xff, NULL                     },
	{0x19, 0xff, 0xff, 0xbf, NULL                     },
		
	// Dip 1
	{0   , 0xfe, 0   , 4   , "Coin A"                 },
	{0x18, 0x01, 0x03, 0x00, "2 Coins 1 Play"         },
	{0x18, 0x01, 0x03, 0x03, "1 Coin  1 Play"         },
	{0x18, 0x01, 0x03, 0x02, "1 Coin  2 Plays"        },
	{0x18, 0x01, 0x03, 0x01, "1 Coin  3 Plays"        },
	
	{0   , 0xfe, 0   , 4   , "Coin B"                 },
	{0x18, 0x01, 0x0c, 0x00, "2 Coins 1 Play"         },
	{0x18, 0x01, 0x0c, 0x0c, "1 Coin  1 Play"         },
	{0x18, 0x01, 0x0c, 0x08, "1 Coin  2 Plays"        },
	{0x18, 0x01, 0x0c, 0x04, "1 Coin  3 Plays"        },
	
	{0   , 0xfe, 0   , 2   , "Service Mode"           },
	{0x18, 0x01, 0x10, 0x10, "Off"                    },
	{0x18, 0x01, 0x10, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x18, 0x01, 0x20, 0x00, "Off"                    },
	{0x18, 0x01, 0x20, 0x20, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x18, 0x01, 0x40, 0x40, "Off"                    },
	{0x18, 0x01, 0x40, 0x00, "On"                     },
	
	// Dip 2
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x19, 0x01, 0x03, 0x01, "1"                      },
	{0x19, 0x01, 0x03, 0x03, "3"                      },
	{0x19, 0x01, 0x03, 0x02, "5"                      },
	{0x19, 0x01, 0x03, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x19, 0x01, 0x0c, 0x08, "Easy"                   },
	{0x19, 0x01, 0x0c, 0x0c, "Normal"                 },
	{0x19, 0x01, 0x0c, 0x04, "Hard"                   },
	{0x19, 0x01, 0x0c, 0x00, "Hardest"                },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x19, 0x01, 0x30, 0x30, "30k   80k 160k"         },
	{0x19, 0x01, 0x30, 0x20, "50k  120k 190k"         },
	{0x19, 0x01, 0x30, 0x10, "100k 200k 300k"         },
	{0x19, 0x01, 0x30, 0x00, "150k 300k 450k"         },
	
	{0   , 0xfe, 0   , 2   , "Allow continue"         },
	{0x19, 0x01, 0x40, 0x40, "No"                     },
	{0x19, 0x01, 0x40, 0x00, "Yes"                    },
};

STDDIPINFO(Hbarrel)

static struct BurnDIPInfo HippodrmDIPList[]=
{
	// Default Values
	{0x18, 0xff, 0xff, 0xff, NULL                     },
	{0x19, 0xff, 0xff, 0xff, NULL                     },
	
	// Dip 1
	{0   , 0xfe, 0   , 4   , "Coin A"                 },
	{0x18, 0x01, 0x03, 0x00, "3 Coins 1 Play"         },
	{0x18, 0x01, 0x03, 0x01, "2 Coins 1 Play"         },
	{0x18, 0x01, 0x03, 0x03, "1 Coin  1 Play"         },
	{0x18, 0x01, 0x03, 0x02, "1 Coin  2 Plays"        },
	
	{0   , 0xfe, 0   , 4   , "Coin B"                 },
	{0x18, 0x01, 0x0c, 0x00, "3 Coins 1 Play"         },
	{0x18, 0x01, 0x0c, 0x04, "2 Coins 1 Play"         },
	{0x18, 0x01, 0x0c, 0x0c, "1 Coin  1 Play"         },
	{0x18, 0x01, 0x0c, 0x08, "1 Coin  2 Plays"        },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x18, 0x01, 0x20, 0x00, "Off"                    },
	{0x18, 0x01, 0x20, 0x20, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x18, 0x01, 0x40, 0x40, "Off"                    },
	{0x18, 0x01, 0x40, 0x00, "On"                     },
	
	// Dip 2
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x19, 0x01, 0x03, 0x01, "1"                      },
	{0x19, 0x01, 0x03, 0x03, "2"                      },
	{0x19, 0x01, 0x03, 0x02, "3"                      },
	{0x19, 0x01, 0x03, 0x00, "5"                      },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x19, 0x01, 0x0c, 0x08, "Easy"                   },
	{0x19, 0x01, 0x0c, 0x0c, "Normal"                 },
	{0x19, 0x01, 0x0c, 0x04, "Hard"                   },
	{0x19, 0x01, 0x0c, 0x00, "Hardest"                },
	
	{0   , 0xfe, 0   , 4   , "Player & Enemy Energy"  },
	{0x19, 0x01, 0x30, 0x10, "Very Low"               },
	{0x19, 0x01, 0x30, 0x20, "Low"                    },
	{0x19, 0x01, 0x30, 0x30, "Medium"                 },
	{0x19, 0x01, 0x30, 0x00, "High"                   },
	
	{0   , 0xfe, 0   , 2   , "Energy Power Decrease on Continue"},
	{0x19, 0x01, 0x40, 0x40, "2 Dots"                 },
	{0x19, 0x01, 0x40, 0x00, "3 Dots"                 },
};

STDDIPINFO(Hippodrm)

static struct BurnDIPInfo RobocopDIPList[]=
{
	// Default Values
	{0x18, 0xff, 0xff, 0x7f, NULL                     },
	{0x19, 0xff, 0xff, 0xff, NULL                     },
	
	// Dip 1
	{0   , 0xfe, 0   , 4   , "Coin A"                 },
	{0x18, 0x01, 0x03, 0x00, "3 Coins 1 Play"         },
	{0x18, 0x01, 0x03, 0x01, "2 Coins 1 Play"         },
	{0x18, 0x01, 0x03, 0x03, "1 Coin  1 Play"         },
	{0x18, 0x01, 0x03, 0x02, "1 Coin  2 Plays"        },
	
	{0   , 0xfe, 0   , 4   , "Coin B"                 },
	{0x18, 0x01, 0x0c, 0x00, "3 Coins 1 Play"         },
	{0x18, 0x01, 0x0c, 0x04, "2 Coins 1 Play"         },
	{0x18, 0x01, 0x0c, 0x0c, "1 Coin  1 Play"         },
	{0x18, 0x01, 0x0c, 0x08, "1 Coin  2 Plays"        },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x18, 0x01, 0x20, 0x00, "Off"                    },
	{0x18, 0x01, 0x20, 0x20, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x18, 0x01, 0x40, 0x40, "Off"                    },
	{0x18, 0x01, 0x40, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x18, 0x01, 0x80, 0x80, "Upright"                },
	{0x18, 0x01, 0x80, 0x00, "Cocktail"               },
	
	// Dip 2
	{0   , 0xfe, 0   , 4   , "Player Energy"          },
	{0x19, 0x01, 0x03, 0x01, "Low"                    },
	{0x19, 0x01, 0x03, 0x03, "Medium"                 },
	{0x19, 0x01, 0x03, 0x02, "High"                   },
	{0x19, 0x01, 0x03, 0x00, "Very High"              },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x19, 0x01, 0x0c, 0x08, "Easy"                   },
	{0x19, 0x01, 0x0c, 0x0c, "Normal"                 },
	{0x19, 0x01, 0x0c, 0x04, "Hard"                   },
	{0x19, 0x01, 0x0c, 0x00, "Hardest"                },
	
	{0   , 0xfe, 0   , 2   , "Allow continue"         },
	{0x19, 0x01, 0x10, 0x10, "No"                     },
	{0x19, 0x01, 0x10, 0x00, "Yes"                    },
	
	{0   , 0xfe, 0   , 2   , "Bonus Stage Energy"     },
	{0x19, 0x01, 0x20, 0x00, "Low"                    },
	{0x19, 0x01, 0x20, 0x20, "High"                   },
	
	{0   , 0xfe, 0   , 2   , "Brink Time"             },
	{0x19, 0x01, 0x40, 0x40, "Normal"                 },
	{0x19, 0x01, 0x40, 0x00, "Less"                   },
};

STDDIPINFO(Robocop)

static struct BurnRomInfo BaddudesRomDesc[] = {
	{ "ei04-1.3c",          0x10000, 0x4bf158a7, BRF_ESS | BRF_PRG },	//  0	68000 Program Code
	{ "ei01-1.3a",          0x10000, 0x74f5110c, BRF_ESS | BRF_PRG },	//  1
	{ "ei06.6c",            0x10000, 0x3ff8da57, BRF_ESS | BRF_PRG },	//  2
	{ "ei03.6a",            0x10000, 0xf8f2bd94, BRF_ESS | BRF_PRG },	//  3
	
	{ "ei07.8a",            0x08000, 0x9fb1ef4b, BRF_ESS | BRF_PRG },	//  4	6502 Program 
	
	{ "ei25.15j",           0x08000, 0xbcf59a69, BRF_GRA },			//  5	Characters
	{ "ei26.16j",           0x08000, 0x9aff67b8, BRF_GRA },			//  6
	
	{ "ei18.14d",           0x10000, 0x05cfc3e5, BRF_GRA },			//  7	Tiles 1
	{ "ei20.17d",           0x10000, 0xe11e988f, BRF_GRA },			//  8
	{ "ei22.14f",           0x10000, 0xb893d880, BRF_GRA },			//  9
	{ "ei24.17f",           0x10000, 0x6f226dda, BRF_GRA },			// 10
	
	{ "ei30.9j",            0x10000, 0x982da0d1, BRF_GRA },			// 11	Tiles 2
	{ "ei28.9f",            0x10000, 0xf01ebb3b, BRF_GRA },			// 12
	
	{ "ei15.16c",           0x10000, 0xa38a7d30, BRF_GRA },			// 13	Sprites
	{ "ei16.17c",           0x08000, 0x17e42633, BRF_GRA },			// 14
	{ "ei11.16a",           0x10000, 0x3a77326c, BRF_GRA },			// 15
	{ "ei12.17a",           0x08000, 0xfea2a134, BRF_GRA },			// 16
	{ "ei13.13c",           0x10000, 0xe5ae2751, BRF_GRA },			// 17
	{ "ei14.14c",           0x08000, 0xe83c760a, BRF_GRA },			// 18
	{ "ei09.13a",           0x10000, 0x6901e628, BRF_GRA },			// 19
	{ "ei10.14a",           0x08000, 0xeeee8a1a, BRF_GRA },			// 20
	
	{ "ei08.2c",            0x10000, 0x3c87463e, BRF_SND },			// 21	Samples
	
	{ "ei31.9a",            0x01000, 0x00000000, BRF_PRG | BRF_NODUMP },	// 22	I8751
};

STD_ROM_PICK(Baddudes)
STD_ROM_FN(Baddudes)

static struct BurnRomInfo DrgninjaRomDesc[] = {
	{ "eg04.3c",            0x10000, 0x41b8b3f8, BRF_ESS | BRF_PRG },	//  0	68000 Program Code
	{ "eg01.3a",            0x10000, 0xe08e6885, BRF_ESS | BRF_PRG },	//  1
	{ "eg06.6c",            0x10000, 0x2b81faf7, BRF_ESS | BRF_PRG },	//  2
	{ "eg03.6a",            0x10000, 0xc52c2e9d, BRF_ESS | BRF_PRG },	//  3
	
	{ "eg07.8a",            0x08000, 0x001d2f51, BRF_ESS | BRF_PRG },	//  4	6502 Program 
	
	{ "eg25.15j",           0x08000, 0xdd557b19, BRF_GRA },			//  5	Characters
	{ "eg26.16j",           0x08000, 0x5d75fc8f, BRF_GRA },			//  6
	
	{ "eg18.14d",           0x10000, 0x05cfc3e5, BRF_GRA },			//  7	Tiles 1
	{ "eg20.17d",           0x10000, 0xe11e988f, BRF_GRA },			//  8
	{ "eg22.14f",           0x10000, 0xb893d880, BRF_GRA },			//  9
	{ "eg24.17f",           0x10000, 0x6f226dda, BRF_GRA },			// 10
	
	{ "eg30.9j",            0x10000, 0x2438e67e, BRF_GRA },			// 11	Tiles 2
	{ "eg28.9f",            0x10000, 0x5c692ab3, BRF_GRA },			// 12
	
	{ "eg15.16c",           0x10000, 0x5617d67f, BRF_GRA },			// 13	Sprites
	{ "eg16.17c",           0x08000, 0x17e42633, BRF_GRA },			// 14
	{ "eg11.16a",           0x10000, 0xba83e8d8, BRF_GRA },			// 15
	{ "eg12.17a",           0x08000, 0xfea2a134, BRF_GRA },			// 16
	{ "eg13.13c",           0x10000, 0xfd91e08e, BRF_GRA },			// 17
	{ "eg14.14c",           0x08000, 0xe83c760a, BRF_GRA },			// 18
	{ "eg09.13a",           0x10000, 0x601b7b23, BRF_GRA },			// 19
	{ "eg10.14a",           0x08000, 0xeeee8a1a, BRF_GRA },			// 20
	
	{ "eg08.2c",            0x10000, 0x92f2c916, BRF_SND },			// 21	Samples
	
	{ "i8751",              0x01000, 0x00000000, BRF_PRG | BRF_NODUMP },	// 22	I8751
};

STD_ROM_PICK(Drgninja)
STD_ROM_FN(Drgninja)

static struct BurnRomInfo HbarrelRomDesc[] = {
	{ "hb04.bin",           0x10000, 0x4877b09e, BRF_ESS | BRF_PRG },	//  0	68000 Program Code
	{ "hb01.bin",           0x10000, 0x8b41c219, BRF_ESS | BRF_PRG },	//  1
	{ "hb05.bin",           0x10000, 0x2087d570, BRF_ESS | BRF_PRG },	//  2
	{ "hb02.bin",           0x10000, 0x815536ae, BRF_ESS | BRF_PRG },	//  3
	{ "hb06.bin",           0x10000, 0xda4e3fbc, BRF_ESS | BRF_PRG },	//  4
	{ "hb03.bin",           0x10000, 0x7fed7c46, BRF_ESS | BRF_PRG },	//  5
	
	{ "hb07.bin",           0x08000, 0xa127f0f7, BRF_ESS | BRF_PRG },	//  6	6502 Program 	
	
	{ "hb25.bin",           0x10000, 0x8649762c, BRF_GRA },			//  7	Characters
	{ "hb26.bin",           0x10000, 0xf8189bbd, BRF_GRA },			//  8
	
	{ "hb18.bin",           0x10000, 0xef664373, BRF_GRA },			//  9	Tiles 1
	{ "hb17.bin",           0x10000, 0xa4f186ac, BRF_GRA },			// 10
	{ "hb20.bin",           0x10000, 0x2fc13be0, BRF_GRA },			// 11
	{ "hb19.bin",           0x10000, 0xd6b47869, BRF_GRA },			// 12
	{ "hb22.bin",           0x10000, 0x50d6a1ad, BRF_GRA },			// 13
	{ "hb21.bin",           0x10000, 0xf01d75c5, BRF_GRA },			// 14
	{ "hb24.bin",           0x10000, 0xae377361, BRF_GRA },			// 15
	{ "hb23.bin",           0x10000, 0xbbdaf771, BRF_GRA },			// 16
	
	{ "hb29.bin",           0x10000, 0x5514b296, BRF_GRA },			// 17	Tiles 2
	{ "hb30.bin",           0x10000, 0x5855e8ef, BRF_GRA },			// 18
	{ "hb27.bin",           0x10000, 0x99db7b9c, BRF_GRA },			// 19
	{ "hb28.bin",           0x10000, 0x33ce2b1a, BRF_GRA },			// 20
	
	{ "hb15.bin",           0x10000, 0x21816707, BRF_GRA },			// 21	Sprites
	{ "hb16.bin",           0x10000, 0xa5684574, BRF_GRA },			// 22
	{ "hb11.bin",           0x10000, 0x5c768315, BRF_GRA },			// 23
	{ "hb12.bin",           0x10000, 0x8b64d7a4, BRF_GRA },			// 24
	{ "hb13.bin",           0x10000, 0x56e3ed65, BRF_GRA },			// 25
	{ "hb14.bin",           0x10000, 0xbedfe7f3, BRF_GRA },			// 26
	{ "hb09.bin",           0x10000, 0x26240ea0, BRF_GRA },			// 27
	{ "hb10.bin",           0x10000, 0x47d95447, BRF_GRA },			// 28
	
	{ "hb08.bin",           0x10000, 0x645c5b68, BRF_SND },			// 29	Samples
	
	{ "hb31.9a",            0x01000, 0x239d726f, BRF_PRG | BRF_OPT },		// 30	I8751
};

STD_ROM_PICK(Hbarrel)
STD_ROM_FN(Hbarrel)

static struct BurnRomInfo HbarrelwRomDesc[] = {
	{ "hb_ec04.rom",        0x10000, 0xd01bc3db, BRF_ESS | BRF_PRG },	//  0	68000 Program Code
	{ "hb_ec01.rom",        0x10000, 0x6756f8ae, BRF_ESS | BRF_PRG },	//  1
	{ "hb05.bin",           0x10000, 0x2087d570, BRF_ESS | BRF_PRG },	//  2
	{ "hb02.bin",           0x10000, 0x815536ae, BRF_ESS | BRF_PRG },	//  3
	{ "hb_ec06.rom",        0x10000, 0x61ec20d8, BRF_ESS | BRF_PRG },	//  4
	{ "hb_ec03.rom",        0x10000, 0x720c6b13, BRF_ESS | BRF_PRG },	//  5
	
	{ "hb_ec07.rom",        0x08000, 0x16a5a1aa, BRF_ESS | BRF_PRG },	//  6	6502 Program 

	{ "hb_ec25.rom",        0x10000, 0x2e5732a2, BRF_GRA },			//  7	Characters
	{ "hb_ec26.rom",        0x10000, 0x161a2c4d, BRF_GRA },			//  8
	
	{ "hb18.bin",           0x10000, 0xef664373, BRF_GRA },			//  9	Tiles 1
	{ "hb17.bin",           0x10000, 0xa4f186ac, BRF_GRA },			// 10
	{ "hb20.bin",           0x10000, 0x2fc13be0, BRF_GRA },			// 11
	{ "hb19.bin",           0x10000, 0xd6b47869, BRF_GRA },			// 12
	{ "hb22.bin",           0x10000, 0x50d6a1ad, BRF_GRA },			// 13
	{ "hb21.bin",           0x10000, 0xf01d75c5, BRF_GRA },			// 14
	{ "hb24.bin",           0x10000, 0xae377361, BRF_GRA },			// 15
	{ "hb23.bin",           0x10000, 0xbbdaf771, BRF_GRA },			// 16
	
	{ "hb29.bin",           0x10000, 0x5514b296, BRF_GRA },			// 17	Tiles 2
	{ "hb30.bin",           0x10000, 0x5855e8ef, BRF_GRA },			// 18
	{ "hb27.bin",           0x10000, 0x99db7b9c, BRF_GRA },			// 19
	{ "hb28.bin",           0x10000, 0x33ce2b1a, BRF_GRA },			// 20
	
	{ "hb15.bin",           0x10000, 0x21816707, BRF_GRA },			// 21	Sprites
	{ "hb16.bin",           0x10000, 0xa5684574, BRF_GRA },			// 22
	{ "hb11.bin",           0x10000, 0x5c768315, BRF_GRA },			// 23
	{ "hb12.bin",           0x10000, 0x8b64d7a4, BRF_GRA },			// 24
	{ "hb13.bin",           0x10000, 0x56e3ed65, BRF_GRA },			// 25
	{ "hb14.bin",           0x10000, 0xbedfe7f3, BRF_GRA },			// 26
	{ "hb09.bin",           0x10000, 0x26240ea0, BRF_GRA },			// 27
	{ "hb10.bin",           0x10000, 0x47d95447, BRF_GRA },			// 28
	
	{ "hb_ec08.rom",        0x10000, 0x2159a609, BRF_SND },			// 29	Samples
	
	{ "ec31.9a",            0x01000, 0xaa14a2ae, BRF_PRG | BRF_OPT },		// 30	I8751
};

STD_ROM_PICK(Hbarrelw)
STD_ROM_FN(Hbarrelw)

static struct BurnRomInfo HippodrmRomDesc[] = {
	{ "ew02",               0x10000, 0xdf0d7dc6, BRF_ESS | BRF_PRG },	//  0	68000 Program Code
	{ "ew01",               0x10000, 0xd5670aa7, BRF_ESS | BRF_PRG },	//  1
	{ "ew05",               0x10000, 0xc76d65ec, BRF_ESS | BRF_PRG },	//  2
	{ "ew00",               0x10000, 0xe9b427a6, BRF_ESS | BRF_PRG },	//  3
	
	{ "ew04",               0x08000, 0x9871b98d, BRF_ESS | BRF_PRG },	//  4	6502 Program 
	
	{ "ew08",               0x10000, 0x53010534, BRF_ESS | BRF_PRG },	//  5	HuC6280 Program
	
	{ "ew14",               0x10000, 0x71ca593d, BRF_GRA },			//  6	Characters
	{ "ew13",               0x10000, 0x86be5fa7, BRF_GRA },			//  7

	{ "ew19",               0x08000, 0x6b80d7a3, BRF_GRA },			//  8	Tiles 1
	{ "ew18",               0x08000, 0x78d3d764, BRF_GRA },			//  9
	{ "ew20",               0x08000, 0xce9f5de3, BRF_GRA },			// 10
	{ "ew21",               0x08000, 0x487a7ba2, BRF_GRA },			// 11
	
	{ "ew24",               0x08000, 0x4e1bc2a4, BRF_GRA },			// 12	Tiles 2
	{ "ew25",               0x08000, 0x9eb47dfb, BRF_GRA },			// 13
	{ "ew23",               0x08000, 0x9ecf479e, BRF_GRA },			// 14
	{ "ew22",               0x08000, 0xe55669aa, BRF_GRA },			// 15
	
	{ "ew15",               0x10000, 0x95423914, BRF_GRA },			// 16	Sprites
	{ "ew16",               0x10000, 0x96233177, BRF_GRA },			// 17
	{ "ew10",               0x10000, 0x4c25dfe8, BRF_GRA },			// 18
	{ "ew11",               0x10000, 0xf2e007fc, BRF_GRA },			// 19
	{ "ew06",               0x10000, 0xe4bb8199, BRF_GRA },			// 20
	{ "ew07",               0x10000, 0x470b6989, BRF_GRA },			// 21
	{ "ew17",               0x10000, 0x8c97c757, BRF_GRA },			// 22
	{ "ew12",               0x10000, 0xa2d244bc, BRF_GRA },			// 23
	
	{ "ew03",               0x10000, 0xb606924d, BRF_SND },			// 24	Samples
};

STD_ROM_PICK(Hippodrm)
STD_ROM_FN(Hippodrm)

static struct BurnRomInfo RobocopRomDesc[] = {
	{ "ep05-4.11c",         0x10000, 0x29c35379, BRF_ESS | BRF_PRG },	//  0	68000 Program Code
	{ "ep01-4.11b",         0x10000, 0x77507c69, BRF_ESS | BRF_PRG },	//  1
	{ "ep04-3",             0x10000, 0x39181778, BRF_ESS | BRF_PRG },	//  2
	{ "ep00-3",             0x10000, 0xe128541f, BRF_ESS | BRF_PRG },	//  3
	
	{ "ep03-3",             0x08000, 0x5b164b24, BRF_ESS | BRF_PRG },	//  4	6502 Program 
	
	{ "en_24_mb7124e.a2",   0x00200, 0xb8e2ca98, BRF_ESS | BRF_PRG },	//  5	HuC6280 Program
	
	{ "ep23",               0x10000, 0xa77e4ab1, BRF_GRA },			//  6	Characters
	{ "ep22",               0x10000, 0x9fbd6903, BRF_GRA },			//  7

	{ "ep20",               0x10000, 0x1d8d38b8, BRF_GRA },			//  8	Tiles 1
	{ "ep21",               0x10000, 0x187929b2, BRF_GRA },			//  9
	{ "ep18",               0x10000, 0xb6580b5e, BRF_GRA },			// 10
	{ "ep19",               0x10000, 0x9bad01c7, BRF_GRA },			// 11
	
	{ "ep14",               0x08000, 0xca56ceda, BRF_GRA },			// 12	Tiles 2
	{ "ep15",               0x08000, 0xa945269c, BRF_GRA },			// 13
	{ "ep16",               0x08000, 0xe7fa4d58, BRF_GRA },			// 14
	{ "ep17",               0x08000, 0x84aae89d, BRF_GRA },			// 15
	
	{ "ep07",               0x10000, 0x495d75cf, BRF_GRA },			// 16	Sprites
	{ "ep06",               0x08000, 0xa2ae32e2, BRF_GRA },			// 17
	{ "ep11",               0x10000, 0x62fa425a, BRF_GRA },			// 18
	{ "ep10",               0x08000, 0xcce3bd95, BRF_GRA },			// 19
	{ "ep09",               0x10000, 0x11bed656, BRF_GRA },			// 20
	{ "ep08",               0x08000, 0xc45c7b4c, BRF_GRA },			// 21
	{ "ep13",               0x10000, 0x8fca9f28, BRF_GRA },			// 22
	{ "ep12",               0x08000, 0x3cd1d0c3, BRF_GRA },			// 23
	
	{ "ep02",               0x10000, 0x711ce46f, BRF_SND },			// 24	Samples
	
	{ "mb7116e.12c",        0x00400, 0xc288a256, BRF_OPT},			// 25	PROMs
	{ "mb7122e.17e",        0x00800, 0x64764ecf, BRF_OPT},			// 26
};

STD_ROM_PICK(Robocop)
STD_ROM_FN(Robocop)

static struct BurnRomInfo RobocopbRomDesc[] = {
	{ "robop_05.rom",       0x10000, 0xbcef3e9b, BRF_ESS | BRF_PRG },	//  0	68000 Program Code
	{ "robop_01.rom",       0x10000, 0xc9803685, BRF_ESS | BRF_PRG },	//  1
	{ "robop_04.rom",       0x10000, 0x9d7b79e0, BRF_ESS | BRF_PRG },	//  2
	{ "robop_00.rom",       0x10000, 0x80ba64ab, BRF_ESS | BRF_PRG },	//  3
	
	{ "ep03-3",             0x08000, 0x5b164b24, BRF_ESS | BRF_PRG },	//  4	6502 Program 
		
	{ "ep23",               0x10000, 0xa77e4ab1, BRF_GRA },			//  5	Characters
	{ "ep22",               0x10000, 0x9fbd6903, BRF_GRA },			//  6

	{ "ep20",               0x10000, 0x1d8d38b8, BRF_GRA },			//  7	Tiles 1
	{ "ep21",               0x10000, 0x187929b2, BRF_GRA },			//  8
	{ "ep18",               0x10000, 0xb6580b5e, BRF_GRA },			//  9
	{ "ep19",               0x10000, 0x9bad01c7, BRF_GRA },			// 10
	
	{ "ep14",               0x08000, 0xca56ceda, BRF_GRA },			// 11	Tiles 2
	{ "ep15",               0x08000, 0xa945269c, BRF_GRA },			// 12
	{ "ep16",               0x08000, 0xe7fa4d58, BRF_GRA },			// 13
	{ "ep17",               0x08000, 0x84aae89d, BRF_GRA },			// 14
	
	{ "ep07",               0x10000, 0x495d75cf, BRF_GRA },			// 15	Sprites
	{ "ep06",               0x08000, 0xa2ae32e2, BRF_GRA },			// 16
	{ "ep11",               0x10000, 0x62fa425a, BRF_GRA },			// 17
	{ "ep10",               0x08000, 0xcce3bd95, BRF_GRA },			// 18
	{ "ep09",               0x10000, 0x11bed656, BRF_GRA },			// 19
	{ "ep08",               0x08000, 0xc45c7b4c, BRF_GRA },			// 20
	{ "ep13",               0x10000, 0x8fca9f28, BRF_GRA },			// 21
	{ "ep12",               0x08000, 0x3cd1d0c3, BRF_GRA },			// 22
	
	{ "ep02",               0x10000, 0x711ce46f, BRF_SND },			// 23	Samples
};

STD_ROM_PICK(Robocopb)
STD_ROM_FN(Robocopb)

static INT32 MemIndex()
{
	UINT8 *Next; Next = Mem;

	Drv68KRom              = Next; Next += 0x60000;
	DrvM6502Rom            = Next; Next += 0x08000;
	DrvH6280Rom            = Next; Next += 0x10000;
	MSM6295ROM             = Next; Next += 0x10000;

	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x05800;
	DrvM6502Ram            = Next; Next += 0x00600;
	DrvH6280Ram            = Next; Next += 0x02000;
	DrvCharRam             = Next; Next += 0x02000;
	DrvCharCtrl0Ram        = Next; Next += 0x00008;
	DrvCharCtrl1Ram        = Next; Next += 0x00008;
	DrvCharColScrollRam    = Next; Next += 0x00080;
	DrvCharRowScrollRam    = Next; Next += 0x00400;
	DrvVideo1Ram           = Next; Next += 0x00800;
	DrvVideo1Ctrl0Ram      = Next; Next += 0x00008;
	DrvVideo1Ctrl1Ram      = Next; Next += 0x00008;
	DrvVideo1ColScrollRam  = Next; Next += 0x00080;
	DrvVideo1RowScrollRam  = Next; Next += 0x00400;
	DrvVideo2Ram           = Next; Next += 0x00800;
	DrvVideo2Ctrl0Ram      = Next; Next += 0x00008;
	DrvVideo2Ctrl1Ram      = Next; Next += 0x00008;
	DrvVideo2ColScrollRam  = Next; Next += 0x00080;
	DrvVideo2RowScrollRam  = Next; Next += 0x00400;
	DrvPaletteRam          = Next; Next += 0x00800;
	DrvPalette2Ram         = Next; Next += 0x00800;	
	DrvSpriteRam           = Next; Next += 0x00800;
	DrvSpriteDMABufferRam  = Next; Next += 0x00800;
	DrvSharedRam           = Next; Next += 0x02000;

	RamEnd                 = Next;

	DrvChars               = Next; Next += 0x1000 * 8 * 8;
	DrvTiles1              = Next; Next += 0x1000 * 16 * 16;
	DrvTiles2              = Next; Next += 0x0800 * 16 * 16;
	DrvSprites             = Next; Next += 0x1000 * 16 * 16;
	DrvPalette             = (UINT32*)Next; Next += 0x00400 * sizeof(UINT32);
	
	pCharLayerDraw         = (UINT16*)Next; Next += (1024 * 256 * sizeof(UINT16));
	pTile1LayerDraw        = (UINT16*)Next; Next += (1024 * 256 * sizeof(UINT16));
	pTile2LayerDraw        = (UINT16*)Next; Next += (1024 * 256 * sizeof(UINT16));

	MemEnd                 = Next;

	return 0;
}

static INT32 DrvDoReset()
{
	SekOpen(0);
	SekReset();
	SekClose();
	
	M6502Open(0);
	M6502Reset();
	M6502Close();
	
	BurnYM3812Reset();
	BurnYM2203Reset();
	MSM6295Reset(0);
	
	i8751RetVal = 0;
	DrvVBlank = 0;
	DrvSoundLatch = 0;
	DrvFlipScreen = 0;
	DrvPriority = 0;
	
	return 0;
}

static INT32 RobocopDoReset()
{
	INT32 nRet = DrvDoReset();
	
	h6280Open(0);
	h6280Reset();
	h6280Close();
	
	return nRet;
}

static void BaddudesI8751Write(UINT16 Data)
{
	i8751RetVal = 0;

	switch (Data & 0xffff) {
		case 0x714: i8751RetVal = 0x700; break;
		case 0x73b: i8751RetVal = 0x701; break;
		case 0x72c: i8751RetVal = 0x702; break;
		case 0x73f: i8751RetVal = 0x703; break;
		case 0x755: i8751RetVal = 0x704; break;
		case 0x722: i8751RetVal = 0x705; break;
		case 0x72b: i8751RetVal = 0x706; break;
		case 0x724: i8751RetVal = 0x707; break;
		case 0x728: i8751RetVal = 0x708; break;
		case 0x735: i8751RetVal = 0x709; break;
		case 0x71d: i8751RetVal = 0x70a; break;
		case 0x721: i8751RetVal = 0x70b; break;
		case 0x73e: i8751RetVal = 0x70c; break;
		case 0x761: i8751RetVal = 0x70d; break;
		case 0x753: i8751RetVal = 0x70e; break;
		case 0x75b: i8751RetVal = 0x70f; break;
	}
}

static void HbarrelI8751Write(UINT16 Data)
{
	static INT32 Level, State;

	static const INT32 Title[]={  1, 2, 5, 6, 9,10,13,14,17,18,21,22,25,26,29,30,33,34,37,38,41,42,0,
                 3, 4, 7, 8,11,12,15,16,19,20,23,24,27,28,31,32,35,36,39,40,43,44,0,
                45,46,49,50,53,54,57,58,61,62,65,66,69,70,73,74,77,78,81,82,0,
                47,48,51,52,55,56,59,60,63,64,67,68,71,72,75,76,79,80,83,84,0,
                85,86,89,90,93,94,97,98,101,102,105,106,109,110,113,114,117,118,121,122,125,126,0,
                87,88,91,92,95,96,99,100,103,104,107,108,111,112,115,116,119,120,123,124,127,128,0,
                129,130,133,134,137,138,141,142,145,146,149,150,153,154,157,158,161,162,165,166,169,170,173,174,0,
                131,132,135,136,139,140,143,144,147,148,151,152,155,156,159,160,163,164,167,168,171,172,175,176,0,
                0x10b1,0x10b2,0,0x10b3,0x10b4,-1
	};

	static const INT32 WeaponsTable[][0x20]={
		{ 0x558,0x520,0x5c0,0x600,0x520,0x540,0x560,0x5c0,0x688,0x688,0x7a8,0x850,0x880,0x880,0x990,0x9b0,0x9b0,0x9e0,0xffff }, /* Level 1 */
		{ 0x330,0x370,0x3d8,0x580,0x5b0,0x640,0x6a0,0x8e0,0x8e0,0x940,0x9f0,0xa20,0xa50,0xa80,0xffff }, /* Level 2 */
		{ 0xb20,0xbd0,0xb20,0xb20,0xbd8,0xb50,0xbd8,0xb20,0xbe0,0xb40,0xb80,0xa18,0xa08,0xa08,0x980,0x8e0,0x780,0x790,0x650,0x600,0x5d0,0x5a0,0x570,0x590,0x5e0,0xffff }, /* Level 3 */
		{ 0x530,0x5d0,0x5e0,0x5c8,0x528,0x520,0x5d8,0x5e0,0x5d8,0x540,0x570,0x5a0,0x658,0x698,0x710,0x7b8,0x8e0,0x8e0,0x8d8,0x818,0x8e8,0x820,0x8e0,0x848,0x848,0xffff }, /* Level 4 */
		{ 0x230,0x280,0x700,0x790,0x790,0x7e8,0x7e8,0x8d0,0x920,0x950,0xad0,0xb90,0xb50,0xb10,0xbe0,0xbe0,0xffff }, /* Level 5 */
		{ 0xd20,0xde0,0xd20,0xde0,0xd80,0xd80,0xd90,0xdd0,0xdb0,0xb20,0xa40,0x9e0,0x960,0x8a0,0x870,0x840,0x7e0,0x7b0,0x780,0xffff }, /* Level 6 */
		{ 0x730,0x7e0,0x720,0x7e0,0x740,0x7c0,0x730,0x7d0,0x740,0x7c0,0x730,0x7d0,0x720,0x7e0,0x720,0x7e0,0x720,0x7e0,0x720,0x7e0,0x730,0x7d0,0xffff } /* Level 7 */
	};

	switch (Data >> 8) {
		case 0x02: {
			i8751RetVal = Level;
			break;
		}
		
		case 0x03: {
			Level++;
			i8751RetVal = 0x301;
			break;
		}
		
		case 0x05: {
			i8751RetVal = 0xb3b;
			Level = 0;
			break;
		}
		
		case 0x06: {
			i8751RetVal = WeaponsTable[Level][Data & 0x1f];
			break;
		}
		
		case 0x0b: {
			i8751RetVal = 0;
			break;
		}
		
		default: {
			i8751RetVal = 0;
		}
	}

	if (Data == 7) i8751RetVal = 0xc000;
	if (Data == 0x175) i8751RetVal = 0x68b;
	if (Data == 0x174) i8751RetVal = 0x68c;

	if (Data == 0x4ff) State=0;
	if (Data > 0x3ff && Data < 0x4ff) {
		State++;

		if (Title[State - 1] == 0) {
			i8751RetVal = 0xfffe;
		} else {
			if (Title[State - 1] == -1) {
				i8751RetVal = 0xffff; 
			} else {
				if (Title[State - 1] > 0x1000) {
					i8751RetVal = (Title[State - 1] & 0xfff) + 128 + 15;
				} else {
					i8751RetVal = Title[State - 1] + 128 + 15 + 0x2000;
				}
			}
		}
	}
}

UINT8 __fastcall Dec068KReadByte(UINT32 a)
{
	if (a >= 0x300000 && a <= 0x30001f) {
		// rotary_r
		return 0;
	}
	
	switch (a) {
		case 0x30c001: {
			return 0xff - DrvInput[0];
		}
		
		case 0x30c003: {
			return (0x7f - DrvInput[2]) | ((DrvVBlank) ? 0x80 : 0x00);
		}
		
		case 0x30c004: {
			return DrvDip[1];
		}
		
		case 0x30c005: {
			return DrvDip[0];
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read byte => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Dec068KWriteByte(UINT32 a, UINT8 d)
{
	switch (a) {
		case 0x30c011: {
			DrvPriority = d;
			return;
		}
		
		case 0x30c015: {
			DrvSoundLatch = d;
			M6502Open(0);
			M6502SetIRQ(M6502_INPUT_LINE_NMI, M6502_IRQSTATUS_AUTO);
			M6502Close();
			return;
		}
		
		case 0x30c01f: {
			i8751RetVal = 0;
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write byte => %06X, %02X\n"), a, d);
		}
	}
}

UINT16 __fastcall Dec068KReadWord(UINT32 a)
{
	if (a >= 0x300000 && a <= 0x30001f) {
		// rotary_r
		return 0;
	}
	
	switch (a) {
		case 0x30c000: {
			return ((0xff - DrvInput[1]) << 8) | (0xff - DrvInput[0]);
		}
		
		case 0x30c002: {
			return (0xff7f - DrvInput[2]) | ((DrvVBlank) ? 0x80 : 0x00);
		}
		
		case 0x30c004: {
			return (DrvDip[1] << 8) | DrvDip[0];
		}
		
		case 0x30c008: {
			return i8751RetVal;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read word => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Dec068KWriteWord(UINT32 a, UINT16 d)
{
	if (a >= 0x31c000 && a <= 0x31c7ff) {
		// ???
		return;
	}
	
	switch (a) {
		case 0x240000:
		case 0x240002:
		case 0x240004:
		case 0x240006: {		
			UINT16 *Control0 = (UINT16*)DrvCharCtrl0Ram;
			Control0[(a - 0x240000) >> 1] = d;
			return;
		}
		
		case 0x240010:
		case 0x240012:
		case 0x240014:
		case 0x240016: {		
			UINT16 *Control1 = (UINT16*)DrvCharCtrl1Ram;
			Control1[(a - 0x240010) >> 1] = d;
			return;
		}
		
		case 0x246000:
		case 0x246002:
		case 0x246004:
		case 0x246006: {		
			UINT16 *Control0 = (UINT16*)DrvVideo1Ctrl0Ram;
			Control0[(a - 0x246000) >> 1] = d;
			return;
		}
		
		case 0x246010:
		case 0x246012:
		case 0x246014:
		case 0x246016: {		
			UINT16 *Control1 = (UINT16*)DrvVideo1Ctrl1Ram;
			Control1[(a - 0x246010) >> 1] = d;
			return;
		}
		
		case 0x24c000:
		case 0x24c002:
		case 0x24c004:
		case 0x24c006: {		
			UINT16 *Control0 = (UINT16*)DrvVideo2Ctrl0Ram;
			Control0[(a - 0x24c000) >> 1] = d;
			return;
		}
		
		case 0x24c010:
		case 0x24c012:
		case 0x24c014:
		case 0x24c016: {		
			UINT16 *Control1 = (UINT16*)DrvVideo2Ctrl1Ram;
			Control1[(a - 0x24c010) >> 1] = d;
			return;
		}
		
		case 0x30c010: {
			DrvPriority = d;
			return;
		}
		
		case 0x30c012: {
			memcpy(DrvSpriteDMABufferRam, DrvSpriteRam, 0x800);
			return;
		}
		
		case 0x30c014: {
			DrvSoundLatch = d & 0xff;
			M6502Open(0);
			M6502SetIRQ(M6502_INPUT_LINE_NMI, M6502_IRQSTATUS_AUTO);
			M6502Close();
			return;
		}
		
		case 0x30c016: {
			if (Dec0Game == DEC0_GAME_BADDUDES) BaddudesI8751Write(d);
			if (Dec0Game == DEC0_GAME_HBARREL) HbarrelI8751Write(d);
			
			SekSetIRQLine(5, SEK_IRQSTATUS_AUTO);
			
			return;
		}
		
		case 0x30c018: {
			SekSetIRQLine(6, SEK_IRQSTATUS_NONE);
			return;
		}
		
		case 0x30c01e: {
			i8751RetVal = 0;
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write word => %06X, %04X\n"), a, d);
		}
	}
}

UINT8 __fastcall HippodrmShared68KReadByte(UINT32 a)
{
	INT32 Offset = (a - 0x180000) >> 1;
	return DrvSharedRam[Offset];
}

void __fastcall HippodrmShared68KWriteByte(UINT32 a, UINT8 d)
{
	INT32 Offset = (a - 0x180000) >> 1;
	DrvSharedRam[Offset] = d;
}

UINT16 __fastcall HippodrmShared68KReadWord(UINT32 a)
{
	INT32 Offset = (a - 0x180000) >> 1;
	return DrvSharedRam[Offset];
}

void __fastcall HippodrmShared68KWriteWord(UINT32 a, UINT16 d)
{
	INT32 Offset = (a - 0x180000) >> 1;
	DrvSharedRam[Offset] = d & 0xff;
}

UINT8 __fastcall RobocopShared68KReadByte(UINT32 a)
{
	INT32 Offset = (a - 0x180000) >> 1;
	return DrvSharedRam[Offset];
}

void __fastcall RobocopShared68KWriteByte(UINT32 a, UINT8 d)
{
	INT32 Offset = (a - 0x180000) >> 1;
	DrvSharedRam[Offset] = d;
	if (Offset == 0x7ff) {
		bprintf(PRINT_NORMAL, _T("IRQ Raised\n"));
	}
}

UINT16 __fastcall RobocopShared68KReadWord(UINT32 a)
{
	INT32 Offset = (a - 0x180000) >> 1;
	return DrvSharedRam[Offset];
}

void __fastcall RobocopShared68KWriteWord(UINT32 a, UINT16 d)
{
	INT32 Offset = (a - 0x180000) >> 1;
	DrvSharedRam[Offset] = d & 0xff;
	if (Offset == 0x7ff) {
		bprintf(PRINT_NORMAL, _T("IRQ Raised\n"));
	}
}

UINT8 Dec0SoundReadByte(UINT16 a)
{
	switch (a) {
		case 0x3000: {
			return DrvSoundLatch;
		}
		
		case 0x3800: {
			return MSM6295ReadStatus(0);
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("M6502 Read Byte %04X\n"), a);
		}
	}

	return 0;
}

void Dec0SoundWriteByte(UINT16 a, UINT8 d)
{
	switch (a) {
		case 0x0800: {
			BurnYM2203Write(0, 0, d);
			return;
		}
		
		case 0x0801: {
			BurnYM2203Write(0, 1, d);
			return;
		}
		
		case 0x1000: {
			BurnYM3812Write(0, d);
			return;
		}
		
		case 0x1001: {
			BurnYM3812Write(1, d);
			return;
		}
		
		case 0x3800: {
			MSM6295Command(0, d);
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("M6502 Write Byte %04X, %02X\n"), a, d);
		}
	}
}

void HippodrmH6280WriteIo(UINT8 Port, UINT8 Data)
{
	bprintf(PRINT_NORMAL, _T("H6280 Write Port %x, %x\n"), Port, Data);
}

UINT8 HippodrmH6280ReadProg(UINT32 Address)
{
	bprintf(PRINT_NORMAL, _T("H6280 Read Prog %x\n"), Address);
	
	return 0;
}

void HippodrmH6280WriteProg(UINT32 Address, UINT8 Data)
{
	if (Address <= 0x00ffff) return;
	
	bprintf(PRINT_NORMAL, _T("H6280 Write Prog %x, %x\n"), Address, Data);
}

void RobocopH6280WriteIo(UINT8 Port, UINT8 Data)
{
	bprintf(PRINT_NORMAL, _T("H6280 Write Port %x, %x\n"), Port, Data);
}

UINT8 RobocopH6280ReadProg(UINT32 Address)
{
	bprintf(PRINT_NORMAL, _T("H6280 Read Prog %x\n"), Address);
	
	return 0;
}

void RobocopH6280WriteProg(UINT32 Address, UINT8 Data)
{
	if (Address <= 0x00ffff) return;
	
	if (Address >= 0x1ff400 && Address <= 0x1ff403) {
		h6280_irq_status_w(Address - 0x1ff400, Data);
		return;
	}
	
	bprintf(PRINT_NORMAL, _T("H6280 Write Prog %x, %x\n"), Address, Data);
}

static INT32 CharPlaneOffsets[4]         = { 0x000000, 0x040000, 0x020000, 0x060000 };
static INT32 RobocopCharPlaneOffsets[4]  = { 0x000000, 0x080000, 0x040000, 0x0c0000 };
static INT32 CharXOffsets[8]             = { 0, 1, 2, 3, 4, 5, 6, 7 };
static INT32 CharYOffsets[8]             = { 0, 8, 16, 24, 32, 40, 48, 56 };
static INT32 Tile1PlaneOffsets[4]        = { 0x080000, 0x180000, 0x000000, 0x100000 };
static INT32 HbarrelTile1PlaneOffsets[4] = { 0x100000, 0x300000, 0x000000, 0x200000 };
static INT32 Tile2PlaneOffsets[4]        = { 0x040000, 0x0c0000, 0x000000, 0x080000 };
static INT32 HbarrelTile2PlaneOffsets[4] = { 0x080000, 0x180000, 0x000000, 0x100000 };
static INT32 SpritePlaneOffsets[4]       = { 0x100000, 0x300000, 0x000000, 0x200000 };
static INT32 TileXOffsets[16]            = { 128, 129, 130, 131, 132, 133, 134, 135, 0, 1, 2, 3, 4, 5, 6, 7 };
static INT32 TileYOffsets[16]            = { 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120 };

inline static INT32 Dec0YM2203SynchroniseStream(INT32 nSoundRate)
{
	return (INT64)SekTotalCycles() * nSoundRate / 10000000;
}

inline static double Dec0YM2203GetTime()
{
	return (double)SekTotalCycles() / 10000000;
}

static void Dec0YM3812IRQHandler(INT32, INT32 nStatus)
{
	if (nStatus) {
		M6502SetIRQ(M6502_IRQ_LINE, M6502_IRQSTATUS_ACK);
	} else {
		M6502SetIRQ(M6502_IRQ_LINE, M6502_IRQSTATUS_NONE);
	}
}

static INT32 Dec0YM3812SynchroniseStream(INT32 nSoundRate)
{
	return (INT64)M6502TotalCycles() * nSoundRate / 1500000;
}

static INT32 Dec0MachineInit()
{
	INT32 nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (UINT8 *)0;
	if ((Mem = (UINT8 *)BurnMalloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	MemIndex();

	DrvTempRom = (UINT8 *)BurnMalloc(0x80000);
	
	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom               , 0x000000, 0x05ffff, SM_ROM);
	SekMapMemory(DrvCharColScrollRam     , 0x242000, 0x24207f, SM_RAM);
	SekMapMemory(DrvCharRowScrollRam     , 0x242400, 0x2427ff, SM_RAM);
	SekMapMemory(Drv68KRam + 0x4000      , 0x242800, 0x243fff, SM_RAM);
	SekMapMemory(DrvCharRam              , 0x244000, 0x245fff, SM_RAM);	
	SekMapMemory(DrvVideo1ColScrollRam   , 0x248000, 0x24807f, SM_RAM);
	SekMapMemory(DrvVideo1RowScrollRam   , 0x248400, 0x2487ff, SM_RAM);
	SekMapMemory(DrvVideo1Ram            , 0x24a000, 0x24a7ff, SM_RAM);	
	SekMapMemory(DrvVideo2ColScrollRam   , 0x24c800, 0x24c87f, SM_RAM);
	SekMapMemory(DrvVideo2RowScrollRam   , 0x24cc00, 0x24cfff, SM_RAM);
	SekMapMemory(DrvVideo2Ram            , 0x24d000, 0x24d7ff, SM_RAM);
	SekMapMemory(DrvPaletteRam           , 0x310000, 0x3107ff, SM_RAM);
	SekMapMemory(DrvPalette2Ram          , 0x314000, 0x3147ff, SM_RAM);
	SekMapMemory(Drv68KRam               , 0xff8000, 0xffbfff, SM_RAM);
	SekMapMemory(DrvSpriteRam            , 0xffc000, 0xffc7ff, SM_RAM);
	SekSetReadByteHandler(0, Dec068KReadByte);
	SekSetWriteByteHandler(0, Dec068KWriteByte);
	SekSetReadWordHandler(0, Dec068KReadWord);
	SekSetWriteWordHandler(0, Dec068KWriteWord);	
	SekClose();
	
	// Setup the M6502 emulation
	M6502Init(0, TYPE_M6502);
	M6502Open(0);
	M6502MapMemory(DrvM6502Ram            , 0x0000, 0x05ff, M6502_RAM);
	M6502MapMemory(DrvM6502Rom            , 0x8000, 0xffff, M6502_ROM);
	M6502SetReadByteHandler(Dec0SoundReadByte);
	M6502SetWriteByteHandler(Dec0SoundWriteByte);
	M6502Close();
	
	GenericTilesInit();
	
	BurnYM3812Init(3000000, &Dec0YM3812IRQHandler, &Dec0YM3812SynchroniseStream, 1);
	BurnTimerAttachM6502YM3812(1500000);
	
	BurnYM2203Init(1, 1500000, NULL, Dec0YM2203SynchroniseStream, Dec0YM2203GetTime, 0);
	BurnTimerAttachSek(10000000);
	
	MSM6295Init(0, 1023924 / 132, 80, 1);
	
	return 0;
}

static INT32 BaddudesInit()
{
	INT32 nRet = 0;

	Dec0MachineInit();

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x00001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x00000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x40001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x40000, 3, 2); if (nRet != 0) return 1;
	
	// Load M6502 Program Rom
	nRet = BurnLoadRom(DrvM6502Rom, 4, 1); if (nRet != 0) return 1;
	
	// Load and decode chars
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x08000,  6, 1); if (nRet != 0) return 1;
	GfxDecode(0x800, 4, 8, 8, CharPlaneOffsets, CharXOffsets, CharYOffsets, 0x40, DrvTempRom, DrvChars);
	
	// Load and decode tiles1
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000,  8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 10, 1); if (nRet != 0) return 1;
	GfxDecode(0x800, 4, 16, 16, Tile1PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles1);
	
	// Load and decode tiles2
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 11, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 12, 1); if (nRet != 0) return 1;
	memcpy(DrvTempRom + 0x08000, DrvTempRom + 0x20000, 0x8000);
	memcpy(DrvTempRom + 0x00000, DrvTempRom + 0x28000, 0x8000);
	memcpy(DrvTempRom + 0x18000, DrvTempRom + 0x30000, 0x8000);
	memcpy(DrvTempRom + 0x10000, DrvTempRom + 0x38000, 0x8000);	
	GfxDecode(0x400, 4, 16, 16, Tile2PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles2);
	
	// Load and decode sprites
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 13, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 14, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 15, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 16, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x40000, 17, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x50000, 18, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x60000, 19, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x70000, 20, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 16, 16, SpritePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvSprites);
	
	// Load the samples
	nRet = BurnLoadRom(MSM6295ROM + 0x00000, 21, 1); if (nRet != 0) return 1;
	
	BurnFree(DrvTempRom);
	
	Dec0DrawFunction = BaddudesDraw;
	Dec0Game = DEC0_GAME_BADDUDES;

	// Reset the driver
	DrvDoReset();

	return 0;
}

static INT32 HbarrelInit()
{
	INT32 nRet = 0;

	Dec0MachineInit();

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x00001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x00000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x20001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x20000, 3, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x40001, 4, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x40000, 5, 2); if (nRet != 0) return 1;
	
	// Load M6502 Program Rom
	nRet = BurnLoadRom(DrvM6502Rom, 6, 1); if (nRet != 0) return 1;
	
	// Load and decode chars
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000,  8, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 8, 8, RobocopCharPlaneOffsets, CharXOffsets, CharYOffsets, 0x40, DrvTempRom, DrvChars);
	
	// Load and decode tiles1
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 10, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 11, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 12, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x40000, 13, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x50000, 14, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x60000, 15, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x70000, 16, 1); if (nRet != 0) return 1;
	GfxDecode(01000, 4, 16, 16, HbarrelTile1PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles1);
	
	// Load and decode tiles2
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 17, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 18, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 19, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 20, 1); if (nRet != 0) return 1;
	GfxDecode(0x800, 4, 16, 16, HbarrelTile2PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles2);
	
	// Load and decode sprites
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 21, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 22, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 23, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 24, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x40000, 25, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x50000, 26, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x60000, 27, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x70000, 28, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 16, 16, SpritePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvSprites);
	
	// Load the samples
	nRet = BurnLoadRom(MSM6295ROM + 0x00000, 29, 1); if (nRet != 0) return 1;
	
	BurnFree(DrvTempRom);
	
	Dec0DrawFunction = HbarrelDraw;
	Dec0Game = DEC0_GAME_HBARREL;
	
	UINT16 *Rom = (UINT16 *)Drv68KRom;
	Rom[0xb68 >> 1] = 0x8008;

	// Reset the driver
	DrvDoReset();

	return 0;
}

static INT32 HippodrmInit()
{
	INT32 nRet = 0;

	Dec0MachineInit();
	
	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x00001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x00000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x20001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x20000, 3, 2); if (nRet != 0) return 1;
	
	// Load M6502 Program Rom
	nRet = BurnLoadRom(DrvM6502Rom, 4, 1); if (nRet != 0) return 1;
	
	nRet = BurnLoadRom(DrvH6280Rom, 5, 1); if (nRet != 0) return 1;
	
	// Load and decode chars
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  6, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000,  7, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 8, 8, RobocopCharPlaneOffsets, CharXOffsets, CharYOffsets, 0x40, DrvTempRom, DrvChars);
	
	// Load and decode tiles1
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x08000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 10, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x18000, 11, 1); if (nRet != 0) return 1;
	GfxDecode(0x400, 4, 16, 16, Tile1PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles1);
	
	// Load and decode tiles2
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 12, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x08000, 13, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 14, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x18000, 15, 1); if (nRet != 0) return 1;
	GfxDecode(0x400, 4, 16, 16, Tile2PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles2);
	
	// Load and decode sprites
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 16, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 17, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 18, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 19, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x40000, 20, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x50000, 21, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x60000, 22, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x70000, 23, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 16, 16, SpritePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvSprites);
	
	// Load the samples
	nRet = BurnLoadRom(MSM6295ROM + 0x00000, 24, 1); if (nRet != 0) return 1;

	BurnFree(DrvTempRom);
	
	Dec0DrawFunction = RobocopDraw;
	
	SekOpen(0);
//	SekMapMemory(DrvSharedRam, 0x180000, 0x180fff, SM_RAM);
	SekMapHandler(1, 0x180000, 0x180fff, SM_RAM);
	SekSetReadByteHandler(1, HippodrmShared68KReadByte);
	SekSetWriteByteHandler(1, HippodrmShared68KWriteByte);
	SekSetReadWordHandler(1, HippodrmShared68KReadWord);
	SekSetWriteWordHandler(1, HippodrmShared68KWriteWord);	
	SekClose();
	
	h6280Init(1);
	h6280Open(0);
	h6280MapMemory(DrvH6280Rom , 0x000000, 0x00ffff, H6280_ROM);
//	h6280MapMemory(DrvH6280Ram , 0x1f0000, 0x1f1fff, H6280_RAM);
//	h6280MapMemory(DrvSharedRam, 0x1f2000, 0x1f3fff, H6280_RAM);
//	h6280MapMemory(DrvH6280Rom , 0x1fe000, 0x1fffff, H6280_ROM);
	h6280SetWritePortHandler(HippodrmH6280WriteIo);
	h6280SetReadHandler(HippodrmH6280ReadProg);
	h6280SetWriteHandler(HippodrmH6280WriteProg);
	h6280Close();

	// Reset the driver
	RobocopDoReset();

	return 0;
}

static INT32 RobocopInit()
{
	INT32 nRet = 0;

	Dec0MachineInit();
	
	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x00001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x00000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x20001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x20000, 3, 2); if (nRet != 0) return 1;
	
	// Load M6502 Program Rom
	nRet = BurnLoadRom(DrvM6502Rom, 4, 1); if (nRet != 0) return 1;
	
	nRet = BurnLoadRom(DrvH6280Rom + 0x01e00, 5, 1); if (nRet != 0) return 1;
	
	// Load and decode chars
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  6, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000,  7, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 8, 8, RobocopCharPlaneOffsets, CharXOffsets, CharYOffsets, 0x40, DrvTempRom, DrvChars);
	
	// Load and decode tiles1
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 10, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 11, 1); if (nRet != 0) return 1;
	GfxDecode(0x800, 4, 16, 16, Tile1PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles1);
	
	// Load and decode tiles2
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 12, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x08000, 13, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 14, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x18000, 15, 1); if (nRet != 0) return 1;
	GfxDecode(0x400, 4, 16, 16, Tile2PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles2);
	
	// Load and decode sprites
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 16, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 17, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 18, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 19, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x40000, 20, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x50000, 21, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x60000, 22, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x70000, 23, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 16, 16, SpritePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvSprites);
	
	// Load the samples
	nRet = BurnLoadRom(MSM6295ROM + 0x00000, 24, 1); if (nRet != 0) return 1;

	BurnFree(DrvTempRom);
	
	Dec0DrawFunction = RobocopDraw;
	
	SekOpen(0);
	SekMapHandler(1, 0x180000, 0x180fff, SM_RAM);
	SekSetReadByteHandler(1, RobocopShared68KReadByte);
	SekSetWriteByteHandler(1, RobocopShared68KWriteByte);
	SekSetReadWordHandler(1, RobocopShared68KReadWord);
	SekSetWriteWordHandler(1, RobocopShared68KWriteWord);	
	SekClose();
	
	h6280Init(1);
	h6280Open(0);
	h6280MapMemory(DrvH6280Rom , 0x000000, 0x00ffff, H6280_ROM);
	h6280MapMemory(DrvH6280Ram , 0x1f0000, 0x1f1fff, H6280_RAM);
	h6280MapMemory(DrvSharedRam, 0x1f2000, 0x1f3fff, H6280_RAM);
	h6280MapMemory(DrvH6280Rom , 0x1fe000, 0x1fffff, H6280_ROM);
	h6280SetWritePortHandler(RobocopH6280WriteIo);
	h6280SetReadHandler(RobocopH6280ReadProg);
	h6280SetWriteHandler(RobocopH6280WriteProg);
	h6280Close();

	// Reset the driver
	RobocopDoReset();

	return 0;
}

static INT32 RobocopbInit()
{
	INT32 nRet = 0;

	Dec0MachineInit();

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x00001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x00000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x20001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x20000, 3, 2); if (nRet != 0) return 1;
	
	// Load M6502 Program Rom
	nRet = BurnLoadRom(DrvM6502Rom, 4, 1); if (nRet != 0) return 1;
	
	// Load and decode chars
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000,  6, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 8, 8, RobocopCharPlaneOffsets, CharXOffsets, CharYOffsets, 0x40, DrvTempRom, DrvChars);
	
	// Load and decode tiles1
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000,  8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 10, 1); if (nRet != 0) return 1;
	GfxDecode(0x800, 4, 16, 16, Tile1PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles1);
	
	// Load and decode tiles2
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 11, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x08000, 12, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 13, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x18000, 14, 1); if (nRet != 0) return 1;
	GfxDecode(0x400, 4, 16, 16, Tile2PlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvTiles2);
	
	// Load and decode sprites
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 15, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 16, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 17, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 18, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x40000, 19, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x50000, 20, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x60000, 21, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x70000, 22, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 16, 16, SpritePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTempRom, DrvSprites);
	
	// Load the samples
	nRet = BurnLoadRom(MSM6295ROM + 0x00000, 23, 1); if (nRet != 0) return 1;
	
	BurnFree(DrvTempRom);
	
	Dec0DrawFunction = RobocopDraw;

	// Reset the driver
	DrvDoReset();

	return 0;
}

static INT32 DrvExit()
{
	SekExit();
	M6502Exit();
	
	BurnYM2203Exit();
	BurnYM3812Exit();
	MSM6295Exit(0);

	GenericTilesExit();
	
	i8751RetVal = 0;
	DrvVBlank = 0;
	DrvSoundLatch = 0;
	DrvFlipScreen = 0;
	DrvPriority = 0;
	
	DrvCharTilemapWidth = 0;
	DrvCharTilemapHeight = 0;
	DrvTile1TilemapWidth = 0;
	DrvTile1TilemapHeight = 0;
	DrvTile2TilemapWidth = 0;
	DrvTile2TilemapHeight = 0;
	
	Dec0DrawFunction = NULL;
	
	Dec0Game = 0;
	
	BurnFree(Mem);

	return 0;
}

static void DrvCalcPalette()
{
	UINT16 *PaletteRam = (UINT16*)DrvPaletteRam;
	UINT16 *Palette2Ram = (UINT16*)DrvPalette2Ram;
	
	INT32 r, g, b;
	
	for (INT32 i = 0; i < 0x400; i++) {
		r = (PaletteRam[i] >> 0) & 0xff;
		g = (PaletteRam[i] >> 8) & 0xff;
		b = (Palette2Ram[i] >> 0) & 0xff;
		
		DrvPalette[i] = BurnHighCol(r, g, b, 0);
	}
}

#define PLOTPIXEL(x, po) pPixel[x] = nPalette | pTileData[x] | po;
#define PLOTPIXEL_FLIPX(x, a, po) pPixel[x] = nPalette | pTileData[a] | po;
#define PLOTPIXEL_MASK(x, mc, po) if (pTileData[x] != mc) {pPixel[x] = nPalette | pTileData[x] | po;}
#define PLOTPIXEL_MASK_FLIPX(x, a, mc, po) if (pTileData[a] != mc) {pPixel[x] = nPalette | pTileData[a] | po;}
#define CLIPPIXEL(x, sx, mx, a) if ((sx + x) >= 0 && (sx + x) < mx) { a; };

static void Dec0Render8x8Tile_Mask(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nMaskColour, INT32 nPaletteOffset, UINT8 *pTile, INT32 TilemapWidth)
{
	UINT32 nPalette = nTilePalette << nColourDepth;
	pTileData = pTile + (nTileNumber << 6);
	
	UINT16* pPixel = pDestDraw + (StartY * TilemapWidth) + StartX;

	for (INT32 y = 0; y < 8; y++, pPixel += TilemapWidth, pTileData += 8) {
		PLOTPIXEL_MASK( 0, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 1, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 2, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 3, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 4, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 5, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 6, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 7, nMaskColour, nPaletteOffset);
	}
}

static void Dec0Render8x8Tile_Mask_FlipXY(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nMaskColour, INT32 nPaletteOffset, UINT8 *pTile, INT32 TilemapWidth)
{
	UINT32 nPalette = nTilePalette << nColourDepth;
	pTileData = pTile + (nTileNumber << 6);
	
	UINT16* pPixel = pDestDraw + ((StartY + 7) * TilemapWidth) + StartX;

	for (INT32 y = 7; y >= 0; y--, pPixel -= TilemapWidth, pTileData += 8) {
		PLOTPIXEL_MASK_FLIPX(7, 0, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(6, 1, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(5, 2, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(4, 3, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(3, 4, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(2, 5, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(1, 6, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(0, 7, nMaskColour, nPaletteOffset);
	}
}

static void Dec0Render16x16Tile(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nPaletteOffset, UINT8 *pTile, INT32 TilemapWidth)
{
	UINT32 nPalette = nTilePalette << nColourDepth;
	pTileData = pTile + (nTileNumber << 8);

	UINT16* pPixel = pDestDraw + (StartY * TilemapWidth) + StartX;

	for (INT32 y = 0; y < 16; y++, pPixel += TilemapWidth, pTileData += 16) {
		PLOTPIXEL( 0, nPaletteOffset);
		PLOTPIXEL( 1, nPaletteOffset);
		PLOTPIXEL( 2, nPaletteOffset);
		PLOTPIXEL( 3, nPaletteOffset);
		PLOTPIXEL( 4, nPaletteOffset);
		PLOTPIXEL( 5, nPaletteOffset);
		PLOTPIXEL( 6, nPaletteOffset);
		PLOTPIXEL( 7, nPaletteOffset);
		PLOTPIXEL( 8, nPaletteOffset);
		PLOTPIXEL( 9, nPaletteOffset);
		PLOTPIXEL(10, nPaletteOffset);
		PLOTPIXEL(11, nPaletteOffset);
		PLOTPIXEL(12, nPaletteOffset);
		PLOTPIXEL(13, nPaletteOffset);
		PLOTPIXEL(14, nPaletteOffset);
		PLOTPIXEL(15, nPaletteOffset);
	}
}

static void Dec0Render16x16Tile_FlipXY(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nPaletteOffset, UINT8 *pTile, INT32 TilemapWidth)
{
	UINT32 nPalette = nTilePalette << nColourDepth;
	pTileData = pTile + (nTileNumber << 8);

	UINT16* pPixel = pDestDraw + ((StartY + 15) * TilemapWidth) + StartX;

	for (INT32 y = 15; y >= 0; y--, pPixel -= TilemapWidth, pTileData += 16) {
		PLOTPIXEL_FLIPX(15,  0, nPaletteOffset);
		PLOTPIXEL_FLIPX(14,  1, nPaletteOffset);
		PLOTPIXEL_FLIPX(13,  2, nPaletteOffset);
		PLOTPIXEL_FLIPX(12,  3, nPaletteOffset);
		PLOTPIXEL_FLIPX(11,  4, nPaletteOffset);
		PLOTPIXEL_FLIPX(10,  5, nPaletteOffset);
		PLOTPIXEL_FLIPX( 9,  6, nPaletteOffset);
		PLOTPIXEL_FLIPX( 8,  7, nPaletteOffset);
		PLOTPIXEL_FLIPX( 7,  8, nPaletteOffset);
		PLOTPIXEL_FLIPX( 6,  9, nPaletteOffset);
		PLOTPIXEL_FLIPX( 5, 10, nPaletteOffset);
		PLOTPIXEL_FLIPX( 4, 11, nPaletteOffset);
		PLOTPIXEL_FLIPX( 3, 12, nPaletteOffset);
		PLOTPIXEL_FLIPX( 2, 13, nPaletteOffset);
		PLOTPIXEL_FLIPX( 1, 14, nPaletteOffset);
		PLOTPIXEL_FLIPX( 0, 15, nPaletteOffset);
	}
}

static void Dec0Render16x16Tile_Mask(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nMaskColour, INT32 nPaletteOffset, UINT8 *pTile, INT32 TilemapWidth)
{
	UINT32 nPalette = nTilePalette << nColourDepth;
	pTileData = pTile + (nTileNumber << 8);
	
	UINT16* pPixel = pDestDraw + (StartY * TilemapWidth) + StartX;

	for (INT32 y = 0; y < 16; y++, pPixel += TilemapWidth, pTileData += 16) {
		PLOTPIXEL_MASK( 0, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 1, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 2, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 3, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 4, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 5, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 6, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 7, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 8, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK( 9, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK(10, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK(11, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK(12, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK(13, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK(14, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK(15, nMaskColour, nPaletteOffset);
	}
}

static void Dec0Render16x16Tile_Mask_FlipXY(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nMaskColour, INT32 nPaletteOffset, UINT8 *pTile, INT32 TilemapWidth)
{
	UINT32 nPalette = nTilePalette << nColourDepth;
	pTileData = pTile + (nTileNumber << 8);
	
	UINT16* pPixel = pDestDraw + ((StartY + 15) * TilemapWidth) + StartX;

	for (INT32 y = 15; y >= 0; y--, pPixel -= TilemapWidth, pTileData += 16) {
		PLOTPIXEL_MASK_FLIPX(15,  0, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(14,  1, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(13,  2, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(12,  3, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(11,  4, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX(10,  5, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 9,  6, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 8,  7, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 7,  8, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 6,  9, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 5, 10, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 4, 11, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 3, 12, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 2, 13, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 1, 14, nMaskColour, nPaletteOffset);
		PLOTPIXEL_MASK_FLIPX( 0, 15, nMaskColour, nPaletteOffset);
	}
}

#undef PLOTPIXEL
#undef PLOTPIXEL_FLIPX
#undef PLOTPIXEL_MASK
#undef CLIPPIXEL

#define TILEMAP_BOTH_LAYERS	2
#define TILEMAP_LAYER0		1
#define TILEMAP_LAYER1		0

static void DrvRenderCustomTilemap(UINT16 *pSrc, UINT16 *pControl0, UINT16 *pControl1, UINT16 *RowScrollRam, UINT16 *ColScrollRam, INT32 TilemapWidth, INT32 TilemapHeight, INT32 Opaque, INT32 DrawLayer)
{
	INT32 x, y, xSrc, ySrc, ColOffset = 0, pPixel;
	UINT32 xScroll = pControl1[0];
	UINT32 yScroll = pControl1[1];
	INT32 WidthMask = TilemapWidth - 1;
	INT32 HeightMask = TilemapHeight - 1;
	INT32 RowScrollEnabled = pControl0[0] & 0x04;
	INT32 ColScrollEnabled = pControl0[0] & 0x08;
	
	ySrc = yScroll;
	
	for (y = 0; y < nScreenHeight; y++) {
		if (RowScrollEnabled) {
			xSrc = xScroll + RowScrollRam[(ySrc >> (pControl1[3] & 0x0f)) & (0x1ff >> (pControl1[3] & 0x0f))];
		} else {
			xSrc = xScroll;
		}
		
		xSrc &= TilemapWidth - 1;
		
		if (DrvFlipScreen) xSrc = -xSrc;
		
		for (x = 0; x < nScreenWidth; x++) {
			if (ColScrollEnabled) ColOffset = ColScrollRam[((xSrc >> 3) >> (pControl1[2] & 0x0f)) & (0x3f >> (pControl1[2] & 0x0f))];
			
			pPixel = pSrc[(((ySrc + ColOffset + 8) & HeightMask) * TilemapWidth) + (xSrc & WidthMask)];
			
			xSrc++;
			
			if (Opaque || (pPixel & 0x0f)) {
				if (DrawLayer == TILEMAP_LAYER0) {
					if ((pPixel & 0x88) == 0x88) pTransDraw[(y * nScreenWidth) + x] = pPixel;
				} else {
					pTransDraw[(y * nScreenWidth) + x] = pPixel;
				}
			}
		}
		
		ySrc++;
	}
}

static void DrvRenderTile1Layer(INT32 Opaque, INT32 DrawLayer)
{
	INT32 mx, my, Code, Attr, Colour, x, y, TileIndex, Layer;
	UINT16 *Control0 = (UINT16*)DrvVideo1Ctrl0Ram;
	UINT16 *VideoRam = (UINT16*)DrvVideo1Ram;
	
	INT32 RenderType = Control0[3] & 0x03;
	
	switch (RenderType) {
		case 0x00: {
			DrvTile1TilemapWidth = 1024;
			DrvTile1TilemapHeight = 256;
			break;
		}
		
		case 0x01: {
			DrvTile1TilemapWidth = 512;
			DrvTile1TilemapHeight = 512;
			break;
		}
		
		case 0x02: {
			DrvTile1TilemapWidth = 256;
			DrvTile1TilemapHeight = 1024;
			break;
		}
	}
	
	memset(pTile1LayerDraw, 0, DrvTile1TilemapWidth * DrvTile1TilemapHeight * sizeof(UINT16));
	
	for (my = 0; my < (DrvTile1TilemapHeight / 16); my++) {
		for (mx = 0; mx < (DrvTile1TilemapWidth / 16); mx++) {
			TileIndex = (mx & 0x0f) + ((my & 0x0f) << 4) + ((mx & 0x30) << 4);
			if (RenderType == 1) TileIndex = (mx & 0x0f) + ((my & 0x0f) << 4) + ((my & 0x10) << 4) + ((mx & 0x10) << 5);
			if (RenderType == 2) TileIndex = (mx & 0x0f) + ((my & 0x3f) << 4);
			
			Attr = VideoRam[TileIndex];
			Code = Attr & 0xfff;
			Colour = Attr >> 12;
			Layer = (Attr >> 12) > 7;
			
			if (DrawLayer == TILEMAP_BOTH_LAYERS || DrawLayer == Layer) {
				x = 16 * mx;
				y = 16 * my;
			
				if (Opaque) {
					if (DrvFlipScreen) {
						x = 240 - x;
						y = 240 - y;
						x &= (DrvTile1TilemapWidth - 1);
						y &= (DrvTile1TilemapHeight - 1);
						Dec0Render16x16Tile_FlipXY(pTile1LayerDraw, Code, x, y, Colour, 4, 512, DrvTiles1, DrvTile1TilemapWidth);
					} else {
						Dec0Render16x16Tile(pTile1LayerDraw, Code, x, y, Colour, 4, 512, DrvTiles1, DrvTile1TilemapWidth);
					}
				} else {
					if (DrvFlipScreen) {
						x = 240 - x;
						y = 240 - y;
						x &= (DrvTile1TilemapWidth - 1);
						y &= (DrvTile1TilemapHeight - 1);
						Dec0Render16x16Tile_Mask_FlipXY(pTile1LayerDraw, Code, x, y, Colour, 4, 0, 512, DrvTiles1, DrvTile1TilemapWidth);
					} else {
						Dec0Render16x16Tile_Mask(pTile1LayerDraw, Code, x, y, Colour, 4, 0, 512, DrvTiles1, DrvTile1TilemapWidth);
					}
				}
			}
		}
	}
	
	DrvRenderCustomTilemap(pTile1LayerDraw, (UINT16*)DrvVideo1Ctrl0Ram, (UINT16*)DrvVideo1Ctrl1Ram, (UINT16*)DrvVideo1RowScrollRam, (UINT16*)DrvVideo1ColScrollRam, DrvTile1TilemapWidth, DrvTile1TilemapHeight, Opaque, DrawLayer);
}

static void DrvRenderTile2Layer(INT32 Opaque, INT32 DrawLayer)
{
	INT32 mx, my, Code, Attr, Colour, x, y, TileIndex, Layer;
	UINT16 *Control0 = (UINT16*)DrvVideo2Ctrl0Ram;
	UINT16 *VideoRam = (UINT16*)DrvVideo2Ram;
	
	INT32 RenderType = Control0[3] & 0x03;
	
	switch (RenderType) {
		case 0x00: {
			DrvTile2TilemapWidth = 1024;
			DrvTile2TilemapHeight = 256;
			break;
		}
		
		case 0x01: {
			DrvTile2TilemapWidth = 512;
			DrvTile2TilemapHeight = 512;
			break;
		}
		
		case 0x02: {
			DrvTile2TilemapWidth = 256;
			DrvTile2TilemapHeight = 1024;
			break;
		}
	}
	
	memset(pTile2LayerDraw, 0, DrvTile2TilemapWidth * DrvTile2TilemapHeight * sizeof(UINT16));
	
	for (my = 0; my < (DrvTile2TilemapHeight / 16); my++) {
		for (mx = 0; mx < (DrvTile2TilemapWidth / 16); mx++) {
			TileIndex = (mx & 0x0f) + ((my & 0x0f) << 4) + ((mx & 0x30) << 4);
			if (RenderType == 1) TileIndex = (mx & 0x0f) + ((my & 0x0f) << 4) + ((my & 0x10) << 4) + ((mx & 0x10) << 5);
			if (RenderType == 2) TileIndex = (mx & 0x0f) + ((my & 0x3f) << 4);
			
			Attr = VideoRam[TileIndex];
			Code = Attr & 0xfff;
			Colour = Attr >> 12;
			Layer = (Attr >> 12) > 7;
			
			if (DrawLayer == TILEMAP_BOTH_LAYERS || DrawLayer == Layer) {
				x = 16 * mx;
				y = 16 * my;
			
				if (Opaque) {
					if (DrvFlipScreen) {
						x = 240 - x;
						y = 240 - y;
						x &= (DrvTile2TilemapWidth - 1);
						y &= (DrvTile2TilemapHeight - 1);
						Dec0Render16x16Tile_FlipXY(pTile2LayerDraw, Code, x, y, Colour, 4, 768, DrvTiles2, DrvTile2TilemapWidth);
					} else {
						Dec0Render16x16Tile(pTile2LayerDraw, Code, x, y, Colour, 4, 768, DrvTiles2, DrvTile2TilemapWidth);
					}
				} else {
					if (DrvFlipScreen) {
						x = 240 - x;
						y = 240 - y;
						x &= (DrvTile2TilemapWidth - 1);
						y &= (DrvTile2TilemapHeight - 1);
						Dec0Render16x16Tile_Mask_FlipXY(pTile2LayerDraw, Code, x, y, Colour, 4, 0, 768, DrvTiles2, DrvTile2TilemapWidth);
					} else {
						Dec0Render16x16Tile_Mask(pTile2LayerDraw, Code, x, y, Colour, 4, 0, 768, DrvTiles2, DrvTile2TilemapWidth);
					}
				}
			}
		}
	}
	
	DrvRenderCustomTilemap(pTile2LayerDraw, (UINT16*)DrvVideo2Ctrl0Ram, (UINT16*)DrvVideo2Ctrl1Ram, (UINT16*)DrvVideo2RowScrollRam, (UINT16*)DrvVideo2ColScrollRam, DrvTile2TilemapWidth, DrvTile2TilemapHeight, Opaque, DrawLayer);
}

static void DrvRenderCharLayer()
{
	INT32 mx, my, Code, Attr, Colour, x, y, TileIndex;
	UINT16 *Control0 = (UINT16*)DrvCharCtrl0Ram;
	UINT16 *CharRam = (UINT16*)DrvCharRam;
	
	INT32 RenderType = Control0[3] & 0x03;
	
	switch (RenderType) {
		case 0x00: {
			DrvCharTilemapWidth = 1024;
			DrvCharTilemapHeight = 256;
			break;
		}
		
		case 0x01: {
			DrvCharTilemapWidth = 512;
			DrvCharTilemapHeight = 512;
			break;
		}
		
		case 0x02: {
			DrvCharTilemapWidth = 256;
			DrvCharTilemapHeight = 1024;
			break;
		}
	}
	
	memset(pCharLayerDraw, 0, DrvCharTilemapWidth * DrvCharTilemapHeight * sizeof(UINT16));
	
	for (my = 0; my < (DrvCharTilemapHeight / 8); my++) {
		for (mx = 0; mx < (DrvCharTilemapWidth / 8); mx++) {
			TileIndex = (mx & 0x1f) + ((my & 0x1f) << 5) + ((mx & 0x60) << 5);
			if (RenderType == 1) TileIndex = (mx & 0x1f) + ((my & 0x1f) << 5) + ((my & 0x20) << 5) + ((mx & 0x20) << 6);
			if (RenderType == 2) TileIndex = (mx & 0x1f) + ((my & 0x7f) << 5);
			
			Attr = CharRam[TileIndex];
			Code = Attr & 0xfff;
			Colour = Attr >> 12;
			
			x = 8 * mx;
			y = 8 * my;
			
			if (DrvFlipScreen) {
				x = 248 - x;
				y = 248 - y;
				x &= (DrvCharTilemapWidth - 1);
				y &= (DrvCharTilemapHeight - 1);
				Dec0Render8x8Tile_Mask_FlipXY(pCharLayerDraw, Code, x, y, Colour, 4, 0, 0, DrvChars, DrvCharTilemapWidth);
			} else {
				Dec0Render8x8Tile_Mask(pCharLayerDraw, Code, x, y, Colour, 4, 0, 0, DrvChars, DrvCharTilemapWidth);
			}
		}
	}
	
	DrvRenderCustomTilemap(pCharLayerDraw, (UINT16*)DrvCharCtrl0Ram, (UINT16*)DrvCharCtrl1Ram, (UINT16*)DrvCharRowScrollRam, (UINT16*)DrvCharColScrollRam, DrvCharTilemapWidth, DrvCharTilemapHeight, 0, TILEMAP_BOTH_LAYERS);
}

static void DrvRenderSprites(INT32 PriorityMask, INT32 PriorityVal)
{
	UINT16 *SpriteRam = (UINT16*)DrvSpriteDMABufferRam;
	
	for (UINT32 Offset = 0; Offset < 0x400; Offset += 4) {
		INT32 x, y, Code, Colour, Multi, xFlip, yFlip, Inc, Flash, Mult, yPlot;

		y = SpriteRam[Offset + 0];
		if ((y & 0x8000) == 0) continue;

		x = SpriteRam[Offset + 2];
		Colour = x >> 12;
		if ((Colour & PriorityMask) != PriorityVal) continue;

		Flash= x & 0x800;
		if (Flash && (GetCurrentFrame() & 1)) continue;

		xFlip = y & 0x2000;
		yFlip = y & 0x4000;
		Multi = (1 << ((y & 0x1800) >> 11)) - 1;

		Code = SpriteRam[Offset + 1] & 0x0fff;

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

		if (x > 256) continue;

		Code &= ~Multi;
		if (yFlip) {
			Inc = -1;
		} else {
			Code += Multi;
			Inc = 1;
		}

		if (DrvFlipScreen) {
			y = 240 - y;
			x = 240 - x;
			xFlip = !xFlip;
			yFlip = !yFlip;
			Mult = 16;
		} else {
			Mult = -16;
		}

		while (Multi >= 0) {
			yPlot = y + (Mult * Multi) - 8;
			if (x > 16 && x < (nScreenWidth - 16) && yPlot > 16 && yPlot < (nScreenHeight - 16)) {
				if (xFlip) {
					if (yFlip) {
						Render16x16Tile_Mask_FlipXY(pTransDraw, Code - (Multi * Inc), x, yPlot, Colour, 4, 0, 256, DrvSprites);
					} else {
						Render16x16Tile_Mask_FlipX(pTransDraw, Code - (Multi * Inc), x, yPlot, Colour, 4, 0, 256, DrvSprites);
					}
				} else {
					if (yFlip) {
						Render16x16Tile_Mask_FlipY(pTransDraw, Code - (Multi * Inc), x, yPlot, Colour, 4, 0, 256, DrvSprites);
					} else {
						Render16x16Tile_Mask(pTransDraw, Code - (Multi * Inc), x, yPlot, Colour, 4, 0, 256, DrvSprites);
					}
				}
			} else {
				if (xFlip) {
					if (yFlip) {
						Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, Code - (Multi * Inc), x, yPlot, Colour, 4, 0, 256, DrvSprites);
					} else {
						Render16x16Tile_Mask_FlipX_Clip(pTransDraw, Code - (Multi * Inc), x, yPlot, Colour, 4, 0, 256, DrvSprites);
					}
				} else {
					if (yFlip) {
						Render16x16Tile_Mask_FlipY_Clip(pTransDraw, Code - (Multi * Inc), x, yPlot, Colour, 4, 0, 256, DrvSprites);
					} else {
						Render16x16Tile_Mask_Clip(pTransDraw, Code - (Multi * Inc), x, yPlot, Colour, 4, 0, 256, DrvSprites);
					}
				}
			}
			
			Multi--;
		}
	}
}

static void BaddudesDraw()
{
	UINT16 *Control0 = (UINT16*)DrvCharCtrl0Ram;
	DrvFlipScreen = Control0[0] & 0x80;
	
	BurnTransferClear();
	DrvCalcPalette();
	
	if ((DrvPriority & 0x01) == 0x00) {
		DrvRenderTile1Layer(1, TILEMAP_BOTH_LAYERS);
		DrvRenderTile2Layer(0, TILEMAP_BOTH_LAYERS);
		if (DrvPriority & 0x02) DrvRenderTile1Layer(0, TILEMAP_LAYER0);
		DrvRenderSprites(0, 0);
		if (DrvPriority & 0x04) DrvRenderTile2Layer(0, TILEMAP_LAYER0);
	} else {
		DrvRenderTile2Layer(1, TILEMAP_BOTH_LAYERS);
		DrvRenderTile1Layer(0, TILEMAP_BOTH_LAYERS);
		if (DrvPriority & 0x02) DrvRenderTile2Layer(0, TILEMAP_LAYER0);
		DrvRenderSprites(0, 0);
		if (DrvPriority & 0x04) DrvRenderTile1Layer(0, TILEMAP_LAYER0);
	}	
	
	DrvRenderCharLayer();
	BurnTransferCopy(DrvPalette);
}

static void HbarrelDraw()
{
	UINT16 *Control0 = (UINT16*)DrvCharCtrl0Ram;
	DrvFlipScreen = Control0[0] & 0x80;
	
	BurnTransferClear();
	DrvCalcPalette();
	
	DrvRenderTile2Layer(1, TILEMAP_BOTH_LAYERS);
	DrvRenderSprites(0x08, 0x08);
	DrvRenderTile1Layer(0, TILEMAP_BOTH_LAYERS);
	DrvRenderSprites(0x08, 0x00);
		
	DrvRenderCharLayer();
	BurnTransferCopy(DrvPalette);
}

static void RobocopDraw()
{
	INT32 Trans;
	UINT16 *Control0 = (UINT16*)DrvCharCtrl0Ram;
	DrvFlipScreen = Control0[0] & 0x80;
	
	if (DrvPriority & 0x04) {
		Trans = 0x08;
	} else {
		Trans = 0x00;
	}
	
	BurnTransferClear();
	DrvCalcPalette();
	
	if (DrvPriority & 0x01) {
		DrvRenderTile1Layer(0, TILEMAP_LAYER1);
		if (DrvPriority & 0x02) DrvRenderSprites(0x08, Trans);
		DrvRenderTile2Layer(1, TILEMAP_BOTH_LAYERS);		
	} else {
		DrvRenderTile2Layer(1, TILEMAP_BOTH_LAYERS);
		if (DrvPriority & 0x02) DrvRenderSprites(0x08, Trans);
		DrvRenderTile1Layer(0, TILEMAP_BOTH_LAYERS);
	}
	
	if (DrvPriority & 0x02) {
		DrvRenderSprites(0x08, Trans ^ 0x08);
	} else {
		DrvRenderSprites(0x00, 0x00);
	}
	
	DrvRenderCharLayer();
	BurnTransferCopy(DrvPalette);
}

#undef TILEMAP_BOTH_LAYERS
#undef TILEMAP_LAYER0
#undef TILEMAP_LAYER1

static INT32 DrvFrame()
{
	INT32 nInterleave = 264;

	if (DrvReset) DrvDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = 10000000 / 60;
	nCyclesTotal[1] = 1500000 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;
	
	SekNewFrame();
	M6502NewFrame();

	for (INT32 i = 0; i < nInterleave; i++) {
		INT32 nCurrentCPU;

		nCurrentCPU = 0;
		SekOpen(0);
		BurnTimerUpdate(i * (nCyclesTotal[nCurrentCPU] / nInterleave));
		if (i == 8) DrvVBlank = 0;
		if (i == 248) DrvVBlank = 1;
		if (i == 263) SekSetIRQLine(6, SEK_IRQSTATUS_ACK);
		SekClose();

		nCurrentCPU = 1;
		M6502Open(0);
		BurnTimerUpdateYM3812(i * (nCyclesTotal[nCurrentCPU] / nInterleave));
		M6502Close();
	}
	
	SekOpen(0);
	BurnTimerEndFrame(nCyclesTotal[0]);
	SekClose();
	
	M6502Open(0);
	BurnTimerEndFrameYM3812(nCyclesTotal[1]);
	M6502Close();
	
	if (pBurnSoundOut) {
		SekOpen(0);
		BurnYM2203Update(pBurnSoundOut, nBurnSoundLen);
		SekClose();
		M6502Open(0);
		BurnYM3812Update(pBurnSoundOut, nBurnSoundLen);
		M6502Close();
		MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
	}
	
	if (pBurnDraw && Dec0DrawFunction) Dec0DrawFunction();

	return 0;
}

static INT32 RobocopFrame()
{
	INT32 nInterleave = 264;

	if (DrvReset) RobocopDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = 10000000 / 60;
	nCyclesTotal[1] = 1500000 / 60;
	nCyclesTotal[2] = 1342329 / 60;
	nCyclesDone[0] = nCyclesDone[1] = nCyclesDone[2] = 0;
	
	SekNewFrame();
	M6502NewFrame();

	for (INT32 i = 0; i < nInterleave; i++) {
		INT32 nCurrentCPU, nNext, nCyclesSegment;

		nCurrentCPU = 0;
		SekOpen(0);
		BurnTimerUpdate(i * (nCyclesTotal[nCurrentCPU] / nInterleave));
		if (i == 8) DrvVBlank = 0;
		if (i == 248) DrvVBlank = 1;
		if (i == 263) SekSetIRQLine(6, SEK_IRQSTATUS_ACK);
		SekClose();
		
		nCurrentCPU = 2;
		h6280Open(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = h6280Run(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		h6280Close();

		nCurrentCPU = 1;
		M6502Open(0);
		BurnTimerUpdateYM3812(i * (nCyclesTotal[nCurrentCPU] / nInterleave));
		M6502Close();
	}
	
	SekOpen(0);
	BurnTimerEndFrame(nCyclesTotal[0]);
	SekClose();
	
	M6502Open(0);
	BurnTimerEndFrameYM3812(nCyclesTotal[1]);
	M6502Close();
	
	if (pBurnSoundOut) {
		SekOpen(0);
		BurnYM2203Update(pBurnSoundOut, nBurnSoundLen);
		SekClose();
		M6502Open(0);
		BurnYM3812Update(pBurnSoundOut, nBurnSoundLen);
		M6502Close();
		MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
	}
	
	if (pBurnDraw && Dec0DrawFunction) Dec0DrawFunction();

	return 0;
}

static INT32 DrvScan(INT32 nAction, INT32 *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {			// Return minimum compatible version
		*pnMin = 0x029708;
	}

	if (nAction & ACB_MEMORY_RAM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = RamStart;
		ba.nLen	  = RamEnd-RamStart;
		ba.szName = "All Ram";
		BurnAcb(&ba);
	}

	return 0;
}

struct BurnDriver BurnDrvBaddudes = {
	"baddudes", NULL, NULL, NULL, "1988",
	"Bad Dudes vs. Dragonninja (US)\0", NULL, "Data East USA", "DEC0",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_PRE90S, GBF_SCRFIGHT, 0,
	NULL, BaddudesRomInfo, BaddudesRomName, NULL, NULL, Dec0InputInfo, BaddudesDIPInfo,
	BaddudesInit, DrvExit, DrvFrame, NULL, DrvScan,
	NULL, 0x400, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDrgninja = {
	"drgninja", "baddudes", NULL, NULL, "1988",
	"Dragonninja (Japan)\0", NULL, "Data East Corporation", "DEC0",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_SCRFIGHT, 0,
	NULL, DrgninjaRomInfo, DrgninjaRomName, NULL, NULL, Dec0InputInfo, BaddudesDIPInfo,
	BaddudesInit, DrvExit, DrvFrame, NULL, DrvScan,
	NULL, 0x400, 256, 240, 4, 3
};

struct BurnDriver BurnDrvHbarrel = {
	"hbarrel", NULL, NULL, NULL, "1987",
	"Heavy Barrel (US)\0", NULL, "Data East USA", "DEC0",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_PRE90S, GBF_VERSHOOT, 0,
	NULL, HbarrelRomInfo, HbarrelRomName, NULL, NULL, Dec0InputInfo, HbarrelDIPInfo,
	HbarrelInit, DrvExit, DrvFrame, NULL, DrvScan,
	NULL, 0x400, 240, 256, 3, 4
};

struct BurnDriver BurnDrvHbarrelw = {
	"hbarrelw", "hbarrel", NULL, NULL, "1987",
	"Heavy Barrel (World)\0", NULL, "Data East USA", "DEC0",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_PRE90S, GBF_VERSHOOT, 0,
	NULL, HbarrelwRomInfo, HbarrelwRomName, NULL, NULL, Dec0InputInfo, HbarrelDIPInfo,
	HbarrelInit, DrvExit, DrvFrame, NULL, DrvScan,
	NULL, 0x400, 240, 256, 3, 4
};

struct BurnDriverD BurnDrvHippodrm = {
	"hippodrm", NULL, NULL, NULL, "1989",
	"Hippodrome (US)\0", NULL, "Data East USA", "DEC0",
	NULL, NULL, NULL, NULL,
	0, 2, HARDWARE_MISC_PRE90S, GBF_VSFIGHT, 0,
	NULL, HippodrmRomInfo, HippodrmRomName, NULL, NULL, Dec0InputInfo, HippodrmDIPInfo,
	HippodrmInit, DrvExit, RobocopFrame, NULL, DrvScan,
	NULL, 0x400, 256, 240, 4, 3
};

struct BurnDriver BurnDrvRobocop = {
	"robocop", NULL, NULL, NULL, "1988",
	"Robocop (World revision 4)\0", NULL, "Data East Corporation", "DEC0",
	NULL, NULL, NULL, NULL,
	0, 2, HARDWARE_MISC_PRE90S, GBF_HORSHOOT, 0,
	NULL, RobocopRomInfo, RobocopRomName, NULL, NULL, Dec0InputInfo, RobocopDIPInfo,
	RobocopInit, DrvExit, RobocopFrame, NULL, DrvScan,
	NULL, 0x400, 256, 240, 4, 3
};

struct BurnDriver BurnDrvRobocopb = {
	"robocopb", "robocop", NULL, NULL, "1988",
	"Robocop (World bootleg)\0", NULL, "bootleg", "DEC0",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_BOOTLEG, 2, HARDWARE_MISC_PRE90S, GBF_HORSHOOT, 0,
	NULL, RobocopbRomInfo, RobocopbRomName, NULL, NULL, Dec0InputInfo, RobocopDIPInfo,
	RobocopbInit, DrvExit, DrvFrame, NULL, DrvScan,
	NULL, 0x400, 256, 240, 4, 3
};
