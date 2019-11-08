
#set(GTEST_ROOT "/home/zw241/cworkspace/install/install_gtest")
#find_package(GTest REQUIRED)
#target_link_libraries(test_mem_putget GTest::GTest GTest::Main)


# Add test cpp file

add_executable(test_mem_putget test_mem_putget.cpp )
target_link_libraries(test_mem_putget memcache filterManager constraintManager ${CMAKE_THREAD_LIBS_INIT})


add_executable(test_mem_getregion test_mem_getregion.cpp)
target_link_libraries(test_mem_getregion memcache filterManager constraintManager ${CMAKE_THREAD_LIBS_INIT})

add_executable(test_tool test_tool.cpp)

add_executable(test_endpointManager test_endpointManager.cpp)
target_link_libraries(test_endpointManager endpointmng)

add_executable(test_constraintManager test_constraintManager.cpp)
target_link_libraries(test_constraintManager constraintManager)

add_executable(test_filterManager test_filterManager.cpp)
target_link_libraries(test_filterManager memcache constraintManager filterManager ${CMAKE_THREAD_LIBS_INIT})


find_package(OpenMP)
add_executable(test_mem_putget_parallel test_mem_putget_parallel.cpp)
target_link_libraries(test_mem_putget_parallel memcache constraintManager filterManager OpenMP::OpenMP_CXX ${CMAKE_THREAD_LIBS_INIT})


# Link test executable against gtest & gtest_main
add_test( NAME test_mem_putget COMMAND test_mem_putget)
add_test( NAME test_tool COMMAND test_tool)
add_test( NAME test_constraintManager COMMAND test_constraintManager)


add_test( NAME test_mem_getregion COMMAND test_mem_getregion)
add_test( NAME test_endpointManager COMMAND test_endpointManager)
add_test( NAME test_mem_putget_parallel COMMAND test_mem_putget_parallel)

# test helloworld for multiple server type
add_executable(01_hello_server server.cpp)
target_link_libraries(01_hello_server thallium)
add_executable(01_hello_client client.cpp)
target_link_libraries(01_hello_client thallium)

add_executable(argobot_test_thread argobot_test_thread.cpp)
target_link_libraries(argobot_test_thread thallium)

add_executable(argobot_test_threadpool argobot_test_threadpool.cpp)
target_link_libraries(argobot_test_threadpool thallium)