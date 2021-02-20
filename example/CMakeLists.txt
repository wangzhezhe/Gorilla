# -ltinfo is for solving mesa issues, refer to https://github.com/halide/Halide/issues/1112
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -luuid -ltinfo")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ltinfo")

add_executable(gray-scott-inline
  ./gssimulation/main_inline.cpp
  ./gssimulation/gray-scott.cpp
  ./gssimulation/writer.cpp
  ./gssimulation/settings.cpp
)

add_executable(gray-scott-stg
  ./gssimulation/main_stg.cpp
  ./gssimulation/gray-scott.cpp
  ./gssimulation/writer.cpp
  ./gssimulation/settings.cpp
)

add_executable(gray-scott-hybrid
  ./gssimulation/main_hybrid.cpp
  ./gssimulation/gray-scott.cpp
  ./gssimulation/writer.cpp
  ./gssimulation/settings.cpp
)

add_executable(isosurface
   ./analytics/isosurface_pulldata.cpp
   ./gssimulation/settings.cpp)

add_executable(isosurface_event
   ./analytics/isosurface_pullevent.cpp
   ./gssimulation/settings.cpp)

if(USE_GNI)
  target_link_libraries(gray-scott-inline ${DRC_LIBRARIES} MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)

target_link_libraries(gray-scott-stg ${DRC_LIBRARIES} MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)

target_link_libraries(gray-scott-hybrid ${DRC_LIBRARIES} MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)

target_link_libraries(isosurface ${DRC_LIBRARIES} MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)

target_link_libraries(isosurface_event ${DRC_LIBRARIES} MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)

else()
  target_link_libraries(gray-scott-inline MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)

  target_link_libraries(gray-scott-stg MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)

  target_link_libraries(gray-scott-hybrid MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)

  target_link_libraries(isosurface MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)

  target_link_libraries(isosurface_event MPI::MPI_C ${VTK_LIBRARIES}
thallium gorilla_clientforsim metaclient)
endif()


#add_executable(sim-inline-adios
#  ./sim-inline-adios/main.cpp
#  ./sim-inline-adios/gray-scott.cpp
#  ./sim-inline-adios/writer.cpp
#  ./sim-inline-adios/settings.cpp
#)

#target_link_libraries(sim-inline-adios adios2::adios2 ${VTK_LIBRARIES} MPI::MPI_C)

# the mb simulation
# TODO, extract the gssimualtion in cmake
# add_subdirectory(mbsimulation)