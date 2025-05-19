

set(APP_NAME picomite)
set(PICOCALC true)
#set(SDBOOT true)

# Compile for PICO 1 Board
set(COMPILE PICO)

set(PICO_PLATFORM rp2040)
set(PICO_BOARD pico)


add_executable(${APP_NAME}
	PicoMite.c
	Memory.c
	regex.c 
	MMBasic.c
	Operators.c
	Custom.c
	Functions.c
	Commands.c
	FileIO.c
	ff.c
	ffsystem.c
	ffunicode.c
	mmc_stm32.c
	Draw.c
	Editor.c
	XModem.c
	MM_Misc.c
	External.c
	MATHS.c
	Onewire.c
	I2C.c
	SPI.c
	Serial.c
	SPI-LCD.c
	BmpDecoder.c
	GPS.c
	Audio.c
	CFunction.c
    picojpeg.c 
    lfs.c
    lfs_util.c
	hxcmod.c
    VS1053.c
	aes.c
)

target_sources(${APP_NAME} PRIVATE 
	SSD1963.c 
	Touch.c 
	GUI.c)

target_sources(${APP_NAME} PRIVATE 
	Keyboard.c 
	mouse.c
	)



if(PICOCALC STREQUAL "true")
	target_sources(${APP_NAME} PRIVATE
	picocalc/i2ckbd.c
	)
endif()

set_source_files_properties(mmc_stm32.c PROPERTIES COMPILE_FLAGS -O2)
set_source_files_properties(ff.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(GUI.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(BmpDecoder.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(GPS.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(I2C.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(lfs.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(picojpeg.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(regex.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(hxcmod.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(MATHS.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(Editor.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(aes.c PROPERTIES COMPILE_FLAGS -Os)
pico_generate_pio_header(${APP_NAME} ${CMAKE_CURRENT_LIST_DIR}/PicoMiteI2S.pio)

pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})                                                                                                                 
target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)                                                                                                               
pico_set_boot_stage2(${APP_NAME} slower_boot2) 

pico_enable_stdio_usb(${APP_NAME} 1)
pico_enable_stdio_uart(${APP_NAME} 0)

pico_add_extra_outputs(${APP_NAME})
pico_add_uf2_output(${APP_NAME})

target_link_options(${APP_NAME} PRIVATE -Wl,--print-memory-usage)
target_link_options(${APP_NAME} PRIVATE -Wl,-z,max-page-size=4096)

pico_set_linker_script(${APP_NAME} ${CMAKE_BINARY_DIR}/${APP_NAME}.ld)


pico_set_printf_implementation(${APP_NAME} compiler)

target_compile_options(${APP_NAME} PRIVATE -DNDEBUG
										-DPICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0
										-DPICO_ADC_CLKDIV_ROUND_NEAREST
										-DPICO_XOSC_STARTUP_DELAY_MULTIPLIER=64 
										-DPICO_CLOCK_AJDUST_PERI_CLOCK_WITH_SYS_CLOCK
										-DPICO_XOSC_STARTUP_DELAY_MULTIPLIER=64 
										-DPICO_FLASH_SIZE_BYTES=16777216
										-DPICO_CORE1_STACK_SIZE=0x00
										-DPICO_MALLOC_PANIC
										-O2
										-Wall)


target_compile_options(${APP_NAME} PRIVATE -DPICOMITE
										-DPICO_HEAP_SIZE=0x1000 
										-DGUICONTROLS
										-DPICO_CORE0_STACK_SIZE=0x1000
										)

#set the PICOCALC flag
if(PICOCALC STREQUAL "true")
	target_compile_options(${APP_NAME} PRIVATE -DPICOCALC
											)
endif()


target_link_libraries(${APP_NAME}
	pico_stdlib
	hardware_flash
	hardware_irq
	hardware_adc
	hardware_pwm
	hardware_i2c
	hardware_spi
	hardware_dma
	hardware_exception
	hardware_pio
	pico_multicore
	pico_unique_id
)


set(UF2_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.uf2)
set(UF2_DEST   ${CMAKE_BINARY_DIR}/${APP_NAME}.uf2)

add_custom_command(
    OUTPUT ${UF2_DEST}
    DEPENDS ${UF2_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E copy ${UF2_SOURCE} ${UF2_DEST}
    COMMENT "Copying ${APP_NAME}.uf2 to top-level build dir"
)


add_custom_target(PREPARE_${APP_NAME}
            COMMENT "Create Linker Script for '${APP_NAME}'"
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_SOURCE_DIR}/applink.py PREPARE
                    ${CMAKE_BINARY_DIR}
                    ${APP_NAME})

add_custom_target(BUILT_${APP_NAME}
        COMMENT "Record Build Details for '${APP_NAME}'"
        DEPENDS ${UF2_DEST}
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/applink.py BUILT
                ${CMAKE_BINARY_DIR}
                ${APP_NAME})



if(SDBOOT STREQUAL "true")
	pico_set_linker_script(${APP_NAME} ${CMAKE_SOURCE_DIR}/memmap_sdcard_app.ld)
endif()
