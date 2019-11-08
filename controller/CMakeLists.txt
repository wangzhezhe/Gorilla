add_executable(taskcontroller ${gorilla_SOURCE_DIR}/controller/controller.cpp)
target_link_libraries (taskcontroller unimosclientlib thallium)