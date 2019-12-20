/*
*****************************************************************************
**
**  File        : stm32_flash.ld
**
**  Abstract    : Linker script for STM32F429VE Device with
**                512KByte FLASH, 192KByte RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  For bootloader use FLASH (rx)      : ORIGIN = 0x08008000, LENGTH = 480K
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack, choose either main RAM or CCM RAM */
/* the 'end' symbol must be placed in the correct location of Stack */
_estack = 0x2000C000;    /* end of 192K RAM (using main RAM) */
//_estack = 0x10010000; /* use CCM RAM (64K) */


/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x4000;      /* required amount of heap  */

/* When choosing whether to use main RAM or CCM ram, change stack sizes accordingly */
_Min_Stack_Size = 0x200; /* required amount of stack */
_Min_Stack_Size_CCM = 0x100; /* in the CCM RAM */

/* Specify the memory areas */
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 512K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 192K
  MEMORY_B1 (rx)  : ORIGIN = 0x60000000, LENGTH = 0K
  CCMRAM (rw)     : ORIGIN = 0x10000000, LENGTH = 64K
  BKPSRAM (rw)	  : ORIGIN = 0x40024000, LENGTH = 4K
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    _stext = .;
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  .version :
  {
    . = ALIGN(4);
    _version_location = .;
    _version_function = . + 4;
    KEEP(*(.version))
    . = ALIGN(4);
  } >FLASH


  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.rodata)		   /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)  	   /* .rodata* sections (constants, strings, etc.) */	
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH


  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);
  
  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
	_sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT> FLASH
  
  _siccmram_data = LOADADDR(.ccmram_data);

  /* CCM-RAM section 
  * 
  * IMPORTANT NOTE! 
  * If initialized variables will be placed in this section, 
  * the startup code needs to be modified to copy the init-values.  
  *
  * ccmram_data - initialised data in the ccmram
  * ccmram_bss  - uninitialised data in the ccmram
  */
  .ccmram_data :
  {
    . = ALIGN(4);
    _sccmram_data = .;       /* create a global symbol at ccmram_data start */
    *(.ccmram_data)
    *(.ccmram_data*)
    
    . = ALIGN(4);
    _eccmram_data = .;       /* create a global symbol at ccmram_data end */
  } >CCMRAM AT> FLASH

  .bkpsram_dualboot_flag :
  {
  	. = ALIGN(4);
  	KEEP(*(.bkpsram_dualboot_flag))
  } >BKPSRAM
	
  _sibkpsram_data = LOADADDR(.bkpsram_data);
  /* Initialized section for Backup SRAM, must be initialised with BKPSRAM.c routines */
  .bkpsram_data :
  {
  	. = ALIGN(4);
    /* This is used by the startup in order to initialize the .bkpsram section */
    _sbkpsram_data = .;         /* define a global symbol at bkpsram_data start */
    *(.bkpsram_data)
    *(.bkpsram_data*)

    . = ALIGN(4);
    _ebkpsram_data = .;         /* define a global symbol at bkpsram_data end */
  } >BKPSRAM AT> FLASH
  

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM
  
  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(4);
	/*PROVIDE ( end = . );*/
	PROVIDE ( _end = . );
	__heap_start = .;
    . = . + _Min_Heap_Size;
    __heap_limit = .;
    . = . + _Min_Stack_Size;
    . = ALIGN(4);
  } >RAM
  
  /* ccmram_bss  - uninitialised data in the ccmram */
  .ccmram_bss :
  {
    . = ALIGN(4);
    _sccmram_bss = .;       /* create a global symbol at ccmram_bss start */
    *(.ccmram_bss)
    *(.ccmram_bss*)
    . = ALIGN(4);
    _eccmram_bss = .;       /* create a global symbol at ccmram_bss end */
  } >CCMRAM
  
  .ccmram_stack :
  {
    . = ALIGN(4);
    
	PROVIDE ( end = . );
	PROVIDE ( _end = . );
    
    . = . + _Min_Stack_Size_CCM; /* Used to check if there is enough stack */
    . = ALIGN(4);
  } >CCMRAM
  

  /* bkpsram_bss  - uninitialised data in the BKPSRAM */
  .bkpsram_bss :
  {
    . = ALIGN(4);
    _sbkpsram_bss = .;       /* create a global symbol at bkpsram_bss start */
    *(.bkpsram_bss)
    *(.bkpsram_bss*)
    . = ALIGN(4);
    _ebkpsram_bss = .;       /* create a global symbol at bkpsram_bss end */
  } >BKPSRAM
  




  /* MEMORY_bank1 section, code must be located here explicitly            */
  /* Example: extern int foo(void) __attribute__ ((section (".mb1text"))); */
  .memory_b1_text :
  {
    *(.mb1text)        /* .mb1text sections (code) */
    *(.mb1text*)       /* .mb1text* sections (code)  */
    *(.mb1rodata)      /* read-only data (constants) */
    *(.mb1rodata*)
  } >MEMORY_B1

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
