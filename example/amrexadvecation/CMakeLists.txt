find_package(AMReX)

set(AMReX_SPACEDIM 2)

# build with mpi
SET(CMAKE_CXX_FLAGS  "-DAMREX_USE_MPI")

# List of source files
set(_sources AdvancePhiAllLevels.cpp  AdvancePhiAtLevel.cpp  AmrCoreAdv.cpp  AmrCoreAdv.H bc_fill.H)
list(APPEND _sources DefineVelocity.cpp  face_velocity.H  Kernels.H  main.cpp  Tagging.H)
list(APPEND _sources Src_K/Adv_K.H  Src_K/compute_flux_${AMReX_SPACEDIM}D_K.H  Src_K/slope_K.H)
list(TRANSFORM _sources PREPEND Source/)
list(APPEND _sources Exec/Prob.H)

# List of input files
set(_input_files inputs  inputs_for_scaling  paraview_amr101.py)
list(TRANSFORM _input_files PREPEND "Exec/")

# setup_tutorial(_sources _input_files)
# message (${_sources})

# message(${AMReX_INCLUDE_DIRS})

add_executable( amrAdvecation ${_sources})

# Find out which include directory is needed
set(_includes ${_sources})
list(FILTER _includes INCLUDE REGEX "\\.H$")
foreach(_item IN LISTS _includes)
   get_filename_component( _include_dir ${_item} DIRECTORY)
   #message(${_include_dir})
   target_include_directories( amrAdvecation PUBLIC ${_include_dir} )
endforeach()

target_link_libraries(amrAdvecation AMReX::amrex )

unset( _sources )
unset( _input_files   )
