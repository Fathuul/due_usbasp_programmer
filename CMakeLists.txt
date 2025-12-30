cmake_minimum_required(VERSION 3.16)

# --- Cross-Compiling Fix ---
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(due_usbasp_programmer C ASM)

# --- Toolchain ---
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# --- Chip & Board Configuration ---
# BOARD=ARDUINO_DUE_X is required if using certain ASF files
add_definitions(
    -D__SAM3X8E__ 
    -DBOARD=ARDUINO_DUE_X
    -DF_CPU=84000000
)

# Compiler Flags
set(MCU_FLAGS "-mcpu=cortex-m3 -mthumb")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -Wall -Wextra  -g -Os -ffunction-sections -fdata-sections")

# --- Path Definitions (Based on your new tree) ---
set(CMSIS_ROOT  "${CMAKE_SOURCE_DIR}/system/CMSIS")
set(DEVICE_PATH "${CMSIS_ROOT}/Device/ATMEL/sam3xa")
set(LIBSAM_PATH "${CMAKE_SOURCE_DIR}/system/libsam")
set(USB_PATH    "${CMAKE_SOURCE_DIR}/lib/usb")
set(DELAY_PATH  "${CMAKE_SOURCE_DIR}/lib/delay")

# --- Linker Script ---
# Based on the tree: system/CMSIS/Device/ATMEL/sam3xa/source/gcc/sam3x8e_flash.ld
set(LINKER_SCRIPT_PATH "${DEVICE_PATH}/source/gcc")
set(LINKER_SCRIPT "${LINKER_SCRIPT_PATH}/sam3x8e_flash.ld")

# -L adds the path so 'INCLUDE' directives inside the ld file work
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -T${LINKER_SCRIPT} -L${LINKER_SCRIPT_PATH} -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs")

# --- Includes ---
include_directories(
    "include"
    
    # CMSIS Core
    "${CMSIS_ROOT}/CMSIS/Include"
    
    # Device Headers (sam3x8e.h)
    "${DEVICE_PATH}/include"
    "${CMSIS_ROOT}/Device/ATMEL"                    # Sometimes needed for generic <sam.h>
    
    # Library Headers (libsam/include contains chip.h, pio.h, pmc.h etc.)
    "${LIBSAM_PATH}/include"
    "${LIBSAM_PATH}"
    
    # USB Library Headers (lib/usb/include)
    "${USB_PATH}/include"

    # delay Library Headers (lib/delay)
    "${DELAY_PATH}"

    # usbasp Library Headers
    "${CMAKE_SOURCE_DIR}/lib/avr_spi"
    "${CMAKE_SOURCE_DIR}/lib/hw_spi"
    "${CMAKE_SOURCE_DIR}/lib/usbasp"
    "${CMAKE_SOURCE_DIR}/lib/avr_devices"

)

# --- Sources ---

# 1. User Application
# Includes main.c, dcd_sam3x.c, usb_descriptors.c AND stubs.c
file(GLOB USER_SOURCES "src/*.c")

# 2. System Core & Startup
# Specific files from your new tree
set(SYSTEM_CORE_SOURCES
    "${DEVICE_PATH}/source/gcc/startup_sam3xa.c"
    "${DEVICE_PATH}/source/system_sam3xa.c"
)

# 3.1 LibSAM Drivers (Optional but recommended)
# This includes pio.c, pmc.c, etc. found in libsam/source/
file(GLOB LIBSAM_SOURCES "${LIBSAM_PATH}/source/*.c")

# 3.2 Apply per-source compile options (keeps your own code strict)
set_source_files_properties(${LIBSAM_SOURCES} PROPERTIES
  COMPILE_OPTIONS
    "-Wno-expansion-to-defined;-Wno-unused-value;-Wno-maybe-uninitialized;-Wno-implicit-fallthrough"
)

# 4. USB Sources
file(GLOB USB_SOURCES "${USB_PATH}/src/*.c")

# 5. delay Sources
file(GLOB DELAY_SOURCES "${DELAY_PATH}/*.c")

# 6. usbasp Sources
set(USBASP_SOURCES
    "${CMAKE_SOURCE_DIR}/lib/avr_spi/avr_spi.c"
    "${CMAKE_SOURCE_DIR}/lib/hw_spi/hw_spi.c"
    "${CMAKE_SOURCE_DIR}/lib/usbasp/usbasp_dispatch.c"
    "${CMAKE_SOURCE_DIR}/lib/usbasp/usbasp_handle.c"
    "${CMAKE_SOURCE_DIR}/lib/avr_devices/avr_devices_isr.c"
)

# --- Targets ---
add_executable(${PROJECT_NAME}.elf 
    ${USER_SOURCES} 
    ${SYSTEM_CORE_SOURCES} 
    ${LIBSAM_SOURCES}
    ${USB_SOURCES}
    ${DELAY_SOURCES}
    ${USBASP_SOURCES}
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Generating BIN file..."
)