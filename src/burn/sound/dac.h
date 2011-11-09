void DACUpdate(INT16* Buffer, INT32 Length);
void DACWrite(UINT8 Data);
void DACSignedWrite(UINT8 Data);
void DACInit(INT32 Clock, INT32 bAdd);
void DACSetVolShift(INT32 nShift);
void DACReset();
void DACExit();
INT32 DACScan(INT32 nAction,INT32 *pnMin);
