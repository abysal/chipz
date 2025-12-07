cmake_minimum_required(VERSION 3.31)
project(Chipz)
set(CMAKE_CXX_STANDARD 26)

include(cmake/CPM.cmake)

# --- Enable LTO globally ---
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if (NOT ipo_supported)
    message(FATAL_ERROR "IPO/LTO not supported: ${ipo_error}")
endif ()

# From this point, all targets with INTERPROCEDURAL_OPTIMIZATION=TRUE get LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# --- Compiler flags ---
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
endif ()

# Dependencies
CPMAddPackage("gh:raysan5/raylib#983efae3e4565cdf1441cb0b3dff9498bb0147e8")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "core/*.cpp" "core/*.hpp")

# Static library (will be built with LTO)
add_library(ChipzCore STATIC ${SOURCES})
target_include_directories(ChipzCore PRIVATE "core")
set_property(TARGET ChipzCore PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

# Final executable (also LTO)
add_executable(Chipz main.cpp)
target_include_directories(Chipz PRIVATE "core")
target_link_libraries(ChipzCore PUBLIC raylib)
target_link_libraries(Chipz ChipzCore)
set_property(TARGET Chipz PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
