set(APP_SOURCES
    ${CMAKE_SOURCE_DIR}/src/csvio.cpp
    ${CMAKE_SOURCE_DIR}/src/datastore.cpp
    ${CMAKE_SOURCE_DIR}/src/protocol.cpp
    ${CMAKE_SOURCE_DIR}/src/regelguete.cpp
    ${CMAKE_SOURCE_DIR}/src/analyse.cpp
    ${CMAKE_SOURCE_DIR}/src/auth.cpp
)

# Logik-Tests (GUILESS)
qt_add_executable(tst_printfarm
    tst_printfarm.cpp ${APP_SOURCES})
target_include_directories(tst_printfarm PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_printfarm PRIVATE Qt6::Core Qt6::Gui Qt6::Sql Qt6::Test)
add_test(NAME printfarm COMMAND tst_printfarm)

# GUI-QA-Test (Widgets, offscreen)
qt_add_executable(tst_gui
    tst_gui.cpp
    ${APP_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/plotwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/ganttwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/tablemodels.cpp
    ${CMAKE_SOURCE_DIR}/src/dialogs.cpp
    ${CMAKE_SOURCE_DIR}/src/druckerwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/teilewidget.cpp
    ${CMAKE_SOURCE_DIR}/src/materialwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/fehlerlogwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/mainwindow.cpp
)
target_include_directories(tst_gui PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_gui PRIVATE Qt6::Widgets Qt6::Charts Qt6::Sql Qt6::Gui Qt6::Core Qt6::Test)
add_test(NAME gui COMMAND tst_gui)
