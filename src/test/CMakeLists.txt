cmake_minimum_required(VERSION 3.5)
project(proxy.test VERSION 1.0.0)

option(GEN_ABI "Generate ABI" OFF)


set(CMAKE_CXX_STANDARD 17)  # require C++17 std
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# if no wasm root is given use default path
if(EOSIO_CDT_ROOT STREQUAL "" OR NOT EOSIO_CDT_ROOT)
   set(EOSIO_CDT_ROOT "/usr/local/eosio.cdt")
endif()

# append the path to the module to include
list(APPEND CMAKE_MODULE_PATH ${EOSIO_CDT_ROOT}/lib/cmake)

#include the toolchain cmake
include(EosioWasmToolchain)

if(GEN_ABI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --abigen")
endif()

add_executable(${PROJECT_NAME}.wasm 
    "proxy_test.cpp" 
)
