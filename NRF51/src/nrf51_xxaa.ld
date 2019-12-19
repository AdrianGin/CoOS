/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00018000, LENGTH = 0x22400
  /*FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x26000*/
  RAM (rwx) :  ORIGIN = 0x20002100, LENGTH = 0x5C00
  
  
  uicr_rbpconf (r) : ORIGIN = 0x10001004, LENGTH = 0x4
  uicr_code_length_region_zero (r) : ORIGIN = 0x10001000, LENGTH = 0x4
  uicr_bootloader_start_address (r) : ORIGIN = 0x10001014, LENGTH = 0x4
  bootloader_settings_page (r) : ORIGIN = 0x0003FC00, LENGTH = 0x400
  

  
  gazell_db (r) : ORIGIN = 0x3BC00, LENGTH = 0x400
  eeprom_emu (r) : ORIGIN = 0x3AC00, LENGTH = 0xC00
  config_database (r) : ORIGIN = 0x3A800, LENGTH = 0x400
  ble_device_manager (r) : ORIGIN = 0x3A400, LENGTH = 0x400
  
  
  
  
}


INCLUDE "nrf51_common.ld"


SECTIONS
{

  .uicr_rbpconf :
  {
    KEEP(*(SORT(.uicr_rbpconf*)))
  } > uicr_rbpconf

  .uicr_code_length_region_zero :
  {
    PROVIDE(__start_uicr_code_length_region_zero = .);
    KEEP(*(SORT(.uicr_code_length_region_zero*)))
    PROVIDE(__stop_uicr_code_length_region_zero = .);
  } > uicr_code_length_region_zero

  .uicr_bootloader_start_address :
  {
    PROVIDE(__start_uicr_bootloader_start_address = .);
    KEEP(*(SORT(.uicr_bootloader_start_address*)))
    PROVIDE(__stop_uicr_bootloader_start_address = .);
  } > uicr_bootloader_start_address
  
  .bootloader_settings_page :
  {
    PROVIDE(__start_bootloader_settings_page = .);
    KEEP(*(SORT(.bootloader_settings_page*)))
    PROVIDE(__stop_bootloader_settings_page = .);
  } > bootloader_settings_page
  
  .gazell_db :
  {
    KEEP(*(SORT(.gazell_db*)))
	__egazell_db = .;
  } > gazell_db
  
  
  .ble_device_manager :
  {
    KEEP(*(.ble_device_manager*))
  } > ble_device_manager  
  
  .config_database :
  {
    KEEP(*(.config_database*))
  } > config_database
    
  .eeprom_emu :
  {
    KEEP(*(SORT(.eeprom_emu*)))
  } > eeprom_emu
  
  

}

