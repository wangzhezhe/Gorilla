add_executable(test_dhtmanager ./DHTManager/test_dhtmanager.cpp ../utils/hilbert/hilbert.c)
target_link_libraries(test_dhtmanager dhtmanager)

add_executable(test_metadatamanager ./MetaData/test_metadata.cpp)
target_link_libraries(test_metadatamanager metadatamanager)

add_test( NAME test_dhtmanager COMMAND test_dhtmanager)

add_test( NAME test_metadatamanager COMMAND test_metadatamanager)