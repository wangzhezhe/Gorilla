
add_executable(gray-scott
  ./simulation/main.cpp
  ./simulation/gray-scott.cpp
  ./simulation/writer.cpp
  ./simulation/settings.cpp
)

if (USE_GRPC)
target_link_libraries(gray-scott MPI::MPI_C ${VTK_LIBRARIES}
metaclient thallium unimos_client)
else()
target_link_libraries(gray-scott MPI::MPI_C ${VTK_LIBRARIES}
thallium unimos_client)
endif()

add_executable(isosurface
   ./analytics/isosurface.cpp
   ./simulation/settings.cpp)

if (USE_GRPC)
target_link_libraries(isosurface MPI::MPI_C ${VTK_LIBRARIES}
metaclient thallium unimos_client)
else()
target_link_libraries(isosurface MPI::MPI_C ${VTK_LIBRARIES}
thallium unimos_client)
endif()