
#set(GTEST_ROOT "/home/zw241/cworkspace/install/install_gtest")
#find_package(GTest REQUIRED)
#target_link_libraries(test_mem_putget GTest::GTest GTest::Main)


# Add test cpp file

add_executable(test_mem_putget test_mem_putget.cpp )
target_link_libraries(test_mem_putget memcache)


add_executable(test_mem_getregion test_mem_getregion.cpp)
target_link_libraries(test_mem_getregion memcache)

add_executable(test_tool test_tool.cpp)

find_package(OpenMP)
add_executable(test_mem_putget_parallel test_mem_putget_parallel.cpp)
target_link_libraries(test_mem_putget_parallel memcache OpenMP::OpenMP_CXX)


# Link test executable against gtest & gtest_main
add_test( NAME test_mem_putget COMMAND test_mem_putget)
add_test( NAME test_tool COMMAND test_tool)
add_test( NAME test_mem_getregion COMMAND test_mem_getregion)

# add_test( NAME test_mem_putget_parallel COMMAND test_mem_putget_parallel)






# test helloworld
add_executable(01_hello_server server.cpp)
target_link_libraries(01_hello_server thallium)
add_executable(01_hello_client client.cpp)
target_link_libraries(01_hello_client thallium)

