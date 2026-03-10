cmake_minimum_required(VERSION 3.21)
project(MyLogReader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt 6
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(my_log_reader
    src/main.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/app/PlusTabBar.cpp
    src/app/PlusTabBar.h
    src/app/TabsWidget.cpp
    src/app/TabsWidget.h
    src/ui/LogReaderTab.cpp
    src/ui/LogReaderTab.h
)

# Link Qt Widgets to your executable
target_link_libraries(my_log_reader PRIVATE Qt6::Widgets)

# Add folder
target_include_directories(my_log_reader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Install
install(TARGETS my_log_reader
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Generate deploy script and run it at install time. Uses windeployqt under the hood.
qt_generate_deploy_app_script(
    TARGET my_log_reader
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
