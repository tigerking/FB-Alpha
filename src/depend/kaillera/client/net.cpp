#include <stdio.h>
#include <windows.h>
#include "net.h"

HINSTANCE Kaillera_HDLL;
int Kaillera_Initialised;

int (WINAPI *Kaillera_Get_Version) (char *version);
int (WINAPI *Kaillera_Init) ();
int (WINAPI *Kaillera_Shutdown) ();
int (WINAPI *Kaillera_Set_Infos) (kailleraInfos *infos);
int (WINAPI *Kaillera_Select_Server_Dialog) (HWND parent);
int (WINAPI *Kaillera_Modify_Play_Values) (void *values, int size);
int (WINAPI *Kaillera_Chat_Send) (char *text);
int (WINAPI *Kaillera_End_Game) ();


int WINAPI Empty_Kaillera_Get_Version(char* /*version*/)
{
	return 0;
}

int WINAPI Empty_Kaillera_Init()
{
	return 0;
}

int WINAPI Empty_Kaillera_Shutdown()
{
	return 0;
}

int WINAPI Empty_Kaillera_Set_Infos(kailleraInfos* /*infos*/)
{
	return 0;
}

int WINAPI Empty_Kaillera_Select_Server_Dialog(HWND /*parent*/)
{
	return 0;
}

int WINAPI Empty_Kaillera_Modify_Play_Values(void* /*values*/, int /*size*/)
{
	return 0;
}

int WINAPI Empty_Kaillera_Chat_Send(char* /*text*/)
{
	return 0;
}

int WINAPI Empty_Kaillera_End_Game()
{
	return 0;
}


int Init_Network(void)
{
	return 0;
}


void End_Network(void)
{

}
