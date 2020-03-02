add_executable(gray-scott
  ./simulation/main.cpp
  ./simulation/gray-scott.cpp
  ./simulation/writer.cpp
  ./simulation/settings.cpp
)

add_executable(isosurface
   ./analytics/isosurface.cpp
   ./simulation/settings.cpp)

if(USE_GNI)
  target_link_libraries(gray-scott ${DRC_LIBRARIES} MPI::MPI_C ${VTK_LIBRARIES}
thallium unimos_client metaclient)

  target_link_libraries(isosurface ${DRC_LIBRARIES} MPI::MPI_C ${VTK_LIBRARIES}
thallium unimos_client metaclient)

else()
  target_link_libraries(gray-scott MPI::MPI_C ${VTK_LIBRARIES}
thallium unimos_client metaclient)
  target_link_libraries(isosurface MPI::MPI_C ${VTK_LIBRARIES}
thallium unimos_client metaclient)
endif()

# compile the sim-inline-adios
find_package(ADIOS2 REQUIRED)
add_executable(sim-inline-adios
  ./sim-inline-adios/main.cpp
  ./sim-inline-adios/gray-scott.cpp
  ./sim-inline-adios/writer.cpp
  ./sim-inline-adios/settings.cpp
)
target_link_libraries(sim-inline-adios ${VTK_LIBRARIES} adios2::adios2 MPI::MPI_C)
