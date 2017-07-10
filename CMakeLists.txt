cmake_minimum_required(VERSION 3.7)
project(tdcpp)

set(CMAKE_CXX_STANDARD 11)

#set(CMAKE_C_FLAGS "-march=native -mtune=native -O2")
#set(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})

set(SOURCE_FILES_COMMON src/TDCpp/TDCpp_data.cpp src/TDCpp/TDCpp_data.h src/TDCpp/TDCpp_merger.cpp src/TDCpp/TDCpp_merger.h src/TDCpp/TDCpp_utils.cpp src/TDCpp/TDCpp_utils.h)

set(SOURCE_FILES_TWO src/two-fold.cpp)
add_executable(two-fold ${SOURCE_FILES_TWO} ${SOURCE_FILES_COMMON})

set(SOURCE_FILES_MATCH_N_PRINT src/match-n-print.cpp)
add_executable(match-n-print ${SOURCE_FILES_MATCH_N_PRINT} ${SOURCE_FILES_COMMON})

set(SOURCE_FILES_FOUR src/four-fold.cpp)
add_executable(four-fold ${SOURCE_FILES_FOUR} ${SOURCE_FILES_COMMON})

set(SOURCE_FILES_ONEBOX2FOLD src/one_box_2fold.cpp)
add_executable(one_box_2fold ${SOURCE_FILES_ONEBOX2FOLD} ${SOURCE_FILES_COMMON})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(four-fold Threads::Threads)
target_link_libraries(two-fold Threads::Threads)
target_link_libraries(match-n-print Threads::Threads)
target_link_libraries(one_box_2fold Threads::Threads)