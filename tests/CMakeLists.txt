qt_add_executable(tst_printfarm
    tst_printfarm.cpp
    ${CMAKE_SOURCE_DIR}/src/csvio.cpp
    ${CMAKE_SOURCE_DIR}/src/datastore.cpp
    ${CMAKE_SOURCE_DIR}/src/protocol.cpp
    ${CMAKE_SOURCE_DIR}/src/regelguete.cpp
    ${CMAKE_SOURCE_DIR}/src/analyse.cpp
    ${CMAKE_SOURCE_DIR}/src/auth.cpp
)
target_include_directories(tst_printfarm PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_printfarm PRIVATE Qt6::Core Qt6::Gui Qt6::Sql Qt6::Test)
add_test(NAME printfarm COMMAND tst_printfarm)
