add_executable(test_mockServer
${gorilla_SOURCE_DIR}/server/addrManager.cpp
${gorilla_SOURCE_DIR}/server/settings.cpp 
test_mockServer.cpp
settings_single.cpp
sim_single.cpp)

target_link_libraries(test_mockServer
 adios2::adios2 
 unimos_client 
 dhtmanager rawdatamanager metadatamanager 
 metatrigger rawexec ${VTK_LIBRARIES}
 thallium)

