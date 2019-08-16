cmake_minimum_required(VERSION 3.0.0)
project(ModelExplorer VERSION 2.1.0)

include(CTest)
enable_testing()

add_compile_options(-std=c++14 -O3)
find_package(PkgConfig REQUIRED)
pkg_search_module(ALLEGRO REQUIRED allegro_monolith-static-5)

include_directories(include)
include_directories("/usr/modelExplorerLibs/include/graphviz")
include_directories("/usr/modelExplorerLibs/include/coin")
include_directories(ALLEGRO_INCLUDE_DIRS)
link_directories("/usr/modelExplorerLibs/lib")

set(Boost_USE_STATIC_LIBS ON) 

set(LIB_NAMES   "libpthread.so"
                
                "libboost_system.a" 
                "libboost_filesystem.a"
                "libboost_serialization.a")

set(LIB_LOCAL   "libcgraph.so"
                "libgvc.so"
                
                "libOsiClp.so"
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

add_executable(ModelExplorer ${SOURCES})
target_link_libraries(ModelExplorer ${Boost_LIBRARIES})
target_link_libraries(ModelExplorer ${LIBS})
target_link_libraries(ModelExplorer ${LIB_LOCAL})
target_link_libraries(ModelExplorer ${ALLEGRO_STATIC_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
