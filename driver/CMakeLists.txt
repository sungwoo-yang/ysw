add_executable(driver tests.cpp tests.h)

target_link_libraries(driver PRIVATE jobsystem project_warnings)
