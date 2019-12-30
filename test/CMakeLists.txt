add_executable(test_dhtmanager ./DHTManager/test_dhtmanager.cpp ../utils/hilbert/hilbert.c)
target_link_libraries(test_dhtmanager dhtmanager)

add_executable(test_metadatamanager ./MetaData/test_metadata.cpp)
target_link_libraries(test_metadatamanager metadatamanager)

add_executable(test_bbx ./Util/test_bbx.cpp)
target_link_libraries(test_bbx)

add_executable(test_matrixbasic ./Util/test_matrixbasic.cpp)
target_link_libraries(test_matrixbasic)

add_executable(test_matrixassemble ./Util/test_matrixassemble.cpp)
target_link_libraries(test_matrixassemble)

add_executable(test_rawdatamanager ./RawData/test_rawdatamanager.cpp)
target_link_libraries(test_rawdatamanager rawdatamanager)

add_executable(test_exengine ./Exec/test_exengine.cpp)
target_link_libraries(test_exengine exengine defaultfunc thallium ${VTK_LIBRARIES})

add_executable(test_trigger ./Trigger/test_trigger.cpp)
target_link_libraries(test_trigger trigger exengine defaultfunc thallium ${VTK_LIBRARIES})

#add_executable(06_custom_server ./thalimtest/server.cpp)
#target_link_libraries(06_custom_server thallium)
#add_executable(06_custom_client ./thalimtest/client.cpp)
#target_link_libraries(06_custom_client thallium )

add_test( NAME test_dhtmanager COMMAND test_dhtmanager)

add_test( NAME test_metadatamanager COMMAND test_metadatamanager)

add_test( NAME test_bbx COMMAND test_bbx)

add_test( NAME test_rawdatamanager COMMAND test_rawdatamanager)

add_test( NAME test_exengine COMMAND test_exengine)

add_test( NAME test_trigger COMMAND test_exengine)