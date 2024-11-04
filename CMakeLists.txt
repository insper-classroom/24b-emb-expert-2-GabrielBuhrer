cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import_optional.cmake)

# Pull in FreeRTOS
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/freertos/FreeRTOS-Kernel)
include(freertos/FreeRTOS_Kernel_import.cmake)

project(pico_emb C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
  message(
    FATAL_ERROR
      "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}"
  )
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(
  -Wall
  -Wall -Wno-format # int != int32_t as far as the compiler is concerned because
                    # gcc has int32_t as long int
  -Wno-unused-function # we have some for the docs that aren't called
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()



# Codes subdir:
add_subdirectory(main)


add_executable(pico-freertos
    main/main.c
)

# Defina que o FreeRTOS será usado no modo SMP
target_compile_definitions(pico-freertos PRIVATE FREERTOS_SMP=1)


target_include_directories(pico-freertos PRIVATE
    ${FREERTOS_KERNEL_PATH}/include                 # Diretório principal do FreeRTOS
    ${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM0   # Diretório do port do FreeRTOS para ARM Cortex-M0
    ${CMAKE_CURRENT_LIST_DIR}/freertos              # Diretório da sua pasta 'freertos'
    ${CMAKE_CURRENT_LIST_DIR}/pico-sdk/src/common/pico_stdlib/include # Diretório de pico_stdlib
    ${CMAKE_CURRENT_LIST_DIR}/pico-sdk/src/rp2_common/hardware_dma/include # Diretório de hardware_dma
    ${CMAKE_CURRENT_LIST_DIR}/path/to/ssd1306/include # Ajuste este caminho para onde 'ssd1306.h' está localizado
    ${CMAKE_CURRENT_LIST_DIR}/path/to/gfx/include      # Ajuste este caminho para onde 'gfx.h' está localizado
    ${CMAKE_CURRENT_LIST_DIR}/oled1_lib
)

target_link_libraries(pico-freertos PRIVATE
    pico_stdlib
    hardware_dma
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    oled1_lib
)

target_compile_options(pico-freertos PRIVATE -Wall)

# Run the entire project in SRAM
# pico_set_binary_type(pico-freertos copy_to_ram)

pico_add_extra_outputs(pico-freertos)

add_subdirectory(oled1_lib)






