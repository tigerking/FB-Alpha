// Nec V20/V30/V33/V25/V35 interface
// Written by OopsWare
// http://oopsware.googlepages.com
// Heavily modified by iq_132 (Nov, 2011)

#include "burnint.h"
#include "vez.h"

//----------------------------------------------------------------------------------
// nec.cpp
void necInit(int cpu, int type);
void necCpuOpen(int cpu);
void necCpuClose();
int nec_reset();
int nec_execute(int cycles);
void nec_set_irq_line_and_vector(int irqline, int vector, int state);
unsigned int nec_total_cycles();
void nec_new_frame();
int necGetPC(int n);
void necScan(int cpu, int nAction);
void necRunEnd();
void necIdle(int cycles);

// v25.cpp
int v25_reset();
void v25_open(int cpu);
void v25_close();
void v25_set_irq_line_and_vector(int irqline, int vector, int state);
int v25_execute(int cycles);
void v25Init(int cpu, int type, int clock);
void v25_set_decode(unsigned char *table);
unsigned int v25_total_cycles();
void v25_new_frame();
int v25GetPC(int n);
void v25Scan(int cpu, int nAction);
void v25RunEnd();
void v25Idle(int cycles);

//----------------------------------------------------------------------------------

struct VezContext {
	void (*cpu_open)(int);
	void (*cpu_close)();
	int (*cpu_reset)();
	int (*cpu_execute)(int);
	void (*cpu_set_irq_line)(int, int, int);
	void (*decode)(unsigned char*);
	unsigned int (*total_cycles)();
	int (*get_pc)(int);
	void (*scan)(int, int);
	void (*runend)();
	void (*idle)(int);

	unsigned char * ppMemRead[512];
	unsigned char * ppMemWrite[512];
	unsigned char * ppMemFetch[512];
	unsigned char * ppMemFetchData[512];

	// Handlers
 #ifdef FASTCALL
	unsigned char (__fastcall *ReadHandler)(unsigned int a);
	void (__fastcall *WriteHandler)(unsigned int a, unsigned char d);
	unsigned char (__fastcall *ReadPort)(unsigned int a);
	void (__fastcall *WritePort)(unsigned int a, unsigned char d);
 #else
	unsigned char (__cdecl *ReadHandler)(unsigned int a);
	void (__cdecl *WriteHandler)(unsigned int a, unsigned char d);
	unsigned char (__cdecl *ReadPort)(unsigned int a);
	void (__cdecl *WritePort)(unsigned int a, unsigned char d);
 #endif
};

static struct VezContext VezCPUContext[4];
struct VezContext *VezCurrentCPU = 0;

#define VEZ_MEM_SHIFT	11
#define VEZ_MEM_MASK	((1 << VEZ_MEM_SHIFT) - 1)

static int nCPUCount = 0;
static int nOpenedCPU = -1;
int nVezCount;

unsigned char __fastcall VezDummyReadHandler(unsigned int) { return 0; }
void __fastcall VezDummyWriteHandler(unsigned int, unsigned char) { }
unsigned char __fastcall VezDummyReadPort(unsigned int) { return 0; }
void __fastcall VezDummyWritePort(unsigned int, unsigned char) { }

unsigned char cpu_readport(unsigned int p)
{
	p &= 0x100ff; // ?

	return VezCurrentCPU->ReadPort(p);
}

void cpu_writeport(unsigned int p,unsigned int d)
{
	VezCurrentCPU->WritePort(p, d);
}

unsigned char cpu_readmem20(unsigned int a)
{
	a &= 0xFFFFF;
	
	unsigned char * p = VezCurrentCPU->ppMemRead[ a >> VEZ_MEM_SHIFT ];
	if ( p )
		return *(p + a);
	else
		return VezCurrentCPU->ReadHandler(a);
}

unsigned char cpu_readmem20_op(unsigned int a)
{
	a &= 0xFFFFF;
	
	unsigned char * p = VezCurrentCPU->ppMemFetch[ a >> VEZ_MEM_SHIFT ];
	if ( p )
		return *(p + a);
	else
		return VezCurrentCPU->ReadHandler(a);
}

unsigned char cpu_readmem20_arg(unsigned int a)
{
	a &= 0xFFFFF;
	
	unsigned char * p = VezCurrentCPU->ppMemFetchData[ a >> VEZ_MEM_SHIFT ];
	if ( p )
		return *(p + a);
	else
		return VezCurrentCPU->ReadHandler(a);
}

void cpu_writemem20(unsigned int a, unsigned char d)
{
	a &= 0xFFFFF;
	
	unsigned char * p = VezCurrentCPU->ppMemWrite[ a >> VEZ_MEM_SHIFT ];
	if ( p )
		*(p + a) = d;
	else
		VezCurrentCPU->WriteHandler(a, d);
}

void VezSetReadHandler(unsigned char (__fastcall *pHandler)(unsigned int))
{
	VezCurrentCPU->ReadHandler = pHandler;
}

void VezSetWriteHandler(void (__fastcall *pHandler)(unsigned int, unsigned char))
{
	VezCurrentCPU->WriteHandler = pHandler;
}

void VezSetReadPort(unsigned char (__fastcall *pHandler)(unsigned int))
{
	VezCurrentCPU->ReadPort = pHandler;
}

void VezSetWritePort(void (__fastcall *pHandler)(unsigned int, unsigned char))
{
	VezCurrentCPU->WritePort = pHandler;
}

void VezSetDecode(unsigned char *table)
{
	if (VezCurrentCPU->decode) {
		VezCurrentCPU->decode(table);
	}
}

int VezInit(int cpu, int type, int clock)
{
	nOpenedCPU = cpu;
	VezCurrentCPU = &VezCPUContext[cpu];

	memset(VezCurrentCPU, 0, sizeof(struct VezContext));

	switch (type)
	{
		case V20_TYPE:
		case V30_TYPE:
		case V33_TYPE:
		{
			necInit(cpu, type);

			VezCurrentCPU->cpu_open = necCpuOpen;
			VezCurrentCPU->cpu_close = necCpuClose;
			VezCurrentCPU->cpu_reset = nec_reset;
			VezCurrentCPU->cpu_execute = nec_execute;
			VezCurrentCPU->cpu_set_irq_line = nec_set_irq_line_and_vector;
			VezCurrentCPU->decode = NULL; // ?
			VezCurrentCPU->total_cycles = nec_total_cycles;
			VezCurrentCPU->get_pc = necGetPC;
			VezCurrentCPU->scan = necScan;
			VezCurrentCPU->runend = necRunEnd;
			VezCurrentCPU->idle = necIdle;
		}
		break;

		case V25_TYPE:
		case V35_TYPE:
		{
			v25Init(cpu, type&0xff, clock);

			VezCurrentCPU->cpu_open = v25_open;
			VezCurrentCPU->cpu_close = v25_close;
			VezCurrentCPU->cpu_reset = v25_reset;
			VezCurrentCPU->cpu_execute = v25_execute;
			VezCurrentCPU->cpu_set_irq_line = v25_set_irq_line_and_vector;
			VezCurrentCPU->decode = v25_set_decode;
			VezCurrentCPU->total_cycles = v25_total_cycles;
			VezCurrentCPU->get_pc = v25GetPC;
			VezCurrentCPU->scan = v25Scan;
			VezCurrentCPU->runend = v25RunEnd;
			VezCurrentCPU->idle = v25Idle;

		}
		break;
	}
		
	VezCurrentCPU->ReadHandler = VezDummyReadHandler;
	VezCurrentCPU->WriteHandler = VezDummyWriteHandler;
	VezCurrentCPU->ReadPort = VezDummyReadPort;
	VezCurrentCPU->WritePort = VezDummyWritePort;

	int nCount = nVezCount+1;

	nVezCount = nCPUCount = nCount;

	CpuCheatRegister(0x0001, cpu);

	return 0;
}

int VezInit(int cpu, int type)
{
	return VezInit(cpu, type, 0);
}

void VezExit()
{
	nCPUCount = 0;
	nOpenedCPU = -1;
	nVezCount = 0;

	nOpenedCPU = -1;
}

void VezOpen(int nCPU)
{
	nOpenedCPU = nCPU;
	VezCurrentCPU = &VezCPUContext[nCPU];
	VezCurrentCPU->cpu_open(nCPU);
}

void VezClose()
{
	nOpenedCPU = -1;
	VezCurrentCPU->cpu_close();
	VezCurrentCPU = 0;
}

void VezNewFrame()
{
	// should be separated?
	v25_new_frame();
	nec_new_frame();
}

void VezRunEnd()
{
	VezCurrentCPU->runend();
}

void VezIdle(int cycles)
{
	VezCurrentCPU->idle(cycles);
}

unsigned int VezTotalCycles()
{
	return VezCurrentCPU->total_cycles();
}

int VezGetActive()
{
	return nOpenedCPU;
}

int VezMemCallback(int nStart,int nEnd,int nMode)
{
	nStart >>= VEZ_MEM_SHIFT;
	nEnd += VEZ_MEM_MASK;
	nEnd >>= VEZ_MEM_SHIFT;

	for (int i = nStart; i < nEnd; i++) {
		switch (nMode) {
			case 0:
				VezCurrentCPU->ppMemRead[i] = NULL;
				break;
			case 1:
				VezCurrentCPU->ppMemWrite[i] = NULL;
				break;
			case 2:
				VezCurrentCPU->ppMemFetch[i] = NULL;
				VezCurrentCPU->ppMemFetchData[i] = NULL;
				break;
		}
	}
	return 0;
}

int VezMapArea(int nStart, int nEnd, int nMode, unsigned char *Mem)
{
	int s = nStart >> VEZ_MEM_SHIFT;
	int e = (nEnd + VEZ_MEM_MASK) >> VEZ_MEM_SHIFT;

	for (int i = s; i < e; i++) {
		switch (nMode) {
			case 0:
				VezCurrentCPU->ppMemRead[i] = Mem - nStart;
				break;
			case 1:
				VezCurrentCPU->ppMemWrite[i] = Mem - nStart;
				break;
			case 2:
				VezCurrentCPU->ppMemFetch[i] = Mem - nStart;
				VezCurrentCPU->ppMemFetchData[i] = Mem - nStart;
				break;
		}
	}

	return 0;
}

int VezMapArea(int nStart, int nEnd, int nMode, unsigned char *Mem1, unsigned char *Mem2)
{
	int s = nStart >> VEZ_MEM_SHIFT;
	int e = (nEnd + VEZ_MEM_MASK) >> VEZ_MEM_SHIFT;
	
	if (nMode != 2) return 1;
	
	for (int i = s; i < e; i++) {
		VezCurrentCPU->ppMemFetch[i] = Mem1 - nStart;
		VezCurrentCPU->ppMemFetchData[i] = Mem2 - nStart;
	}
	return 0;
}

int VezReset()
{
	return VezCurrentCPU->cpu_reset();
}

int VezRun(int nCycles)
{
	if (nCycles <= 0) return 0;

	return VezCurrentCPU->cpu_execute(nCycles);
}

int VezPc(int n)
{
	if (n == -1) {
		return VezCurrentCPU->get_pc(-1);
	} else {
		struct VezContext *CPU = &VezCPUContext[n];
		return CPU->get_pc(n);
	}

	return 0;
}

int VezScan(int nAction)
{
	if ((nAction & ACB_DRIVER_DATA) == 0)
		return 0;

	for (int i = 0; i < nCPUCount; i++) {
		struct VezContext *CPU = &VezCPUContext[i];
		if (CPU->scan) {
			CPU->scan(i, nAction);
		}
	}
	
	return 0;
}

void VezSetIRQLineAndVector(const int line, const int vector, const int status)
{
	VezCurrentCPU->cpu_set_irq_line(line, vector, status);
}
