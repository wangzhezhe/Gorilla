
add_executable(gray-scott
  ./simulation/main.cpp
  ./simulation/gray-scott.cpp
  ./simulation/writer.cpp
  ./simulation/settings.cpp
)


target_link_libraries(gray-scott MPI::MPI_C ${VTK_LIBRARIES}
thallium unimos_client metaclient)


add_executable(isosurface
   ./analytics/isosurface.cpp
   ./simulation/settings.cpp)


target_link_libraries(isosurface MPI::MPI_C ${VTK_LIBRARIES}
thallium unimos_client metaclient)
