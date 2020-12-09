cmake_minimum_required(VERSION 3.12.0)

set(MODULE_NAME pngtiler)
project(${MODULE_NAME} CXX C)

message(STATUS "-------- Running CMake for module ${MODULE_NAME} --------")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(CMAKE_C_FLAGS "--bind -std=c++17 -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_CXX_FLAGS "--bind -std=c++17 -s ALLOW_MEMORY_GROWTH=1")

    SET(EMCC_LINKER_FLAGS "${EMCC_LINKER_FLAGS} ${CMAKE_CXX_FLAGS}")
    foreach(FLAG_VAR CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${FLAG_VAR} MATCHES "-g")
            string(REGEX REPLACE "-g" "-g4" ${FLAG_VAR} "${${FLAG_VAR}}")
        endif()
    endforeach()
    message(STATUS "Setting EMCC_LINKER_FLAGS=${EMCC_LINKER_FLAGS}")
else()
    if (MSVC)
        foreach(FLAG_VAR CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${FLAG_VAR} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${FLAG_VAR} "${${FLAG_VAR}}")
            endif()
        endforeach()
        add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    endif()
    if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        set(CMAKE_MACOSX_RPATH 1)
    endif()
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(THIRD_PARTY_INSTALL_DIR ${THIRD_PARTY_DIR}/install_${BUILD_NAME_SUFFIX})
endif()


#-----------------------------
# Build the module library
#-----------------------------
file(GLOB LIB_C_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB LIB_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
set(LIB_SRC_FILES ${LIB_C_FILES} ${LIB_CPP_FILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(${MODULE_NAME} ${LIB_SRC_FILES})
target_link_libraries(${MODULE_NAME} ${MODULE_LIB_DEPS})

message(STATUS "-------- Finished configuring CMake for module ${MODULE_NAME} --------")