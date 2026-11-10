# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\Gruppe_JFH_Printfarm_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Gruppe_JFH_Printfarm_autogen.dir\\ParseCache.txt"
  "Gruppe_JFH_Printfarm_autogen"
  "tests\\CMakeFiles\\tst_gui_autogen.dir\\AutogenUsed.txt"
  "tests\\CMakeFiles\\tst_gui_autogen.dir\\ParseCache.txt"
  "tests\\CMakeFiles\\tst_printfarm_autogen.dir\\AutogenUsed.txt"
  "tests\\CMakeFiles\\tst_printfarm_autogen.dir\\ParseCache.txt"
  "tests\\tst_gui_autogen"
  "tests\\tst_printfarm_autogen"
  )
endif()
