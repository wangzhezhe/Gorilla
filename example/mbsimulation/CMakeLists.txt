# mandelbulb simulation
add_executable(mandelbulb
  ./InSituAdaptor.cpp
  ./Mandelbulb_dynamic.cpp
  ./writer.cpp
)

if(USE_GNI)
  target_link_libraries(mandelbulb ${DRC_LIBRARIES} ${VTK_LIBRARIES} MPI::MPI_C  ParaView::icet ParaView::RemotingViews ParaView::PythonCatalyst
thallium unimos_client metaclient )
else()
  target_link_libraries(mandelbulb ${VTK_LIBRARIES} MPI::MPI_C ParaView::icet ParaView::RemotingViews ParaView::PythonCatalyst
thallium unimos_client metaclient)
endif()