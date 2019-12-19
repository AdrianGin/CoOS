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

#define EMBOS_PLUGIN_VERSION             100

#define EMBOS_THREAD_NAME_SIZE           200

#define EMBOS_CURRENT_TASK_OFFSET          8
#define EMBOS_FIRST_TASK_OFFSET           12

#define EMBOS_TCB_NEXT_OFFSET              0
#define EMBOS_TCB_STACK_OFFSET             4
#define EMBOS_TCB_STAT_OFFSET             16
#define EMBOS_TCB_PRIO_OFFSET             20
#define EMBOS_TCB_NAME_OFFSET             36

#define THREAD_LIST_CHUNK                 10

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/

typedef struct {
  U32   threadid;     // thread ID
  U8    prio;         // thread priority
  char *sThreadName;  // thread name
  char *sThreadState; // thread state
} THREAD_DETAIL;

typedef struct {
  U8  Data[0xD0];     // stack data, maximum possible stack size
  U32 Pointer;        // stack pointer
  U32 ThreadID;       // thread ID
} STACK_MEM;

typedef struct {
  char *Running;
  char *Suspended;
  char *Waiting;
} TASK_STATE;

typedef struct {
  signed short   offset;
  unsigned short bits;
} STACK_REGS;

typedef struct _Stacking {
  unsigned char     RegistersSize;
  signed char       GrowthDirection;
  unsigned char     OutputRegisters;
  U32             (*CalcProcessStack) (const struct _Stacking *Stacking, const U8 *StackData, U32 StackPtr);
  const STACK_REGS *RegisterOffsets;
} STACKING;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static const GDB_API *_pAPI;

static STACK_MEM _StackMem;

static struct {
  const STACKING *StackingInfo;
  U32              CurrentThread;
  U32              ThreadCount;
  U32              uThreadDetails;
  THREAD_DETAIL   *pThreadDetails;
} _embOS;

static const STACK_REGS _CortexM4FStackOffsets[] = {
  { 0x28, 32 },    // R0
  { 0x2C, 32 },    // R1
  { 0x30, 32 },    // R2
  { 0x34, 32 },    // R3
  { 0x04, 32 },    // R4
  { 0x08, 32 },    // R5
  { 0x0C, 32 },    // R6
  { 0x10, 32 },    // R7
  { 0x14, 32 },    // R8
  { 0x18, 32 },    // R9
  { 0x1C, 32 },    // R10
  { 0x20, 32 },    // R11
  { 0x38, 32 },    // R12
  { -2,   32 },    // SP
  { 0x3C, 32 },    // LR
  { 0x40, 32 },    // PC
  { 0x44, 32 },    // XPSR
  { -1,   32 },    // MSP
  { -1,   32 },    // PSP
  { -1,   32 },    // PRIMASK
  { -1,   32 },    // BASEPRI
  { -1,   32 },    // FAULTMASK
  { -1,   32 },    // CONTROL
};

static const STACK_REGS _CortexM4FStackOffsetsVFP[] = {
  { 0x68, 32 },    // R0
  { 0x6C, 32 },    // R1
  { 0x70, 32 },    // R2
  { 0x74, 32 },    // R3
  { 0x04, 32 },    // R4
  { 0x08, 32 },    // R5
  { 0x0C, 32 },    // R6
  { 0x10, 32 },    // R7
  { 0x14, 32 },    // R8
  { 0x18, 32 },    // R9
  { 0x1C, 32 },    // R10
  { 0x20, 32 },    // R11
  { 0x78, 32 },    // R12
  { -2,   32 },    // SP
  { 0x7C, 32 },    // LR
  { 0x80, 32 },    // PC
  { 0x84, 32 },    // XPSR
  { -1,   32 },    // MSP
  { -1,   32 },    // PSP
  { -1,   32 },    // PRIMASK
  { -1,   32 },    // BASEPRI
  { -1,   32 },    // FAULTMASK
  { -1,   32 },    // CONTROL
  { 0xC8, 32 },    // FPSCR
  { 0x88, 32 },    // S0
  { 0x8C, 32 },    // S1
  { 0x90, 32 },    // S2
  { 0x94, 32 },    // S3
  { 0x98, 32 },    // S4
  { 0x9C, 32 },    // S5
  { 0xA0, 32 },    // S6
  { 0xA4, 32 },    // S7
  { 0xA8, 32 },    // S8
  { 0xAC, 32 },    // S9
  { 0xB0, 32 },    // S10
  { 0xB4, 32 },    // S11
  { 0xB8, 32 },    // S12
  { 0xBC, 32 },    // S13
  { 0xC0, 32 },    // S14
  { 0xC4, 32 },    // S15
  { 0x28, 32 },    // S16
  { 0x2C, 32 },    // S17
  { 0x30, 32 },    // S18
  { 0x34, 32 },    // S19
  { 0x38, 32 },    // S20
  { 0x3C, 32 },    // S21
  { 0x40, 32 },    // S22
  { 0x44, 32 },    // S23
  { 0x48, 32 },    // S24
  { 0x4C, 32 },    // S25
  { 0x50, 32 },    // S26
  { 0x54, 32 },    // S27
  { 0x58, 32 },    // S28
  { 0x5C, 32 },    // S29
  { 0x60, 32 },    // S30
  { 0x64, 32 },    // S31
};

static const TASK_STATE _TaskState = {
  "Running",
  "Suspended",
  "Waiting"
};

static const char _TaskCurrentExecution[] = {
  "Current Execution"
};

static RTOS_SYMBOLS RTOS_Symbols[] = {
  { "OS_Global",                 0, 0 },
  { "OS_Switch",                 1, 0 },
  { "OS_Switch_End",             1, 0 },
  { NULL, 0, 0 }
};

enum RTOS_Symbol_Values {
  embOS_Global = 0,
  embOS_Switch,
  embOS_Switch_End
};

static U32 _DoCortexMStackAlign(const STACKING *stacking, const U8 *StackData, U32 StackPtr, size_t XPSROffset) {
  const U32 ALIGN_NEEDED = (1 << 9);
  U32 xpsr;
  U32 NewStackPtr;

  NewStackPtr = StackPtr - stacking->GrowthDirection * stacking->RegistersSize;
  xpsr = _pAPI->pfLoad32TE(&StackData[XPSROffset]);
  if ((xpsr & ALIGN_NEEDED) != 0) {
    _pAPI->pfDebugOutf("XPSR(0x%08X) indicated stack alignment was necessary.\n", xpsr);
    NewStackPtr -= (stacking->GrowthDirection * 4);
  }
  return NewStackPtr;
}

static U32 _CortexM4FStackAlign(const STACKING *stacking, const U8 *StackData, U32 StackPtr) {
  const int XPSROffset = 0x44;
  return _DoCortexMStackAlign(stacking, StackData, StackPtr, XPSROffset);
}

static U32 _CortexM4FStackAlignVFP(const STACKING *stacking, const U8 *StackData, U32 StackPtr) {
  const int XPSROffset = 0x84;
  return _DoCortexMStackAlign(stacking, StackData, StackPtr, XPSROffset);
}

static const STACKING _CortexM4FStacking = {
  0x48,                         // RegistersSize
  -1,                           // GrowthDirection
  17,                           // OutputRegisters
  _CortexM4FStackAlign,         // stack_alignment
  _CortexM4FStackOffsets        // RegisterOffsets
};

static const STACKING _CortexM4FStackingVFP = {
  0xD0,                         // RegistersSize
  -1,                           // GrowthDirection
  17,                           // OutputRegisters
  _CortexM4FStackAlignVFP,      // stack_alignment
  _CortexM4FStackOffsetsVFP     // RegisterOffsets
};

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _AllocThreadlist(int count)
*
*  Function description
*    Allocates a thread list for count entries.
*/
static void _AllocThreadlist(int count) {
  _embOS.pThreadDetails = (THREAD_DETAIL*)_pAPI->pfAlloc(count * sizeof(THREAD_DETAIL));
  memset(_embOS.pThreadDetails, 0, count * sizeof(THREAD_DETAIL));
  _embOS.uThreadDetails = count;
}

/*********************************************************************
*
*       _ReallocThreadlist(int count)
*
*  Function description
*    Reallocates the thread list to count entries.
*/
static void _ReallocThreadlist(int count) {
  int growth;

  growth = count - _embOS.uThreadDetails;
  _embOS.pThreadDetails = (THREAD_DETAIL*)_pAPI->pfRealloc(_embOS.pThreadDetails, count * sizeof(THREAD_DETAIL));
  memset(&_embOS.pThreadDetails[_embOS.uThreadDetails], 0, growth * sizeof(THREAD_DETAIL));
  _embOS.uThreadDetails = count;
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

  if (_embOS.pThreadDetails) {
    for (i = 0; i < _embOS.ThreadCount; i++) {
      _pAPI->pfFree(_embOS.pThreadDetails[i].sThreadName);
    }
    _pAPI->pfFree(_embOS.pThreadDetails);
    _embOS.pThreadDetails = NULL;
    _embOS.uThreadDetails = 0;
    _embOS.ThreadCount = 0;
  }
}

/*********************************************************************
*
*       _ReadStack(U32 threadid)
*
*  Function description
*    Reads the task stack of the task with the ID threadid into _StackMem.
*/
static int _ReadStack(U32 threadid) {
  U32 retval;
  U32 i;
  U32 task;
  U32 StackPtr;
  U32 PC;
  U32 address;
  //
  // search for thread ID
  //
  task = 0;
  for (i = 0; i < _embOS.ThreadCount; i++) {
    if (_embOS.pThreadDetails[i].threadid == threadid) {
      task = i;
      goto found;
      break;
    }
  }
  _pAPI->pfErrorOutf("Task not found.\n");
  return -2;

found:
  retval = _pAPI->pfReadU32(_embOS.pThreadDetails[task].threadid + EMBOS_TCB_STACK_OFFSET, &StackPtr);
  if (retval != 0) {
    _pAPI->pfErrorOutf("Error reading stack frame from embOS task.\n");
    return retval;
  }
  _pAPI->pfDebugOutf("Read stack pointer at 0x%08X, value 0x%08X.\n",
                    _embOS.pThreadDetails[task].threadid + EMBOS_TCB_STACK_OFFSET,
                    StackPtr);
  if (StackPtr == 0) {
    _pAPI->pfErrorOutf("Null stack pointer in task.\n");
    return -3;
  }
  _embOS.StackingInfo = &_CortexM4FStacking;

start:
  address = StackPtr;

  if (_embOS.StackingInfo->GrowthDirection == 1)
    address -= _embOS.StackingInfo->RegistersSize;
  retval = _pAPI->pfReadMem(address, (char*)_StackMem.Data, _embOS.StackingInfo->RegistersSize);
  if (retval == 0) {
    _pAPI->pfErrorOutf("Error reading stack frame from task.\n");
    return retval;
  }
  _pAPI->pfDebugOutf("Read stack frame at 0x%08X.\n", address);
  retval = _pAPI->pfLoad32TE(&_StackMem.Data[0x24]);
  if (_embOS.StackingInfo == &_CortexM4FStacking &&
      !(retval & 0x10)) {
    _pAPI->pfDebugOutf("LR(0x%08X) indicated task uses VFP, reading stack frame again.\n", retval);
    _embOS.StackingInfo = &_CortexM4FStackingVFP;
    goto start;
  }
  //
  // calculate stack pointer
  //
  if (_embOS.StackingInfo->CalcProcessStack != NULL) {
    _StackMem.Pointer = _embOS.StackingInfo->CalcProcessStack(_embOS.StackingInfo,
      _StackMem.Data, StackPtr);
  } else {
    _StackMem.Pointer = StackPtr - _embOS.StackingInfo->GrowthDirection *
      _embOS.StackingInfo->RegistersSize;
  }
  //
  // Check for cooperative task switch
  //
  if (RTOS_Symbols[embOS_Switch].address && RTOS_Symbols[embOS_Switch_End].address) {
    PC = _pAPI->pfLoad32TE(&_StackMem.Data[_CortexM4FStackOffsets[15].offset]);
    if (PC >= RTOS_Symbols[embOS_Switch].address && PC <= RTOS_Symbols[embOS_Switch_End].address) {
      //
      // Replace PC by LR and "pop" 8 bytes from stack
      //
      *(U32 *)&_StackMem.Data[_CortexM4FStackOffsets[15].offset] = *(U32 *)&_StackMem.Data[_CortexM4FStackOffsets[14].offset];
      _StackMem.Pointer += 8;
    }
  }
  _StackMem.ThreadID = threadid;
  return 0;
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

EXPORT int RTOS_Init(const GDB_API *pAPI, U32 core) {
  _pAPI = pAPI;
  memset(&_embOS, 0, sizeof(_embOS));
  if (   (core == JLINK_CORE_CORTEX_M0)
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
  return EMBOS_PLUGIN_VERSION;
}

EXPORT RTOS_SYMBOLS* RTOS_GetSymbols() {
  return RTOS_Symbols;
}

EXPORT U32 RTOS_GetNumThreads() {
  return _embOS.ThreadCount;
}

EXPORT U32 RTOS_GetCurrentThreadId() {
  return _embOS.CurrentThread;
}

EXPORT U32 RTOS_GetThreadId(U32 index) {
  return _embOS.pThreadDetails[index].threadid;
}

EXPORT int RTOS_GetThreadDisplay(char *pDisplay, U32 threadid) {
  const U32 reserved = 256;
  U32 i;
  U32 size;

  if (_embOS.ThreadCount) {
    for (i = 0; i < _embOS.ThreadCount; i++) {
      if (_embOS.pThreadDetails[i].threadid == threadid) {
        size = 0;
        if (_embOS.pThreadDetails[i].sThreadName) {
          size += snprintf(pDisplay, reserved, "%s", _embOS.pThreadDetails[i].sThreadName);
        }
        if (_embOS.pThreadDetails[i].sThreadState) {
          if (size != 0) {
            size += snprintf(pDisplay + size, reserved - size, " : ");
          }
          size += snprintf(pDisplay + size, reserved - size, "%s", _embOS.pThreadDetails[i].sThreadState);
        }
        if (threadid != 1 && _embOS.pThreadDetails[i].prio) {
          size += snprintf(pDisplay + size, reserved - size, " [P: %d]", _embOS.pThreadDetails[i].prio);
        }
        return size;
      }
    }
  }
  return 0;
}

EXPORT int RTOS_GetThreadReg(char *pHexRegVal, U32 RegIndex, U32 threadid) {
  int retval;
  I32 j;

  if (threadid == 1 || threadid == _embOS.CurrentThread) {
    return -1; // Current thread or current execution returns CPU registers
  }

  //
  // load stack memory if necessary
  //
  if (_StackMem.ThreadID != threadid) {
    retval = _ReadStack(threadid);
    if (retval != 0) {
      return retval;
    }
  }

  if (RegIndex > 0x16 && _embOS.StackingInfo == &_CortexM4FStackingVFP) {
    for (j = 0; j < _embOS.StackingInfo->RegisterOffsets[RegIndex].bits/8; j++) {
      if (_embOS.StackingInfo->RegisterOffsets[RegIndex].offset == -1) {
        pHexRegVal += snprintf(pHexRegVal, 3, "%02x", 0);
      } else if (_embOS.StackingInfo->RegisterOffsets[RegIndex].offset == -2) {
        pHexRegVal += snprintf(pHexRegVal, 3, "%02x", ((U8 *)&_StackMem.Pointer)[j]);
      } else {
        pHexRegVal += snprintf(pHexRegVal, 3, "%02x",
          _StackMem.Data[_embOS.StackingInfo->RegisterOffsets[RegIndex].offset + j]);
      }
    }
    _pAPI->pfDebugOutf("Read task register 0x%02X, addr 0x%08X.\n", RegIndex, _embOS.StackingInfo->RegisterOffsets[RegIndex].offset);
    return 0;
  } else {
    return -1;
  }
}

EXPORT int RTOS_GetThreadRegList(char *pHexRegList, U32 threadid) {
  int retval;
  U32 i;
  I32 j;

  if (threadid == 1 || threadid == _embOS.CurrentThread) {
    return -1; // Current thread or current execution returns CPU registers
  }

  //
  // load stack memory if necessary
  //
  if (_StackMem.ThreadID != threadid) {
    retval = _ReadStack(threadid);
    if (retval != 0) {
      return retval;
    }
  }

  for (i = 0; i < _embOS.StackingInfo->OutputRegisters; i++) {
    for (j = 0; j < _embOS.StackingInfo->RegisterOffsets[i].bits/8; j++) {
      if (_embOS.StackingInfo->RegisterOffsets[i].offset == -1) {
        pHexRegList += snprintf(pHexRegList, 3, "%02x", 0);
      } else if (_embOS.StackingInfo->RegisterOffsets[i].offset == -2) {
        pHexRegList += snprintf(pHexRegList, 3, "%02x", ((U8 *)&_StackMem.Pointer)[j]);
      } else {
        pHexRegList += snprintf(pHexRegList, 3, "%02x",
          _StackMem.Data[_embOS.StackingInfo->RegisterOffsets[i].offset + j]);
      }
    }
  }
  return 0;
}

EXPORT int RTOS_SetThreadReg(char* pHexRegVal, U32 RegIndex, U32 threadid) {
  (void)pHexRegVal;
  (void)RegIndex;
  if (threadid == 1 || threadid == _embOS.CurrentThread) {
    return -1; // Current thread or current execution return CPU registers
  }
  //
  // Currently not supported
  //
  return 0;
}

EXPORT int RTOS_SetThreadRegList(char *pHexRegList, U32 threadid) {
  (void)pHexRegList;
  if (threadid == 1 || threadid == _embOS.CurrentThread) {
    return -1; // Current thread or current execution return CPU registers
  }
  //
  // Currently not supported
  //
  return 0;
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
  _StackMem.ThreadID = 0;

  //
  // Create new thread list
  //
  ThreadListSize = THREAD_LIST_CHUNK;
  _AllocThreadlist(ThreadListSize);
  if (!_embOS.pThreadDetails) {
    _pAPI->pfErrorOutf("Error allocating memory for %d threads.\n", ThreadListSize);
    return -2;
  }

  //
  // Read the current thread
  //
  retval = _pAPI->pfReadU32(RTOS_Symbols[embOS_Global].address + EMBOS_CURRENT_TASK_OFFSET, &_embOS.CurrentThread);
  if (retval != 0) {
    _pAPI->pfErrorOutf("Error reading current task.\n");
    return retval;
  }
  _pAPI->pfDebugOutf("Read current task at 0x%08X, value 0x%08X.\n",
                    RTOS_Symbols[embOS_Global].address + EMBOS_CURRENT_TASK_OFFSET,
                    _embOS.CurrentThread);

  TasksFound = 0;
  if (_embOS.CurrentThread == 0) {
    //
    // Current thread is null, if either task scheduler has not been started yet or
    // no task is currently executing. Show one dummy thread with the current execution.
    //
    _embOS.pThreadDetails->threadid = 0x00000001;
    _embOS.pThreadDetails->sThreadState = NULL;
    _embOS.pThreadDetails->sThreadName = (char*)_pAPI->pfAlloc(sizeof(_TaskCurrentExecution));
    _embOS.pThreadDetails->prio = 0;
    strncpy(_embOS.pThreadDetails->sThreadName, _TaskCurrentExecution, sizeof(_TaskCurrentExecution));

    _embOS.CurrentThread = 0x00000001;
    TasksFound = 1;
  }

  //
  // Find out how many lists are needed to be read from pxReadyTasksLists,
  //
  retval = _pAPI->pfReadU32(RTOS_Symbols[embOS_Global].address + EMBOS_FIRST_TASK_OFFSET, &tcb);
  if (retval != 0) {
    return retval;
  }
  _pAPI->pfDebugOutf("Read TCB at 0x%08X, value %08X.\n",
                    RTOS_Symbols[embOS_Global].address + EMBOS_FIRST_TASK_OFFSET,
                    tcb);
  while (tcb) {
    U32  tcb_name;
    U8   tcb_stat;
    char tmp_str[EMBOS_THREAD_NAME_SIZE];

    //
    // Store thread ID, grow thread list if required
    //
    if (TasksFound >= ThreadListSize) {
      ThreadListSize += THREAD_LIST_CHUNK;
      _ReallocThreadlist(ThreadListSize);
    }
    _embOS.pThreadDetails[TasksFound].threadid = tcb;
    //
    // Read the thread name
    //
    retval = _pAPI->pfReadU32(_embOS.pThreadDetails[TasksFound].threadid + EMBOS_TCB_NAME_OFFSET, &tcb_name);
    if (retval != 0) {
      return retval;
    }
    retval = _pAPI->pfReadMem(tcb_name, tmp_str, EMBOS_THREAD_NAME_SIZE - 1);
    if (retval == 0) {
      _pAPI->pfErrorOutf("Error reading first thread.\n");
      return retval;
    }
    tmp_str[EMBOS_THREAD_NAME_SIZE - 1] = '\x00';
    _pAPI->pfDebugOutf("Read thread name at 0x%08X, value \"%s\".\n",
                  RTOS_Symbols[embOS_Global].address + EMBOS_TCB_NAME_OFFSET,
                  tmp_str);
    if (tmp_str[0] == '\x00') {
      strncpy(tmp_str, "No Name", EMBOS_THREAD_NAME_SIZE - 1);
    }

    _embOS.pThreadDetails[TasksFound].sThreadName = (char*)_pAPI->pfAlloc(strlen(tmp_str) + 1);
    strncpy(_embOS.pThreadDetails[TasksFound].sThreadName, tmp_str, strlen(tmp_str) + 1);
    //
    // Save thread state
    //
    if (_embOS.pThreadDetails[TasksFound].threadid == _embOS.CurrentThread) {
      _embOS.pThreadDetails[TasksFound].sThreadState = _TaskState.Running;
    } else {
      retval = _pAPI->pfReadU8(_embOS.pThreadDetails[TasksFound].threadid + EMBOS_TCB_STAT_OFFSET, &tcb_stat);
      if (retval != 0) {
        _pAPI->pfErrorOutf("Error reading thread state.\n");
        return retval;
      }
      if ((tcb_stat & 0x3) > 0) {
        _embOS.pThreadDetails[TasksFound].sThreadState = _TaskState.Suspended;
      } else if ((tcb_stat & 0xF8) > 0) {
        _embOS.pThreadDetails[TasksFound].sThreadState = _TaskState.Waiting;
      } else {
        _embOS.pThreadDetails[TasksFound].sThreadState = NULL;
      }
    }
    retval = _pAPI->pfReadU8(_embOS.pThreadDetails[TasksFound].threadid + EMBOS_TCB_PRIO_OFFSET, &_embOS.pThreadDetails[TasksFound].prio);
    if (retval != 0) {
      _pAPI->pfErrorOutf("Error reading thread priority.\n");
      return retval;
    }
    retval = _pAPI->pfReadU32(_embOS.pThreadDetails[TasksFound].threadid + EMBOS_TCB_NEXT_OFFSET, &tcb);
    if (retval != 0) {
      _pAPI->pfErrorOutf("Error reading next thread item location in embOS thread list.\n");
      return retval;
    }
    _pAPI->pfDebugOutf("Read next thread location at 0x%08X, value 0x%08X.\n",
                  _embOS.pThreadDetails[TasksFound].threadid + EMBOS_TCB_NEXT_OFFSET,
                  tcb);
    TasksFound++;
  }

  _embOS.ThreadCount = TasksFound;
  return 0;
}
