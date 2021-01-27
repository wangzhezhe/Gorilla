add_executable(test_dhtmanager ./DHTManager/test_dhtmanager.cpp ../utils/hilbert/hilbert.c)
target_link_libraries(test_dhtmanager thallium dhtmanager)

add_executable(test_dhtmanagerManual ./DHTManager/test_dhtmanagerManual.cpp)
target_link_libraries(test_dhtmanagerManual dhtmanager)

add_executable(test_metadatamanager ./MetaData/test_metadata.cpp)
target_link_libraries(test_metadatamanager thallium metadatamanager)

add_executable(test_meta_bbxcover ./MetaData/test_meta_bbxcover.cpp)
target_link_libraries(test_meta_bbxcover thallium metadatamanager)

add_executable(test_bbx ./Util/test_bbx.cpp)
target_link_libraries(test_bbx)

add_executable(test_matrixbasic ./Util/test_matrixbasic.cpp)
target_link_libraries(test_matrixbasic)

add_executable(test_matrixassemble ./Util/test_matrixassemble.cpp)
target_link_libraries(test_matrixassemble)

add_executable(test_membackend ./BlockData/test_membackend.cpp)
target_link_libraries(test_membackend thallium blockManager)

add_executable(test_filebackend ./BlockData/test_filebackend.cpp)
target_link_libraries(test_filebackend thallium blockManager)

add_executable(test_blockwitharray ./BlockData/test_blockwitharray.cpp)
target_link_libraries(test_blockwitharray thallium blockManager)

add_executable(test_funcraw_vtk ./FuncManager/test_funcraw_vtk.cpp)
target_link_libraries(test_funcraw_vtk PRIVATE thallium adios2::adios2 rawexec ${VTK_LIBRARIES})

add_executable(test_funcraw_basic ./FuncManager/test_funcraw_basic.cpp)
target_link_libraries(test_funcraw_basic PRIVATE thallium adios2::adios2 rawexec )

add_executable(test_funcmeta ./FuncManager/test_funcmeta.cpp)
target_link_libraries(test_funcmeta PRIVATE thallium adios2::adios2 metatrigger unimos_client ${VTK_LIBRARIES})

add_executable(test_trigger ./Trigger/test_trigger.cpp)
target_link_libraries(test_trigger thallium adios2::adios2 metatrigger unimos_client ${VTK_LIBRARIES})

add_executable(test_addrManager ./Server/test_addrManager.cpp ${gorilla_SOURCE_DIR}/server/addrManager.cpp)
target_link_libraries(test_addrManager thallium ${UUID_LIBRARIES} unimos_client)

add_executable(test_put ./Server/test_put.cpp)
target_link_libraries(test_put thallium unimos_client)

add_executable(test_put_get ./Server/test_put_get.cpp)
target_link_libraries(test_put_get thallium unimos_client)


add_executable(test_vtkobj_put ./Server/test_vtkobj_put.cpp)
target_link_libraries(test_vtkobj_put thallium blockManager unimos_client ${VTK_LIBRARIES} ${DRC_LIBRARIES})


add_executable(test_threadpool ./Util/test_threadpool.cpp)
target_link_libraries(test_threadpool thallium)

add_executable(test_circulararraytpool ./Util/test_circulararraytpool.cpp)
target_link_libraries(test_circulararraytpool thallium)

add_executable(test_addtrigger ./Trigger/test_trigger.cpp)
target_link_libraries(test_addtrigger thallium metatrigger unimos_client)


add_executable(test_metricManager ./Metrics/test_metricManager.cpp)
target_link_libraries(test_metricManager metricmanager thallium)


file(COPY ${gorilla_SOURCE_DIR}/server/settings.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_setting ./Setting/test_setting.cpp ${gorilla_SOURCE_DIR}/server/settings.cpp)
target_link_libraries(test_setting)

add_executable(06_custom_server ./thalimtest/server.cpp)
target_link_libraries(06_custom_server thallium)

add_executable(06_custom_client ./thalimtest/client.cpp)
target_link_libraries(06_custom_client thallium)

add_executable(mpinotifyblock ./thalimtest/mpinotifyblock.cpp)
target_link_libraries(mpinotifyblock thallium MPI::MPI_C)

#add_executable(thalliumtest_threadpool ./thalimtest/thalliumtest_threadpool.cpp)

add_executable(test_insitu_exp ./Trigger/test_insitu_exp.cpp)
target_link_libraries(test_insitu_exp thallium unimos_client)

add_executable(test_trigger_loader ./Trigger/test_trigger_loader.cpp)
target_link_libraries(test_trigger_loader thallium unimos_client ${DRC_LIBRARIES})

add_executable(test_putgetevent ./Trigger/test_putgetevent.cpp)
target_link_libraries(test_putgetevent thallium metatrigger adios2::adios2)

add_executable(test_client_watcher ./Client/test_watcher.cpp)
target_link_libraries(test_client_watcher thallium unimos_client)

add_executable(adiosthread ./thalimtest/adiosthread.cpp)
target_link_libraries(adiosthread adios2::adios2)

add_executable(test_scheduler ./Scheduler/test_scheduler.cpp)
target_link_libraries(test_scheduler schedulemanager thallium)

# vtk marshal
add_executable(test_vtkmem ./BlockData/test_vtkmem.cpp)
target_link_libraries(test_vtkmem ${VTK_LIBRARIES} thallium blockManager)

# vtk explicit array
add_executable(test_vtkexplicit ./BlockData/test_vtkexplicit.cpp)
target_link_libraries(test_vtkexplicit ${VTK_LIBRARIES} thallium blockManager)

# vtk explicit array with zero copy
add_executable(test_vtkexplicitzero ./BlockData/test_vtkexplicitzero.cpp)
target_link_libraries(test_vtkexplicitzero ${VTK_LIBRARIES} thallium blockManager)

# deprecated
# add_subdirectory(mock)

add_test( NAME test_dhtmanager COMMAND test_dhtmanager)

add_test( NAME test_metadatamanager COMMAND test_metadatamanager)

add_test( NAME test_bbx COMMAND test_bbx)

add_test( NAME test_membackend COMMAND test_membackend)

add_test( NAME test_filebackend COMMAND test_filebackend)

add_test( NAME test_blockwitharray COMMAND test_blockwitharray)

add_test( NAME test_vtkmem COMMAND test_vtkmem)

add_test( NAME test_funcmeta COMMAND test_funcmeta)

add_test( NAME test_addrManager COMMAND test_addrManager)

add_test( NAME test_trigger COMMAND test_trigger)

add_test( test_setting test_setting)

add_test( NAME test_meta_bbxcover COMMAND test_meta_bbxcover)

add_test( NAME test_putgetevent COMMAND test_putgetevent)

add_test( NAME test_vtkexplicit COMMAND test_vtkexplicit)

add_test( NAME test_metricManager COMMAND test_metricManager)
