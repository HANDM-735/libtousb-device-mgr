cmake_minimum_required(VERSION 3.10)
project(Test_LibToUSB-Device-Mgr)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

aux_source_directory(. MAIN_SRC_LIST)

if(SOFTVERSION)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSOFT_VERSION=\"${SOFTVERSION}\"")
    message(STATUS "SOFTVERSION is ${SOFTVERSION}")
endif()

if(DEVICETYPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEV_TYPE=\"${DEVICETYPE}\"")
    message(STATUS "DEVICETYPE is ${DEVICETYPE}")
endif()

add_subdirectory(libjson)
add_subdirectory(libusb)

add_executable(Test_LibToUSB-Device-Mgr ${MAIN_SRC_LIST})

target_link_libraries(Test_LibToUSB-Device-Mgr
    json
    usb
    boost_system
    boost_date_time
    boost_filesystem
    boost_thread
    pthread
    dl
)

target_include_directories(Test_LibToUSB-Device-Mgr PUBLIC
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/libjson
    ${PROJECT_SOURCE_DIR}/libusb
)

install(TARGETS Test_LibToUSB-Device-Mgr DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/config/ DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/config/g_libtousb.conf DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/ DESTINATION script)
