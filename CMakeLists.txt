cmake_minimum_required(VERSION 3.5)
project(transfer.proxy VERSION 1.0.0)

option(GEN_ABI "Generate ABI" OFF)
option(BUILD_TEST "Build test dapp" OFF)

if(BUILD_TEST)
    add_subdirectory(src/test)
endif()

find_package(eosio.cdt)

if(GEN_ABI)
    add_contract(${PROJECT_NAME} ${PROJECT_NAME} 
        "src/transfer_proxy.cpp" 
    )
else()
    add_executable(${PROJECT_NAME}.wasm
        "src/transfer_proxy.cpp" 
    )
endif()
