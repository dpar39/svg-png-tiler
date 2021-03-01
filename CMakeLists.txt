cmake_minimum_required(VERSION 3.16)
set(MODULE_NAME svg-png-tiler)
project(${MODULE_NAME})

string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_CONFIG_LOWER)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_UNITY_BUILD ON)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    endif()
endif()

if(EMSCRIPTEN)
    set(PRE_POST "--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/src/pre.js --post-js ${CMAKE_CURRENT_SOURCE_DIR}/src/post.js")
    set(EM_FLAGS "${PRE_POST} --bind -std=c++17 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=33554432 -s ASYNCIFY=1 -s 'ASYNCIFY_IMPORTS=[\"renderTile\"]'")
    if (${BUILD_CONFIG_LOWER} MATCHES "debug")
        message("Building in debug mode")
        set(EM_FLAGS "${EM_FLAGS} -g --source-map-base http://127.0.0.1:5501/example/ -s DISABLE_EXCEPTION_CATCHING=0")
    else()
        message("Building in release mode")
        set(EM_FLAGS "${EM_FLAGS} -Os -flto")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EM_FLAGS}")
    foreach(FLAG_VAR CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${FLAG_VAR} MATCHES "-g")
            string(REGEX REPLACE "-g" "-g4" ${FLAG_VAR} "${${FLAG_VAR}}")
        endif()
    endforeach()
else()
    message(STATUS "Compiling native C++")
endif()
message(STATUS CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS})


#-----------------------------
# Build the module library
#-----------------------------
file(GLOB LIB_CPP_FILES "src/*.cpp")
set(LIB_SRC_FILES ${LIB_C_FILES} ${LIB_CPP_FILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(${MODULE_NAME} ${LIB_SRC_FILES})
target_link_libraries(${MODULE_NAME} ${MODULE_LIB_DEPS})

message(STATUS "-------- Finished configuring CMake for module ${MODULE_NAME} --------")