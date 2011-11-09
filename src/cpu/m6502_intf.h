#include "m6502.h"

typedef unsigned char (*pReadPortHandler)(unsigned short a);
typedef void (*pWritePortHandler)(unsigned short a, unsigned char d);
typedef unsigned char (*pReadByteHandler)(unsigned short a);
typedef void (*pWriteByteHandler)(unsigned short a, unsigned char d);
typedef unsigned char (*pReadMemIndexHandler)(unsigned short a);
typedef void (*pWriteMemIndexHandler)(unsigned short a, unsigned char d);
typedef unsigned char (*pReadOpHandler)(unsigned short a);
typedef unsigned char (*pReadOpArgHandler)(unsigned short a);

struct M6502Ext {

	m6502_Regs reg;
	
	int (*execute)(int cycles);
	void (*reset)();
	void (*init)();
	void (*set_irq_line)(int irqline, int state);

	unsigned char* pMemMap[0x100 * 3];

	pReadPortHandler ReadPort;
	pWritePortHandler WritePort;
	pReadByteHandler ReadByte;
	pWriteByteHandler WriteByte;
	pReadMemIndexHandler ReadMemIndex;
	pWriteMemIndexHandler WriteMemIndex;
	pReadOpHandler ReadOp;
	pReadOpArgHandler ReadOpArg;
	
	int nCyclesTotal;
	int nCyclesSegment;
	int nCyclesLeft;
};

#define M6502_IRQSTATUS_NONE	0
#define M6502_IRQSTATUS_ACK	1
#define M6502_IRQSTATUS_AUTO	2

#define M6502_READ		1
#define M6502_WRITE	2
#define M6502_FETCH	4

#define M6502_RAM	(M6502_READ | M6502_WRITE | M6502_FETCH)
#define M6502_ROM	(M6502_READ | M6502_FETCH)

extern int nM6502Count;

extern int nM6502CyclesTotal;

void M6502Reset();
void M6502NewFrame();

void n2a03_irq(void); // USED FOR PSG!!

// use these at addresses 0 and 1 for M6510, M6510T, M7501, M8502 map!
unsigned char m6510_read_0000(unsigned short offset);
void m6510_write_0000(unsigned short address, unsigned char data);

// The M6504 only has 13 address bits! use address mirroring!

enum { TYPE_M6502=0, TYPE_M6504, TYPE_M65C02, TYPE_M65SC02, TYPE_N2A03, TYPE_DECO16,
//	 these are the same!
	TYPE_M6510, TYPE_M6510T, TYPE_M7501, TYPE_M8502	 };

int M6502Init(int cpu, int type); // if you're using more than one type
void M6502Exit();
void M6502Open(int num);
void M6502Close();
int M6502GetActive();
void M6502SetIRQ(int vector, int status);
int M6502Run(int cycles);
void M6502RunEnd();
int M6502MapMemory(unsigned char* pMemory, unsigned short nStart, unsigned short nEnd, int nType);
void M6502SetReadPortHandler(unsigned char (*pHandler)(unsigned short));
void M6502SetWritePortHandler(void (*pHandler)(unsigned short, unsigned char));
void M6502SetReadByteHandler(unsigned char (*pHandler)(unsigned short));
void M6502SetWriteByteHandler(void (*pHandler)(unsigned short, unsigned char));
void M6502SetReadMemIndexHandler(unsigned char (*pHandler)(unsigned short));
void M6502SetWriteMemIndexHandler(void (*pHandler)(unsigned short, unsigned char));
void M6502SetReadOpHandler(unsigned char (*pHandler)(unsigned short));
void M6502SetReadOpArgHandler(unsigned char (*pHandler)(unsigned short));
int M6502Scan(int nAction);

UINT32 M6502GetPC();

void M6502WriteRom(unsigned short Address, unsigned char Data);

inline static int M6502TotalCycles()
{
	return nM6502CyclesTotal;
}
