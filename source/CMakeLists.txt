add_library(jobsystem STATIC JobSystem.cpp JobSystem.h)
target_link_libraries(jobsystem PRIVATE project_warnings Threads::Threads)
