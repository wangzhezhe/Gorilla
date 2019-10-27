add_executable(gray-scott
  ./simulation/main.cpp
  ./simulation/gray-scott.cpp
  ./simulation/settings.cpp
  ./simulation/writer.cpp
  ./simulation/settings.cpp)

target_link_libraries(gray-scott MPI::MPI_C unimosclientlib thallium)

find_package(VTK COMPONENTS
    FiltersCore
    IOImage
    IOXML
    IOLegacy
)

add_executable(isosurface analytics/isosurface.cpp)
target_link_libraries(isosurface ${VTK_LIBRARIES} MPI::MPI_C unimosclientlib thallium)