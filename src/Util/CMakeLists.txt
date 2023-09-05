FILE(GLOB LIB_Util *.cc)
add_library(Util ${LIB_Util})
#add_executable(testStatus StatusTest.cc)
#target_link_libraries(testStatus gtest gtest_main Util)