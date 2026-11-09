cmake_minimum_required(VERSION 3.16)
project(Gruppe_JFH_Printfarm VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Sql Test LinguistTools)

qt_standard_project_setup()

# ---------------------------------------------------------------- Anwendung
qt_add_executable(Gruppe_JFH_Printfarm
    src/main.cpp
    src/datatypes.h
    src/csvio.h            src/csvio.cpp
    src/datastore.h        src/datastore.cpp
    src/protocol.h         src/protocol.cpp
    src/regelguete.h       src/regelguete.cpp
    src/analyse.h          src/analyse.cpp
    src/auth.h             src/auth.cpp
    src/plotwidget.h       src/plotwidget.cpp
    src/tablemodels.h      src/tablemodels.cpp
    src/dialogs.h          src/dialogs.cpp
    src/druckerwidget.h    src/druckerwidget.cpp
    src/teilewidget.h      src/teilewidget.cpp
    src/materialwidget.h   src/materialwidget.cpp
    src/fehlerlogwidget.h  src/fehlerlogwidget.cpp
    src/mainwindow.h       src/mainwindow.cpp
)

target_include_directories(Gruppe_JFH_Printfarm PRIVATE src)
target_link_libraries(Gruppe_JFH_Printfarm PRIVATE
    Qt6::Widgets Qt6::Charts Qt6::Sql Qt6::Gui Qt6::Core)

# Übersetzungen (Deutsch/Englisch) zur Laufzeit umschaltbar; die .qm-Dateien
# werden neben die Anwendung kopiert und dort per applicationDirPath geladen.
qt_add_translations(Gruppe_JFH_Printfarm
    TS_FILES translations/app_de.ts translations/app_en.ts
    QM_FILES_OUTPUT_VARIABLE qm_files)
add_custom_command(TARGET Gruppe_JFH_Printfarm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qm_files} $<TARGET_FILE_DIR:Gruppe_JFH_Printfarm>)

# ---------------------------------------------------------------- Tests
enable_testing()
add_subdirectory(tests)
