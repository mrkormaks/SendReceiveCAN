
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <WinIoCtl.h>
#include <string.h>
#include <iostream>
#include <conio.h>
//#include "AdvCan.h"
//#include "AdvCANIO.h"
DWORD WINAPI ThreadMethod( LPVOID ) ;
//void WriteToProjBrainEXE();
//#include "../HeaderExtern/Main_struct_IBKO.h"
#include "../HeaderExtern/STRUCT_AVN_410_RTSH_4.h"
//#include "../HeaderExtern/Main_struct_cabine.h"
//#include "../HeaderExtern/struct_410_b8.h"
//#include "../HeaderExtern/Struct_SUT_410.h"
extern bool close_all;

extern LONG LtikCAN;
//extern SHMAIN_B8*                     b8;
extern SHAVN_410*                     AVN;
extern SHMAIN_STRUCT_CABINE*          cab;
//extern SHMAIN_STRUCT_IN*              ibko;

//----------------------------------
//
int  CreateCANThread()
{
	 HANDLE hThread;
	 DWORD dwThread;

	 hThread= CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)ThreadMethod, 0, 0, &dwThread);
	 if(!hThread)
	 {
	     //MessageBox(NULL,TEXT("Íå ñîçäàí ïîòîê ââîäà!"),TEXT("ÎØÈÁÊÀ"), MB_ICONWARNING | MB_OK );
		 return -1;
	 }
	// else
        // Pc->iReadyProcessRTSS=1;// Ready
	 SetThreadPriority(hThread,THREAD_PRIORITY_NORMAL);//THREAD_PRIORITY_ABOVE_NORMALTHREAD_PRIORITY_HIGHESTTHREAD_PRIORITY_TIME_CRITICAL
	 return 1;
}