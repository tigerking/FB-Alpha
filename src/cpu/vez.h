// Nec V20/V30/V33 interface

#define V33_TYPE 0
#define V30_TYPE 8
#define V20_TYPE 16
#define V25_TYPE (V20_TYPE|(1<<16))
#define V35_TYPE (V30_TYPE|(1<<16))

unsigned char cpu_readmem20(unsigned int a);
void cpu_writemem20(unsigned int a, unsigned char d);

extern int nVezCount;

unsigned int VezTotalCycles();
void VezNewFrame();
void VezRunEnd();
void VezIdle(int cycles);

int VezInit(int nCPU, int type, int clock); // v20/v25/v30/v33/v35
int VezInit(int cpu, int type); // v20/v30/v33 only
void VezExit();
void VezOpen(int nCPU);
void VezClose();
int VezGetActive();

void VezSetDecode(unsigned char *decode); // set opcode decode

int VezMemCallback(int nStart,int nEnd,int nMode);
int VezMapArea(int nStart, int nEnd, int nMode, unsigned char *Mem);
int VezMapArea(int nStart, int nEnd, int nMode, unsigned char *Mem1, unsigned char *Mem2);

void VezSetReadHandler(unsigned char (__fastcall *)(unsigned int));
void VezSetWriteHandler(void (__fastcall *)(unsigned int, unsigned char));

#define V25_PORT_P0 0x10000
#define V25_PORT_P1 0x10002
#define V25_PORT_P2 0x10004
#define V25_PORT_PT 0x10006

void VezSetReadPort(unsigned char (__fastcall *)(unsigned int));
void VezSetWritePort(void (__fastcall *)(unsigned int, unsigned char));
void VezSetIrqCallBack(int (*cb)(int));

int VezReset();
int VezPc(int n);
int VezScan(int nAction);

int VezRun(int nCycles);

#define NEC_INPUT_LINE_INTP0 10
#define NEC_INPUT_LINE_INTP1 11
#define NEC_INPUT_LINE_INTP2 12
#define NEC_INPUT_LINE_POLL  20

#define VEZ_IRQSTATUS_NONE 0
#define VEZ_IRQSTATUS_ACK  1
//#define VEZ_IRQSTATUS_AUTO 2 // unimplemented

void VezSetIRQLineAndVector(const int line, const int vector, const int status);

