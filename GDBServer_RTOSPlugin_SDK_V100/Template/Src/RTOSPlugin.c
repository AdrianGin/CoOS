/*********************************************************************
*                SEGGER MICROCONTROLLER SYSTEME GmbH                 *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (C) 2004-2009    SEGGER Microcontroller Systeme GmbH        *
*                                                                    *
*      Internet: www.segger.com    Support:  support@segger.com      *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File        : RTOSPlugin.c
Purpose     : Extracts information about tasks from RTOS.

Additional information:
  Eclipse based debuggers show information about threads.

---------------------------END-OF-HEADER------------------------------
*/

#include "RTOSPlugin.h"
#include "JLINKARM_Const.h"
//#include <stdio.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#ifdef WIN32
  #define EXPORT __declspec(dllexport)
#else
  #define EXPORT __attribute__((visibility("default")))
#endif

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
static const GDB_API* _pAPI;
#define PLUGIN_VERSION             100

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static RTOS_SYMBOLS RTOS_Symbols[] = {
  { "pool",								0, 0 },
  { "_currentTask",              0, 0 },
  { "_nextTask",                 0, 0 },
  { "CoOS::RoundRobin::count",   0, 0 },
  { NULL, 0, 0 }
};

enum RTOS_Symbol_Values {
	pool = 0,
	_currentTask,
	_nextTask,
	count,
};

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

EXPORT int RTOS_Init(const GDB_API *pAPI, U32 core) {
	_pAPI = pAPI;
	if ((core == JLINK_CORE_CORTEX_M0)
		|| (core == JLINK_CORE_CORTEX_M1)
		|| (core == JLINK_CORE_CORTEX_M3)
		|| (core == JLINK_CORE_CORTEX_M4)
		|| (core == JLINK_CORE_CORTEX_M7)
		) {
		return 1;
	}
	return 0;
}

EXPORT U32 RTOS_GetVersion() {
  return PLUGIN_VERSION;
}

EXPORT RTOS_SYMBOLS* RTOS_GetSymbols() {
  return RTOS_Symbols;
}

EXPORT U32 RTOS_GetNumThreads() {
  return 0;
}

EXPORT U32 RTOS_GetCurrentThreadId() {
  return 0;
}

EXPORT U32 RTOS_GetThreadId(U32 n) {
  return 0;
}

EXPORT int RTOS_GetThreadDisplay(char *pDisplay, U32 threadid) {
  return 0;
}

EXPORT int RTOS_GetThreadReg(char *pHexRegVal, U32 RegIndex, U32 threadid) {
  return -1;
}

EXPORT int RTOS_GetThreadRegList(char *pHexRegList, U32 threadid) {
  return -1;
}

EXPORT int RTOS_SetThreadReg(char* pHexRegVal, U32 RegIndex, U32 threadid) {
  return -1;
}

EXPORT int RTOS_SetThreadRegList(char *pHexRegList, U32 threadid) {
  return -1;
}


typedef struct {
	U32   threadid;     // thread ID
} THREAD_DETAIL;

static struct {
	//const STACKING* StackingInfo;
	U32              CurrentThread;
	U32              ThreadCount;
	U32              uThreadDetails;
	THREAD_DETAIL* pThreadDetails;
} _OS;

/*********************************************************************
*
*       _FreeThreadlist()
*
*  Function description
*    Frees the thread list
*/
static void _FreeThreadlist() {
	U32 i;

	if (_OS.pThreadDetails) {
		for (i = 0; i < _OS.ThreadCount; i++) {
		}
		_OS.pThreadDetails = NULL;
		_OS.uThreadDetails = 0;
		_OS.ThreadCount = 0;
	}
}

EXPORT int RTOS_UpdateThreads() {

	U32  retval;
	U32  tcb;
	U32  TasksFound;
	U32  ThreadListSize;

	//
// Free previous thread details
//
	_FreeThreadlist();
	//_StackMem.ThreadID = 0;


	//
	// Read the current thread
	//
	retval = _pAPI->pfReadU32(RTOS_Symbols[_currentTask].address, &_OS.CurrentThread);
	if (retval != 0) {
		_pAPI->pfErrorOutf("Error reading current task.\n");
		return retval;
	}
	_pAPI->pfDebugOutf("Read current task at 0x%08X, value 0x%08X.\n", RTOS_Symbols[_currentTask].address, _OS.CurrentThread);


  return 0;
}
