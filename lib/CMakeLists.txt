add_library(lib apa102.c apa102.h)

pico_generate_pio_header(lib ${CMAKE_CURRENT_LIST_DIR}/apa102.pio)

target_include_directories(lib PUBLIC ${CMAKE_CURRENT_LIST_DIR})