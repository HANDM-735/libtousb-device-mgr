message(STATUS "enter into ${CMAKE_CURRENT_SOURCE_DIR}")

aux_source_directory(. LIB_CJSON_SRC_LIST)

add_library(json STATIC ${LIB_CJSON_SRC_LIST})


install(TARGETS json DESTINATION lib)