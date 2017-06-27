cmake_minimum_required(VERSION 3.7)
project(tdccpp)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_FLAGS  "-march=native -mtune=native -O2")
set(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})

set(SOURCE_FILES_COMMON TDC_data.cpp TDC_data.h TDC_merger.cpp TDC_merger.h TDC_utils.cpp TDC_utils.h)

set(SOURCE_FILES_TWO two-fold.cpp)
add_executable(two-fold ${SOURCE_FILES_TWO} ${SOURCE_FILES_COMMON})

set(SOURCE_FILES_MATCH_N_PRINT match-n-print.cpp)
add_executable(match-n-print ${SOURCE_FILES_MATCH_N_PRINT} ${SOURCE_FILES_COMMON})

set(SOURCE_FILES_FOUR four-fold.cpp)
add_executable(four-fold ${SOURCE_FILES_FOUR} ${SOURCE_FILES_COMMON})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(four-fold Threads::Threads)
target_link_libraries(two-fold Threads::Threads)
target_link_libraries(match-n-print Threads::Threads)