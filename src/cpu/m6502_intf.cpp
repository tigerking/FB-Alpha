#include "burnint.h"
#include "m6502_intf.h"

#define MAX_CPU		8

int nM6502Count = 0;
static int nActiveCPU = 0;

static M6502Ext m6502CPUContext[MAX_CPU];
static M6502Ext *pCurrentCPU;
static int nM6502CyclesDone[MAX_CPU];
int nM6502CyclesTotal;

static unsigned char M6502ReadPortDummyHandler(unsigned short)
{
	return 0;
}

static void M6502WritePortDummyHandler(unsigned short, unsigned char)
{
}

static unsigned char M6502ReadByteDummyHandler(unsigned short)
{
	return 0;
}

static void M6502WriteByteDummyHandler(unsigned short, unsigned char)
{
}

static unsigned char M6502ReadMemIndexDummyHandler(unsigned short)
{
	return 0;
}

static void M6502WriteMemIndexDummyHandler(unsigned short, unsigned char)
{
}

static unsigned char M6502ReadOpDummyHandler(unsigned short)
{
	return 0;
}

static unsigned char M6502ReadOpArgDummyHandler(unsigned short)
{
	return 0;
}

void M6502Reset()
{
	pCurrentCPU->reset();
}

void M6502NewFrame()
{
	for (int i = 0; i < nM6502Count; i++) {
		nM6502CyclesDone[i] = 0;
	}
	nM6502CyclesTotal = 0;
}

int M6502Init(int cpu, int type)
{
	nActiveCPU = cpu;
	nM6502Count++;

	pCurrentCPU = &m6502CPUContext[cpu];

	memset(pCurrentCPU, 0, sizeof(M6502Ext));

	switch (type)
	{
		case TYPE_M6502:
		case TYPE_M6504:
			pCurrentCPU->execute = m6502_execute;
			pCurrentCPU->reset = m6502_reset;
			pCurrentCPU->init = m6502_init;
			pCurrentCPU->set_irq_line = m6502_set_irq_line;
		break;

		case TYPE_M65C02:
			pCurrentCPU->execute = m65c02_execute;
			pCurrentCPU->reset = m65c02_reset;
			pCurrentCPU->init = m65c02_init;
			pCurrentCPU->set_irq_line = m65c02_set_irq_line;
		break;

		case TYPE_DECO16:
			pCurrentCPU->execute = deco16_execute;
			pCurrentCPU->reset = deco16_reset;
			pCurrentCPU->init = deco16_init;
			pCurrentCPU->set_irq_line = deco16_set_irq_line;
		break;

		case TYPE_N2A03:
			pCurrentCPU->execute = m6502_execute;
			pCurrentCPU->reset = m6502_reset;
			pCurrentCPU->init = n2a03_init;		// different
			pCurrentCPU->set_irq_line = m6502_set_irq_line;
		break;

		case TYPE_M65SC02:
			pCurrentCPU->execute = m65c02_execute;
			pCurrentCPU->reset = m65c02_reset;
			pCurrentCPU->init = m65sc02_init;	// different
			pCurrentCPU->set_irq_line = m65c02_set_irq_line;
		break;

		case TYPE_M6510:
		case TYPE_M6510T:
		case TYPE_M7501:
		case TYPE_M8502:
			pCurrentCPU->execute = m6502_execute;
			pCurrentCPU->reset = m6510_reset;	// different
			pCurrentCPU->init = m6510_init;		// different
			pCurrentCPU->set_irq_line = m6502_set_irq_line;
		break;
	}

	pCurrentCPU->ReadPort = M6502ReadPortDummyHandler;
	pCurrentCPU->WritePort = M6502WritePortDummyHandler;
	pCurrentCPU->ReadByte = M6502ReadByteDummyHandler;
	pCurrentCPU->WriteByte = M6502WriteByteDummyHandler;
	pCurrentCPU->ReadMemIndex = M6502ReadMemIndexDummyHandler;
	pCurrentCPU->WriteMemIndex = M6502WriteMemIndexDummyHandler;
	pCurrentCPU->ReadOp = M6502ReadOpDummyHandler;
	pCurrentCPU->ReadOpArg = M6502ReadOpArgDummyHandler;
	
	nM6502CyclesDone[cpu] = 0;
	
	for (int j = 0; j < (0x0100 * 3); j++) {
		pCurrentCPU->pMemMap[j] = NULL;
	}
	
	nM6502CyclesTotal = 0;
	
	pCurrentCPU->init();

	CpuCheatRegister(0x0003, cpu);

	return 0;
}

void M6502Exit()
{
	nM6502Count = 0;
}

void M6502Open(int num)
{
	nActiveCPU = num;

	pCurrentCPU = &m6502CPUContext[num];

	m6502_set_context(&pCurrentCPU->reg);
	
	nM6502CyclesTotal = nM6502CyclesDone[nActiveCPU];
}

void M6502Close()
{
	m6502_get_context(&pCurrentCPU->reg);
	
	nM6502CyclesDone[nActiveCPU] = nM6502CyclesTotal;

	pCurrentCPU = NULL; // cause problems! 
	
	nActiveCPU = -1;
}

int M6502GetActive()
{
	return nActiveCPU;
}

void M6502SetIRQ(int vector, int status)
{
	if (status == M6502_IRQSTATUS_NONE) {
		pCurrentCPU->set_irq_line(vector, 0);
	}
	
	if (status == M6502_IRQSTATUS_ACK) {
		pCurrentCPU->set_irq_line(vector, 1);
	}
	
	if (status == M6502_IRQSTATUS_AUTO) {
		pCurrentCPU->set_irq_line(vector, 1);
		pCurrentCPU->execute(0);
		pCurrentCPU->set_irq_line(vector, 0);
		pCurrentCPU->execute(0);
	}
}

int M6502Run(int cycles)
{
	cycles = pCurrentCPU->execute(cycles);
	
	nM6502CyclesTotal += cycles;
	
	return cycles;
}

void M6502RunEnd()
{

}

int M6502MapMemory(unsigned char* pMemory, unsigned short nStart, unsigned short nEnd, int nType)
{
	unsigned char cStart = (nStart >> 8);
	unsigned char **pMemMap = pCurrentCPU->pMemMap;

	for (unsigned short i = cStart; i <= (nEnd >> 8); i++) {
		if (nType & M6502_READ)	{
			pMemMap[0     + i] = pMemory + ((i - cStart) << 8);
		}
		if (nType & M6502_WRITE) {
			pMemMap[0x100 + i] = pMemory + ((i - cStart) << 8);
		}
		if (nType & M6502_FETCH) {
			pMemMap[0x200 + i] = pMemory + ((i - cStart) << 8);
		}
	}
	return 0;

}

void M6502SetReadPortHandler(unsigned char (*pHandler)(unsigned short))
{
	pCurrentCPU->ReadPort = pHandler;
}

void M6502SetWritePortHandler(void (*pHandler)(unsigned short, unsigned char))
{
	pCurrentCPU->WritePort = pHandler;
}

void M6502SetReadByteHandler(unsigned char (*pHandler)(unsigned short))
{
	pCurrentCPU->ReadByte = pHandler;
}

void M6502SetWriteByteHandler(void (*pHandler)(unsigned short, unsigned char))
{
	pCurrentCPU->WriteByte = pHandler;
}

void M6502SetReadMemIndexHandler(unsigned char (*pHandler)(unsigned short))
{
	pCurrentCPU->ReadMemIndex = pHandler;
}

void M6502SetWriteMemIndexHandler(void (*pHandler)(unsigned short, unsigned char))
{
	pCurrentCPU->WriteMemIndex = pHandler;
}

void M6502SetReadOpHandler(unsigned char (*pHandler)(unsigned short))
{
	pCurrentCPU->ReadOp = pHandler;
}

void M6502SetReadOpArgHandler(unsigned char (*pHandler)(unsigned short))
{
	pCurrentCPU->ReadOpArg = pHandler;
}

unsigned char M6502ReadPort(unsigned short Address)
{
	// check handler
	if (pCurrentCPU->ReadPort != NULL) {
		return pCurrentCPU->ReadPort(Address);
	}
	
	return 0;
}

void M6502WritePort(unsigned short Address, unsigned char Data)
{
	// check handler
	if (pCurrentCPU->WritePort != NULL) {
		pCurrentCPU->WritePort(Address, Data);
		return;
	}
}

unsigned char M6502ReadByte(unsigned short Address)
{
	// check mem map
	unsigned char * pr = pCurrentCPU->pMemMap[0x000 | (Address >> 8)];
	if (pr != NULL) {
		return pr[Address & 0xff];
	}
	
	// check handler
	if (pCurrentCPU->ReadByte != NULL) {
		return pCurrentCPU->ReadByte(Address);
	}
	
	return 0;
}

void M6502WriteByte(unsigned short Address, unsigned char Data)
{
	// check mem map
	unsigned char * pr = pCurrentCPU->pMemMap[0x100 | (Address >> 8)];
	if (pr != NULL) {
		pr[Address & 0xff] = Data;
		return;
	}
	
	// check handler
	if (pCurrentCPU->WriteByte != NULL) {
		pCurrentCPU->WriteByte(Address, Data);
		return;
	}
}

unsigned char M6502ReadMemIndex(unsigned short Address)
{
	// check mem map
	unsigned char * pr = pCurrentCPU->pMemMap[0x000 | (Address >> 8)];
	if (pr != NULL) {
		return pr[Address & 0xff];
	}
	
	// check handler
	if (pCurrentCPU->ReadMemIndex != NULL) {
		return pCurrentCPU->ReadMemIndex(Address);
	}
	
	return 0;
}

void M6502WriteMemIndex(unsigned short Address, unsigned char Data)
{
	// check mem map
	unsigned char * pr = pCurrentCPU->pMemMap[0x100 | (Address >> 8)];
	if (pr != NULL) {
		pr[Address & 0xff] = Data;
		return;
	}
	
	// check handler
	if (pCurrentCPU->WriteMemIndex != NULL) {
		pCurrentCPU->WriteMemIndex(Address, Data);
		return;
	}
}

unsigned char M6502ReadOp(unsigned short Address)
{
	// check mem map
	unsigned char * pr = pCurrentCPU->pMemMap[0x200 | (Address >> 8)];
	if (pr != NULL) {
		return pr[Address & 0xff];
	}
	
	// check handler
	if (pCurrentCPU->ReadOp != NULL) {
		return pCurrentCPU->ReadOp(Address);
	}
	
	return 0;
}

unsigned char M6502ReadOpArg(unsigned short Address)
{
	// check mem map
	unsigned char * pr = pCurrentCPU->pMemMap[0x000 | (Address >> 8)];
	if (pr != NULL) {
		return pr[Address & 0xff];
	}
	
	// check handler
	if (pCurrentCPU->ReadOpArg != NULL) {
		return pCurrentCPU->ReadOpArg(Address);
	}
	
	return 0;
}

void M6502WriteRom(unsigned short Address, unsigned char Data)
{
	unsigned char * pr = pCurrentCPU->pMemMap[0x000 | (Address >> 8)];
	unsigned char * pw = pCurrentCPU->pMemMap[0x100 | (Address >> 8)];
	unsigned char * pf = pCurrentCPU->pMemMap[0x200 | (Address >> 8)];

	if (pr != NULL) {
		pr[Address & 0xff] = Data;
	}
	
	if (pw != NULL) {
		pw[Address & 0xff] = Data;
	}

	if (pf != NULL) {
		pf[Address & 0xff] = Data;
	}

	// check handler
	if (pCurrentCPU->WriteByte != NULL) {
		pCurrentCPU->WriteByte(Address, Data);
		return;
	}
}

UINT32 M6502GetPC()
{
	return m6502_get_pc();
}

int M6502Scan(int nAction)
{
	struct BurnArea ba;
	
	if ((nAction & ACB_DRIVER_DATA) == 0) {
		return 1;
	}

	for (int i = 0; i < nM6502Count; i++) {

		M6502Ext *ptr = &m6502CPUContext[i];

		int (*Callback)(int irqline);

		Callback = ptr->reg.irq_callback;

		char szName[] = "M6502 #n";
		szName[7] = '0' + i;

		ba.Data = &m6502CPUContext[i].reg;
		ba.nLen = sizeof(m6502CPUContext[i].reg);
		ba.szName = szName;
		BurnAcb(&ba);

		// necessary?
		SCAN_VAR(ptr->nCyclesTotal);
		SCAN_VAR(ptr->nCyclesSegment);
		SCAN_VAR(ptr->nCyclesLeft);

		ptr->reg.irq_callback = Callback;
	}
	
	return 0;
}
