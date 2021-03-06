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
#include <stdio.h>
#include <stdint.h>

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
#define PLUGIN_VERSION             101

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
  { "_isRunning",   0, 0 },
  { "CoOS::RoundRobin::count",   0, 0 },
  { NULL, 0, 0 }
};

enum RTOS_Symbol_Values {
	pool = 0,
	_currentTask,
	_nextTask,
	_isRunning,
	eThreadCount,
};


typedef struct {
	const char* regName;
	signed short   offset;
	unsigned short bits;
} STACK_REGS;

static const STACK_REGS _CortexM0StackOffsets[] = {
  { "R0", 0x20, 32 },    // R0
  { "R1", 0x24, 32 },    // R1
  { "R2", 0x28, 32 },    // R2
  { "R3", 0x2C, 32 },    // R3
  { "R4", 0x1C, 32 },    // R4
  { "R5", 0x18, 32 },    // R5
  { "R6", 0x14, 32 },    // R6
  { "R7", 0x10, 32 },    // R7
  { "R8", 0x0C, 32 },    // R8
  { "R9", 0x08, 32 },    // R9
  { "R10", 0x04, 32 },    // R10
  { "R11", 0x00, 32 },    // R11
  { "R12", 0x30, 32 },    // R12
  { "SP", -2,   32 },    // SP
  { "LR", 0x34, 32 },    // LR
  { "PC", 0x38, 32 },    // PC
  { "XPSR", 0x3C, 32 },    // XPSR
  { "MSP", -1,   32 },    // MSP
  { "PSP", -1,   32 },    // PSP
  { "PRIMASK", -1,   32 },    // PRIMASK
  { "BASEPRI", -1,   32 },    // BASEPRI
  { "FAULTMASK", -1,   32 },    // FAULTMASK
  { "CONTROL", -1,   32 },    // CONTROL
};

static const STACK_REGS _CortexM4StackOffsets[] = {
  { "R0", 0x20, 32 },    // R0
  { "R1", 0x24, 32 },    // R1
  { "R2", 0x28, 32 },    // R2
  { "R3", 0x2C, 32 },    // R3
  { "R4", 0x00, 32 },    // R4
  { "R5", 0x04, 32 },    // R5
  { "R6", 0x08, 32 },    // R6
  { "R7", 0x0C, 32 },    // R7
  { "R8", 0x10, 32 },    // R8
  { "R9", 0x14, 32 },    // R9
  { "R10", 0x18, 32 },    // R10
  { "R11", 0x1C, 32 },    // R11
  { "R12", 0x30, 32 },    // R12
  { "SP", -2,   32 },    // SP
  { "LR", 0x34, 32 },    // LR
  { "PC", 0x38, 32 },    // PC
  { "XPSR", 0x3C, 32 },    // XPSR
  { "MSP", -1,   32 },    // MSP
  { "PSP", -1,   32 },    // PSP
  { "PRIMASK", -1,   32 },    // PRIMASK
  { "BASEPRI", -1,   32 },    // BASEPRI
  { "FAULTMASK", -1,   32 },    // FAULTMASK
  { "CONTROL", -1,   32 },    // CONTROL
};

typedef struct {
	uint32_t rx[11 - 4 + 1]; //PSP = R4  - R11
	uint32_t r[5]; //R0-R3, R12
	uint32_t lr;   //R14
	uint32_t pc;   //Return Addr
	uint32_t xpsr; //Highest Memory
} Context;

typedef struct {
	U32   psp;
	U32   threadid;     // thread ID
	Context context;
} THREAD_DETAIL;

typedef struct _Stacking {
	const STACK_REGS* RegisterOffsets;
	int RegisterCount;
} STACKING;

typedef struct {
	U8  Data[0xD0];     // stack data, maximum possible stack size
	U32 Pointer;        // stack pointer
	U32 ThreadID;       // thread ID
} STACK_MEM;

static struct {
	STACKING			  StackingInfo;
	U32              CurrentThread;
	U32              ThreadCount;
	U8					  IsActive;
	U32              uThreadDetails;
	THREAD_DETAIL* pThreadDetails;
} _OS;

static STACK_MEM _StackMem;
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

	_pAPI->pfLogOutf("RTOS_Init Core ID: %d\n", core);

	if ((core == JLINK_CORE_CORTEX_M0)
		|| (core == JLINK_CORE_CORTEX_M1)
		|| (core == JLINK_CORE_CORTEX_M3)
		|| (core == JLINK_CORE_CORTEX_M4)
		|| (core == JLINK_CORE_CORTEX_M7)
		) {

		_OS.CurrentThread = 0;
		_OS.ThreadCount = 0;
		_OS.StackingInfo.RegisterOffsets = _CortexM4StackOffsets;
		_OS.StackingInfo.RegisterCount = 17;
		_OS.IsActive = 0;
		_pAPI->pfLogOutf(" << RTOS_Init\n");

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
	_pAPI->pfLogOutf("RTOS_GetNumThreads %d\n", _OS.ThreadCount);
  return _OS.ThreadCount;
}

EXPORT U32 RTOS_GetCurrentThreadId() {
  _pAPI->pfLogOutf("RTOS_GetCurrentThreadId 0x%x\n", _OS.CurrentThread);
  return _OS.CurrentThread;
}

EXPORT U32 RTOS_GetThreadId(U32 n) {
	_pAPI->pfLogOutf("RTOS_GetThreadId %d\n", n);
  return _OS.pThreadDetails[n].threadid;
}


EXPORT int RTOS_GetThreadDisplay(char *pDisplay, U32 threadid) {
   snprintf(pDisplay, 256, "CoOS Thread 0x%x", threadid);
   return strlen(pDisplay);
}


/*********************************************************************
*
*       _ReadStack(U32 threadid)
*
*  Function description
*    Reads the task stack of the task with the ID threadid into _StackMem.
*/
static int _ReadStack(U32 threadid) {
	uint32_t regVal = 0;
	U32 idx;
	for (idx = 0; idx < _OS.ThreadCount; idx++) {
		if (_OS.pThreadDetails[idx].threadid == threadid) {
			break;
		}
	}
	if (idx == _OS.ThreadCount) {
		_pAPI->pfLogOutf("Task not found.\n");
		return -2;
	}


	_pAPI->pfLogOutf(">> _ReadStack :: psp[%d]=0x%x\n", idx, _OS.pThreadDetails[idx].psp);

	U32 retval = _pAPI->pfReadU32(_OS.pThreadDetails[idx].psp, &regVal);
	if (retval != 0) {
		_pAPI->pfLogOutf("Error reading stack frame from OS.\n");
		return retval;
	}

	_StackMem.Pointer = regVal;
	_pAPI->pfLogOutf(">> _ReadStack :: StackFrame[%d]=0x%x\n", idx, _StackMem.Pointer);

	for (uint8_t i = 0; i < sizeof(Context); i++) {
		retval = _pAPI->pfReadU8(_StackMem.Pointer + i, (char*)&_StackMem.Data[i]);
		if (retval != 0) {
			_pAPI->pfLogOutf("Error reading stack frame from task.\n");
			return retval;
		}
	}

	if (threadid != _OS.CurrentThread) {
		_StackMem.Pointer = regVal + 0x40;
	}
	else {
		//_StackMem.Pointer = regVal + 0x20;
	}

	Context* context = &_StackMem.Data;

	if (context->xpsr & (1 << 9)) {
		_StackMem.Pointer += 4;
		_pAPI->pfLogOutf(">> _ReadStack :: Adjusting alignment\n");
	}

	_StackMem.ThreadID = threadid;
	return retval;
}

EXPORT int RTOS_GetThreadReg(char *pHexRegVal, U32 RegIndex, U32 threadid) {

	U32 retval;
	_pAPI->pfLogOutf(">> RTOS_GetThreadReg :: Thread[0x%x] Reg[0x%x]\n", threadid, RegIndex);
	_pAPI->pfLogOutf(">> RTOS_GetThreadReg :: Current = 0x%x\n", _OS.CurrentThread);

	if ( threadid == _OS.CurrentThread || _OS.CurrentThread <= 1) {

		_pAPI->pfLogOutf("<< RTOS_GetThreadReg :: Return Direct CPU\n");
		return -1; // Current thread or current execution returns CPU registers
	}
	else {
		//
		// load stack memory if necessary
		//
				//> XPSR
		if (RegIndex > 25) {
			return -1;
		}
		if (_StackMem.ThreadID != threadid) {
			if (_ReadStack(threadid))
			{
				_pAPI->pfLogOutf("<< RTOS_GetThreadReg :: _ReadStack failed\n");
				return -1;
			}
		}
		 
		int offset = _OS.StackingInfo.RegisterOffsets[RegIndex].offset;
		uint32_t regVal = 0;
		I32 j;
		if (RegIndex > 0x16 )
		{
			for (j = 0; j < _OS.StackingInfo.RegisterOffsets[RegIndex].bits / 8; j++) {
				if (_OS.StackingInfo.RegisterOffsets[RegIndex].offset == -1) {
					_pAPI->pfLogOutf(">> RTOS_GetThreadReg Reg[%s] = 0x%02x\n", _OS.StackingInfo.RegisterOffsets[RegIndex].regName, 0);
					pHexRegVal += snprintf(pHexRegVal, 3, "%02x", 0);
				}
				else if (_OS.StackingInfo.RegisterOffsets[RegIndex].offset == -2) {
					_pAPI->pfLogOutf(">> RTOS_GetThreadReg :: SP = 0x%02x\n", _StackMem.Pointer + 32);
					pHexRegVal += snprintf(pHexRegVal, 3, "%02x", ((U8 *)&_StackMem.Pointer + 32) [j]);
				}
				else {
					pHexRegVal += snprintf(pHexRegVal, 3, "%02x",
						_StackMem.Data[_OS.StackingInfo.RegisterOffsets[RegIndex].offset + j]);

					uint32_t* data = &_StackMem.Data[_OS.StackingInfo.RegisterOffsets[RegIndex].offset];
					_pAPI->pfLogOutf(">> RTOS_GetThreadReg Reg[%s] = 0x%02x\n", _OS.StackingInfo.RegisterOffsets[RegIndex].regName, *data);
				}
			}
			return 0;
		}
	}

	
	return -1;
}

EXPORT int RTOS_GetThreadRegList(char *pHexRegList, U32 threadid) {
	U32 i;
	I32 j;
	int retval;

	static int lastThreadId = 0;

	_pAPI->pfLogOutf(">> RTOS_GetThreadRegList :: Req=0x%x, Cur=0x%x\n", threadid, _OS.CurrentThread);


	if (threadid == 0 || threadid == _OS.CurrentThread) {
		_pAPI->pfLogOutf("<< threadid == _OS.CurrentThread, 0x%x = 0x%x\n", threadid, _OS.CurrentThread);
		return -1; // Current thread or current execution returns CPU registers
	}

	if (_OS.pThreadDetails == 0) {
		UpdateThreads();
	}
	//
	// load stack memory if necessary
	//
	if (_StackMem.ThreadID != threadid) 
	{
		retval = _ReadStack(threadid);
		if (retval != 0) {
			_pAPI->pfLogOutf("<< RTOS_GetThreadRegList Failed\n");
			return retval;
		}
	}


	for (i = 0; i < _OS.StackingInfo.RegisterCount; i++) {
		for (j = 0; j < _OS.StackingInfo.RegisterOffsets[i].bits / 8; j++) {
			if (_OS.StackingInfo.RegisterOffsets[i].offset == -1) {
				pHexRegList += snprintf(pHexRegList, 3, "%02x", 0);
			}
			else if (_OS.StackingInfo.RegisterOffsets[i].offset == -2) {
				pHexRegList += snprintf(pHexRegList, 3, "%02x", ((U8*)&_StackMem.Pointer)[j]);
			}
			else {
				pHexRegList += snprintf(pHexRegList, 3, "%02x",
					_StackMem.Data[_OS.StackingInfo.RegisterOffsets[i].offset + j]);
			}
		}

		if (_OS.StackingInfo.RegisterOffsets[i].offset > 0) {
			uint32_t* data = &_StackMem.Data[_OS.StackingInfo.RegisterOffsets[i].offset];
			_pAPI->pfLogOutf("Reg[%s] = 0x%08x\n", _OS.StackingInfo.RegisterOffsets[i].regName, *data);
		} else if (_OS.StackingInfo.RegisterOffsets[i].offset == -2) {
			_pAPI->pfLogOutf("Reg[%s] = 0x%08x\n", _OS.StackingInfo.RegisterOffsets[i].regName, _StackMem.Pointer);
		} 
	}

	_pAPI->pfLogOutf("<< RTOS_GetThreadRegList\n");


  return 0;
}

EXPORT int RTOS_SetThreadReg(char* pHexRegVal, U32 RegIndex, U32 threadid) {
	_pAPI->pfLogOutf("<< RTOS_SetThreadReg\n");
	if (threadid == _OS.CurrentThread) {
		return -1; // Current thread or current execution return CPU registers
	}

  return 0;
}

EXPORT int RTOS_SetThreadRegList(char *pHexRegList, U32 threadid) {
	_pAPI->pfLogOutf("<< RTOS_SetThreadRegList\n");

	if ( threadid == _OS.CurrentThread) {
		return -1; // Current thread or current execution return CPU registers
	}

  return 0;
}


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
		_pAPI->pfFree(_OS.pThreadDetails);
		_OS.pThreadDetails = NULL;
		_OS.uThreadDetails = 0;
		_OS.ThreadCount = 0;
	}
}

/*********************************************************************
*
*       _AllocThreadlist(int count)
*
*  Function description
*    Allocates a thread list for count entries.
*/
static void _AllocThreadlist(int count) {
	_OS.pThreadDetails = (THREAD_DETAIL*)_pAPI->pfAlloc(count * sizeof(THREAD_DETAIL));
	memset(_OS.pThreadDetails, 0, count * sizeof(THREAD_DETAIL));
	_OS.uThreadDetails = count;
}


int UpdateThreads() {
	U32  retval;
	U32  tcb;
	U32  TasksFound;
	U32  ThreadListSize;

	//
// Free previous thread details
//
	_FreeThreadlist();
	_StackMem.ThreadID = 0;
	_AllocThreadlist(10);

	//
	// Read the current thread
	//
	uint32_t currentTaskIdx;
	retval = _pAPI->pfReadU32(RTOS_Symbols[_currentTask].address, &currentTaskIdx);
	_OS.CurrentThread = RTOS_Symbols[pool].address + (currentTaskIdx * 4);
	if (retval != 0) {
		_pAPI->pfLogOutf("Error reading current task.\n");
		return retval;
	}

	_pAPI->pfLogOutf("UpdateThreads:: Read current task (%d), value 0x%08X.\n", currentTaskIdx, _OS.CurrentThread);

	//Get Thread Count
	retval = _pAPI->pfReadU8(RTOS_Symbols[_isRunning].address, &_OS.IsActive);
	if (!_OS.IsActive) {

		_OS.pThreadDetails[0].threadid = 0x00000001;
		_OS.CurrentThread = 0x00000001;
		_OS.ThreadCount = 1;

		_pAPI->pfLogOutf("UpdateThreads:: OS Not Active\n");
		return 0;
	}

	retval = _pAPI->pfReadU32(RTOS_Symbols[eThreadCount].address, &_OS.ThreadCount);
	if (retval != 0) {
		_pAPI->pfLogOutf("Error reading thread count.\n");
		return retval;
	}
	_pAPI->pfLogOutf("RTOS_UpdateThreads:: Thread count, Addr 0x%x, %d.\n", RTOS_Symbols[eThreadCount].address, _OS.ThreadCount);

	uint32_t poolAdr = RTOS_Symbols[pool].address;
	_pAPI->pfLogOutf("RTOS_UpdateThreads:: Pool Addr 0x%0x.\n", RTOS_Symbols[pool].address);

	//Read in psp for each thread.
	for (uint8_t i = 0; i < _OS.ThreadCount; i++) {
		retval = _pAPI->pfReadU32(poolAdr, &_OS.pThreadDetails[i].psp);
		if (retval != 0) {
			_pAPI->pfLogOutf("Error reading thread PSP.\n");
			return retval;
		}

		_OS.pThreadDetails[i].threadid = poolAdr;
		_pAPI->pfLogOutf("RTOS_UpdateThreads:: Pool Addr 0x%0x, PSP[%i] = 0x%0x.\n", poolAdr, i, _OS.pThreadDetails[i].psp);
		poolAdr += 4;
	}


	return 0;
}

EXPORT int RTOS_UpdateThreads() {
	return UpdateThreads();
}
