cmake_minimum_required(VERSION 3.10)
project(flasher CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})

set(SOURCES
    src/main.cpp
    src/ui/main_window.cpp
    src/ui/status_bar.cpp
    src/core/flasher.cpp
    src/core/adb_helper.cpp
    src/core/fastboot_helper.cpp
    src/utils/thread_worker.cpp
    src/utils/logger.cpp
)

add_executable(flasher ${SOURCES})
target_link_libraries(flasher ${GTKMM_LIBRARIES} pthread)
