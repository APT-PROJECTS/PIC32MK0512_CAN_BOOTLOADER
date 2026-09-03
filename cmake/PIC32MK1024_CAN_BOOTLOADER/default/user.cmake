target_sources(PIC32MK1024_CAN_BOOTLOADER_default_default_XC32_compile PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../../src/driver/driver_CANbus.c"
    "${CMAKE_CURRENT_LIST_DIR}/../../../src/driver/flash_eeprom.c"
    "${CMAKE_CURRENT_LIST_DIR}/../../../src/middleware/boot_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/../../../src/middleware/device_stat.c"
    "${CMAKE_CURRENT_LIST_DIR}/../../../src/middleware/mcu_config.c"
    "${CMAKE_CURRENT_LIST_DIR}/../../../src/software/state_engine.c"
)