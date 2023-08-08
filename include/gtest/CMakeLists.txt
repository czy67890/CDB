
FILE(GLOB LIB_DATA_BASE SKipList.cc)
add_library(DataBase ${LIB_DATA_BASE})
add_executable(testSkipList SkipListTest.cc)
target_link_libraries(testSkipList gtest gtest_main)
target_link_libraries(DataBase Util)