# CMake generated Testfile for 
# Source directory: C:/Users/witts/Desktop/Gruppe_JFH_Printfarm/tests
# Build directory: C:/Users/witts/Desktop/Gruppe_JFH_Printfarm/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(printfarm "C:/Users/witts/Desktop/Gruppe_JFH_Printfarm/build/tst_printfarm.exe")
set_tests_properties(printfarm PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" _BACKTRACE_TRIPLES "C:/Users/witts/Desktop/Gruppe_JFH_Printfarm/tests/CMakeLists.txt;15;add_test;C:/Users/witts/Desktop/Gruppe_JFH_Printfarm/tests/CMakeLists.txt;0;")
add_test(gui "C:/Users/witts/Desktop/Gruppe_JFH_Printfarm/build/tst_gui.exe")
set_tests_properties(gui PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=C:/Qt/6.11.1/mingw_64/lib/cmake/Qt6/../../../plugins" _BACKTRACE_TRIPLES "C:/Users/witts/Desktop/Gruppe_JFH_Printfarm/tests/CMakeLists.txt;37;add_test;C:/Users/witts/Desktop/Gruppe_JFH_Printfarm/tests/CMakeLists.txt;0;")
