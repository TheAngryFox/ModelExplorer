cmake_minimum_required(VERSION 3.0.0)
project(ErrorTracer VERSION 1.0.0)

include(CTest)
enable_testing()

add_compile_options(-std=c++14 -O3)

include_directories(include)
include_directories("/usr/modelExplorerLibs/include/coin")
link_directories("/usr/modelExplorerLibs/lib")

set(Boost_USE_STATIC_LIBS ON) 
#set(DWANT_STATIC_RUNTIME ON)


set(LIB_NAMES   "libpthread.so"
                "libboost_system.a" 
                "libboost_filesystem.a"
                "libboost_serialization.a")

set(LIB_LOCAL   "libOsiClp.so"
                "libClpSolver.so"
                "libClp.so"
                "libOsiCommonTests.so"
                "libOsi.so"
                "libCoinUtils.so")

foreach(LIB ${LIB_NAMES})
        find_library(FOUND_LIB_${LIB} ${LIB} HINTS "/usr/lib/x86_64-linux-gnu" "/usr/lib")
        list(APPEND LIBS ${FOUND_LIB_${LIB}})
        message("Lib: ${LIB}")
        message("Found Lib: ${FOUND_LIB_${LIB}}")
endforeach(LIB)

file(GLOB SOURCES "src/*.cpp")

add_executable(ErrorTracer ${SOURCES})
target_link_libraries(ErrorTracer ${Boost_LIBRARIES})
target_link_libraries(ErrorTracer ${LIBS})
target_link_libraries(ErrorTracer ${LIB_LOCAL})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
