

add_executable(test_dhtmanager ./DHTManager/test_dhtmanager.cpp ../utils/hilbert/hilbert.c)
target_link_libraries(test_dhtmanager dhtmanager)

add_executable(test_dhtmanagerManual ./DHTManager/test_dhtmanagerManual.cpp)
target_link_libraries(test_dhtmanagerManual dhtmanager)

add_executable(test_metadatamanager ./MetaData/test_metadata.cpp)
target_link_libraries(test_metadatamanager metadatamanager)

add_executable(test_meta_bbxcover ./MetaData/test_meta_bbxcover.cpp)
target_link_libraries(test_meta_bbxcover metadatamanager)

add_executable(test_bbx ./Util/test_bbx.cpp)
target_link_libraries(test_bbx)

add_executable(test_matrixbasic ./Util/test_matrixbasic.cpp)
target_link_libraries(test_matrixbasic)

add_executable(test_matrixassemble ./Util/test_matrixassemble.cpp)
target_link_libraries(test_matrixassemble)

add_executable(test_rawdatamanager ./RawData/test_rawdatamanager.cpp)
target_link_libraries(test_rawdatamanager rawdatamanager)

add_executable(test_vtkdataset ./RawData/test_vtkdataset.cpp)
target_link_libraries(test_vtkdataset rawdatamanager ${VTK_LIBRARIES})

add_executable(test_funcraw_vtk ./FuncManager/test_funcraw_vtk.cpp)
target_link_libraries(test_funcraw_vtk PRIVATE adios2::adios2 rawexec thallium ${VTK_LIBRARIES})

add_executable(test_funcraw_basic ./FuncManager/test_funcraw_basic.cpp)
target_link_libraries(test_funcraw_basic PRIVATE adios2::adios2 rawexec thallium )

add_executable(test_funcmeta ./FuncManager/test_funcmeta.cpp)
target_link_libraries(test_funcmeta PRIVATE adios2::adios2 metatrigger unimos_client thallium ${VTK_LIBRARIES})

add_executable(test_trigger ./Trigger/test_trigger.cpp)
target_link_libraries(test_trigger adios2::adios2 metatrigger unimos_client thallium ${VTK_LIBRARIES})

add_executable(test_addrManager ./Server/test_addrManager.cpp ${gorilla_SOURCE_DIR}/server/addrManager.cpp)
target_link_libraries(test_addrManager ${UUID_LIBRARIES} thallium unimos_client)

add_executable(test_put ./Server/test_put.cpp)
target_link_libraries(test_put thallium unimos_client)

add_executable(test_put_get ./Server/test_put_get.cpp)
target_link_libraries(test_put_get thallium unimos_client)

add_executable(test_threadpool ./Util/test_threadpool.cpp)
target_link_libraries(test_threadpool thallium )

add_executable(test_addtrigger ./Trigger/test_trigger.cpp)
target_link_libraries(test_addtrigger metatrigger thallium unimos_client)

add_executable(test_triggerthreadpool ./Trigger/test_triggerthreadpool.cpp)
target_link_libraries(test_triggerthreadpool metatrigger thallium)

file(COPY ${gorilla_SOURCE_DIR}/server/settings.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_executable(test_setting ./Setting/test_setting.cpp ${gorilla_SOURCE_DIR}/server/settings.cpp)
target_link_libraries(test_setting)

add_executable(06_custom_server ./thalimtest/server.cpp)
target_link_libraries(06_custom_server thallium)
add_executable(06_custom_client ./thalimtest/client.cpp)
target_link_libraries(06_custom_client thallium )
add_executable(mpinotifyblock ./thalimtest/mpinotifyblock.cpp)
target_link_libraries(mpinotifyblock thallium MPI::MPI_C)


#add_executable(thalliumtest_threadpool ./thalimtest/thalliumtest_threadpool.cpp)
#target_link_libraries(thalliumtest_threadpool thallium)

add_executable(test_insitu_exp ./Trigger/test_insitu_exp.cpp)
target_link_libraries(test_insitu_exp thallium unimos_client)

add_executable(test_trigger_loader ./Trigger/test_trigger_loader.cpp)
target_link_libraries(test_trigger_loader thallium unimos_client ${DRC_LIBRARIES})

add_executable(test_putgetevent ./Trigger/test_putgetevent.cpp)
target_link_libraries(test_putgetevent metatrigger thallium adios2::adios2)

add_executable(test_client_watcher ./Client/test_watcher.cpp)
target_link_libraries(test_client_watcher thallium unimos_client)

add_executable(adiosthread ./thalimtest/adiosthread.cpp)
target_link_libraries(adiosthread adios2::adios2)

add_subdirectory(mock)

add_test( NAME test_dhtmanager COMMAND test_dhtmanager)

add_test( NAME test_metadatamanager COMMAND test_metadatamanager)

add_test( NAME test_bbx COMMAND test_bbx)

add_test( NAME test_rawdatamanager COMMAND test_rawdatamanager)

add_test( NAME test_funcmeta COMMAND test_funcmeta)

add_test( NAME test_addrManager COMMAND test_addrManager)

add_test( NAME test_trigger COMMAND test_trigger)

add_test( NAME test_setting COMMAND test_setting)

add_test( NAME test_meta_bbxcover COMMAND test_meta_bbxcover)

add_test( NAME test_putgetevent COMMAND test_putgetevent)