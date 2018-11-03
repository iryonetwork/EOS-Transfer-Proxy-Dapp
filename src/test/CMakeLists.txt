cmake_minimum_required(VERSION 3.5)
project(proxy.test VERSION 1.0.0)

option(GEN_ABI "Generate ABI" OFF)

find_package(eosio.cdt)

if(GEN_ABI)
    add_contract(${PROJECT_NAME} ${PROJECT_NAME} 
        "proxy_test.cpp" 
    )
else()
    add_executable(${PROJECT_NAME}.wasm
        "proxy_test.cpp" 
    )
endif()