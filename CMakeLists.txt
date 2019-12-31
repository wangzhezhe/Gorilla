cmake_minimum_required (VERSION 3.15)
project (gorilla C CXX)
set (CMAKE_CXX_STANDARD 14)

option(ENABLE_TESTING "Enable Testing" ON)
enable_testing()

option(ENABLE_EXAMPLE "Enable Example" ON)

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

# add the depedency
include_directories(${gorilla_SOURCE_DIR}/dep)


add_library(dhtmanager 
${gorilla_SOURCE_DIR}/DHTManager/dhtmanager.cpp)
target_link_libraries (dhtmanager)


add_library(metadatamanager 
${gorilla_SOURCE_DIR}/server/MetadataManager/metadataManager.cpp)
target_link_libraries (metadatamanager thallium)


add_library(rawdatamanager 
${gorilla_SOURCE_DIR}/server/RawdataManager/blockManager.cpp
${gorilla_SOURCE_DIR}/server/RawdataManager/rawmemobj/rawmemobj.cpp
)
target_link_libraries(rawdatamanager thallium abt-io)

find_package(MPI REQUIRED) 
find_package(VTK COMPONENTS
    FiltersCore
    IOImage
    IOXML
    CommonCore
    FiltersSources
)

add_library(defaultfunc 
${gorilla_SOURCE_DIR}/server/ExecutionEngine/defaultFunctions/defaultfuncraw.cpp
${gorilla_SOURCE_DIR}/server/ExecutionEngine/defaultFunctions/defaultfuncmeta.cpp
)
target_link_libraries(defaultfunc ${VTK_LIBRARIES})


add_library(exengine 
${gorilla_SOURCE_DIR}/server/ExecutionEngine/executionengine.cpp)
target_link_libraries(exengine rawdatamanager thallium defaultfunc)

add_library(trigger 
${gorilla_SOURCE_DIR}/server/TriggerManager/dynamictrigger.cpp)
target_link_libraries(trigger exengine thallium defaultfunc)


add_executable (unimos_server 
${gorilla_SOURCE_DIR}/server/unimosserver.cpp
${gorilla_SOURCE_DIR}/server/addrManagement.cpp
)
target_link_libraries (unimos_server thallium MPI::MPI_CXX)





# add enable_testing
if (ENABLE_TESTING)
  add_subdirectory(test)
endif()

