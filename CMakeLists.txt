cmake_minimum_required(VERSION 3.16)
project(PrintFarm_Gruppe_03 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Sql Test LinguistTools)

qt_standard_project_setup()

# ---------------------------------------------------------------- Anwendung
qt_add_executable(PrintFarm_Gruppe_03 WIN32   # WIN32 = GUI-Subsystem, kein Konsolenfenster
    src/main.cpp
    src/datatypes.h
    src/theme.h
    src/csvio.h            src/csvio.cpp
    src/datastore.h        src/datastore.cpp
    src/protocol.h         src/protocol.cpp
    src/regelguete.h       src/regelguete.cpp
    src/analyse.h          src/analyse.cpp
    src/pidsim.h           src/pidsim.cpp
    src/auth.h             src/auth.cpp
    src/plotwidget.h       src/plotwidget.cpp
    src/achseutil.h
    src/ganttwidget.h      src/ganttwidget.cpp
    src/tablemodels.h      src/tablemodels.cpp
    src/dialogs.h          src/dialogs.cpp
    src/druckerwidget.h    src/druckerwidget.cpp
    src/teilewidget.h      src/teilewidget.cpp
    src/materialwidget.h   src/materialwidget.cpp
    src/fehlerlogwidget.h  src/fehlerlogwidget.cpp
    src/mainwindow.h       src/mainwindow.cpp
)

target_include_directories(PrintFarm_Gruppe_03 PRIVATE src)
set_target_properties(PrintFarm_Gruppe_03 PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(PrintFarm_Gruppe_03 PRIVATE
    Qt6::Widgets Qt6::Charts Qt6::Sql Qt6::Gui Qt6::Core Qt6::EntryPointPrivate)

# Übersetzungen (Deutsch/Englisch) zur Laufzeit umschaltbar; die .qm-Dateien
# werden neben die Anwendung kopiert und dort per applicationDirPath geladen.
qt_add_translations(PrintFarm_Gruppe_03
    TS_FILES translations/app_de.ts translations/app_en.ts
    QM_FILES_OUTPUT_VARIABLE qm_files)
add_custom_command(TARGET PrintFarm_Gruppe_03 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qm_files} $<TARGET_FILE_DIR:PrintFarm_Gruppe_03>)

# ---------------------------------------------------------------- Tests
enable_testing()
add_subdirectory(tests)
