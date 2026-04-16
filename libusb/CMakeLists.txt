message(STATUS "enter into ${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

aux_source_directory(. LIB_USB_SRC_LIST)

add_library(usb ${LIB_USB_SRC_LIST})


install(TARGETS usb DESTINATION lib)
install(FILES lib_interface.h DESTINATION include)