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



