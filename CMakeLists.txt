cmake_minimum_required (VERSION 3.14)
project (gorilla C CXX)
set (CMAKE_CXX_STANDARD 14)

option(ENABLE_TESTING "Enable Testing" ON)
option(ENABLE_EXAMPLE "Enable ENABLE_EXAMPLE" ON)
option(USE_TIMERS "Use profiling timers" ON)
enable_testing()
add_definitions(-DENABLE_TIMERS)

find_package(MPI REQUIRED) 

# add our cmake module directory to the path
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


# link shared lib with full rpath
set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# setup cache variables for ccmake
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release
         CACHE STRING "Choose the type of build." FORCE)
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS 
                  "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif ()
set (CMAKE_PREFIX_PATH "" CACHE STRING "External dependencies path")
set (BUILD_SHARED_LIBS "OFF" CACHE BOOL "Build a shared library")

# packages we depend on (load all the necessary depedency by spack)
include (xpkg-import)
find_package (mercury REQUIRED)
include_directories(${MERCURY_INCLUDE_DIR})
xpkg_import_module (argobots REQUIRED argobots)
xpkg_import_module (margo REQUIRED margo)
xpkg_import_module (abtio REQUIRED abt-io)
find_package (thallium REQUIRED)


option(USE_GNI "use cray gni" OFF)

if(USE_GNI)
  add_definitions(-DUSE_GNI)
  find_package(DRC REQUIRED)
  include_directories(${DRC_INCLUDE_DIRS})
  message ("use cray DRC")
  message ("include drc" ${DRC_INCLUDE_DIRS})
  message ("link drc lib" ${DRC_LIBRARIES})
endif()


# add uuid <there is problem to use this on suse>
# pkg_search_module(UUID REQUIRED uuid)

# add the depedency
include_directories(
     ${gorilla_SOURCE_DIR}/dep
)


add_library(dhtmanager 
${gorilla_SOURCE_DIR}/server/DHTManager/dhtmanager.cpp
${gorilla_SOURCE_DIR}/utils/hilbert/hilbert.c)

add_library(metadatamanager 
${gorilla_SOURCE_DIR}/server/MetadataManager/metadataManager.cpp)
target_link_libraries (metadatamanager thallium)

add_library(rawdatamanager 
${gorilla_SOURCE_DIR}/server/RawdataManager/blockManager.cpp
${gorilla_SOURCE_DIR}/server/RawdataManager/rawmemobj/rawmemobj.cpp
)
target_link_libraries(rawdatamanager thallium abt-io)

find_package(VTK COMPONENTS
    FiltersCore
    IOImage
    IOXML
    CommonCore
    FiltersSources
)

add_library(metaclient ./putgetMeta/metaclient.cpp)
target_link_libraries(metaclient thallium)

add_library (unimos_client 
${gorilla_SOURCE_DIR}/client/unimosclient.cpp)
target_link_libraries (unimos_client thallium)

# there is reference between each other, so it is better to build them together
add_library(trigger 
${gorilla_SOURCE_DIR}/server/FunctionManager/defaultFunctions/defaultfuncraw.cpp
${gorilla_SOURCE_DIR}/server/FunctionManager/defaultFunctions/defaultfuncmeta.cpp
${gorilla_SOURCE_DIR}/server/FunctionManager/functionManager.cpp
${gorilla_SOURCE_DIR}/server/TriggerManager/triggerManager.cpp
)
target_link_libraries(trigger thallium ${VTK_LIBRARIES} unimos_client)

# add enable_testing
# test everything ok before integrating them into a server

if (ENABLE_TESTING)
  add_subdirectory(test)
endif()

add_executable (unimos_server 
${gorilla_SOURCE_DIR}/server/unimosserver.cpp
${gorilla_SOURCE_DIR}/server/addrManager.cpp
${gorilla_SOURCE_DIR}/server/settings.cpp
)

if(USE_GNI)
target_link_libraries (unimos_server
 ${DRC_LIBRARIES}
 thallium 
 MPI::MPI_C 
 unimos_client 
 dhtmanager rawdatamanager metadatamanager 
 trigger ${VTK_LIBRARIES}
 metaclient)
else()
 target_link_libraries (unimos_server
 thallium 
 MPI::MPI_C 
 unimos_client 
 dhtmanager rawdatamanager metadatamanager 
 trigger ${VTK_LIBRARIES}
 metaclient)
endif()



# build the example
# the grpc only works for one level cmake, not sure the reason

if(ENABLE_EXAMPLE)
  add_subdirectory(example)
endif()
