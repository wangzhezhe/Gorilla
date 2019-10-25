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
find_package (thallium REQUIRED)
xpkg_import_module (abtio REQUIRED abt-io)


# add the depedency
include_directories(${gorilla_SOURCE_DIR}/dep)


# build the server library
find_package(MPI REQUIRED) 
add_library(memcache 
${gorilla_SOURCE_DIR}/unimos/server/memcache.cpp
${gorilla_SOURCE_DIR}/unimos/server/objectbyrawmem.cpp
)

add_library(endpointmng 
${gorilla_SOURCE_DIR}/unimos/server/endpointManagement.cpp
)

add_library(unimosclientlib
${gorilla_SOURCE_DIR}/unimos/client/unimosclient.cpp
)


add_executable (unimos_server ${gorilla_SOURCE_DIR}/unimos/server/unimosserver.cpp)
target_link_libraries (unimos_server memcache endpointmng thallium MPI::MPI_CXX)


# execute the module load mpi before this part
add_executable(unimos_client ${gorilla_SOURCE_DIR}/unimos/client/minidsclient.cpp)
target_link_libraries (unimos_client MPI::MPI_CXX unimosclientlib thallium)

add_executable(largedatatest ${gorilla_SOURCE_DIR}/unimos/client/largedatatest.cpp)
target_link_libraries (largedatatest MPI::MPI_CXX thallium)



# add enable_testing
if (ENABLE_TESTING)
  add_subdirectory(unimos/test)
endif()


if (ENABLE_EXAMPLE)
  add_subdirectory(example)
endif()