add_executable(gray-scott
  ./simulation/main.cpp
  ./simulation/gray-scott.cpp
  ./simulation/settings.cpp
  ./simulation/writer.cpp
  ./simulation/settings.cpp)

target_link_libraries(gray-scott MPI::MPI_C unimosclientlib thallium)