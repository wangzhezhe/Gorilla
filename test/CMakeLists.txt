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

add_executable(test_rawexengine ./FuncManager/test_rawexengine.cpp)
target_link_libraries(test_rawexengine trigger thallium ${VTK_LIBRARIES})

add_executable(test_funcmeta ./FuncManager/test_funcmeta.cpp)
target_link_libraries(test_funcmeta trigger thallium ${VTK_LIBRARIES})

add_executable(test_trigger ./Trigger/test_trigger.cpp)
target_link_libraries(test_trigger trigger thallium ${VTK_LIBRARIES})

add_executable(test_addrManager ./Server/test_addrManager.cpp ${gorilla_SOURCE_DIR}/server/addrManager.cpp)
target_link_libraries(test_addrManager ${UUID_LIBRARIES} thallium unimos_client)

add_executable(test_put ./Server/test_put.cpp)
target_link_libraries(test_put thallium unimos_client)

add_executable(test_put_get ./Server/test_put_get.cpp)
target_link_libraries(test_put_get thallium unimos_client)

add_executable(test_threadpool ./Util/test_threadpool.cpp)
target_link_libraries(test_threadpool thallium )

add_executable(test_addtrigger ./Server/test_addtrigger.cpp)
target_link_libraries(test_addtrigger trigger thallium unimos_client )


add_executable(06_custom_server ./thalimtest/server.cpp)
target_link_libraries(06_custom_server thallium)
add_executable(06_custom_client ./thalimtest/client.cpp)
target_link_libraries(06_custom_client thallium )

add_executable(thalliumtest_threadpool ./thalimtest/thalliumtest_threadpool.cpp)
target_link_libraries(thalliumtest_threadpool thallium)

add_test( NAME test_dhtmanager COMMAND test_dhtmanager)

add_test( NAME test_metadatamanager COMMAND test_metadatamanager)

add_test( NAME test_bbx COMMAND test_bbx)

add_test( NAME test_rawdatamanager COMMAND test_rawdatamanager)

add_test( NAME test_rawexengine COMMAND test_rawexengine)

add_test( NAME test_funcmeta COMMAND test_funcmeta)

add_test( NAME test_addrManager COMMAND test_addrManager)

add_test( NAME test_trigger COMMAND test_trigger)