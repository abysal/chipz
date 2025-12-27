cmake_minimum_required(VERSION 3.31)
project(Chipz)
set(CMAKE_CXX_STANDARD 26)

include(cmake/CPM.cmake)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if (ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()


if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
endif ()
set(ASMJIT_STATIC TRUE)
set(ASMJIT_NO_FOREIGN TRUE)

CPMAddPackage("gh:raysan5/raylib#983efae3e4565cdf1441cb0b3dff9498bb0147e8")
CPMAddPackage("gh:asmjit/asmjit#12f9ca3b320e0d7e7eb6637a6af7fedbd12b8ddc")
CPMAddPackage("gh:ridiculousfish/libdivide#eea3b2436925396287abe64298e0e8a1c5674f68")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "core/*.cpp" "core/*.hpp")

add_library(ChipzCore STATIC ${SOURCES})
target_include_directories(ChipzCore PRIVATE "core")
set_property(TARGET ChipzCore PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

add_executable(Chipz main.cpp)
target_include_directories(Chipz PRIVATE "core")
target_link_libraries(ChipzCore PUBLIC raylib asmjit::asmjit libdivide)
target_link_libraries(Chipz ChipzCore)
set_property(TARGET Chipz PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
