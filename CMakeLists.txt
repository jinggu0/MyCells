cmake_minimum_required(VERSION 3.21)

project(MyCells VERSION 0.1.0 LANGUAGES C CXX)

# ------------------------------------------------------------
# Global settings
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(MYCELLS_BUILD_TESTS "Build tests" ON)
option(MYCELLS_BUILD_APP "Build desktop app target if main.cpp exists" ON)
option(MYCELLS_USE_SYSTEM_SQLITE3 "Use find_package(SQLite3) first" ON)

include(CTest)

if(MYCELLS_BUILD_APP)
    find_package(Qt6 QUIET COMPONENTS Core Gui Quick Qml QuickControls2)
    if(NOT Qt6_FOUND)
        message(FATAL_ERROR
            "[MyCells] Qt6 was not found, but MYCELLS_BUILD_APP is ON.\n"
            "Install Qt 6, then configure with the real Qt prefix, for example:\n"
            "  cmake -S . -B build -DCMAKE_PREFIX_PATH=\"C:/Qt/6.8.2/msvc2022_64\"\n"
            "The path must contain lib/cmake/Qt6/Qt6Config.cmake.\n"
            "If you only want core/storage/tests, configure with:\n"
            "  cmake -S . -B build -DMYCELLS_BUILD_APP=OFF"
        )
    endif()

    string(TOLOWER "${Qt6_DIR};${CMAKE_PREFIX_PATH}" MYCELLS_QT_HINT_LOWER)
    if(MSVC AND MYCELLS_QT_HINT_LOWER MATCHES "mingw")
        message(FATAL_ERROR
            "[MyCells] Compiler/Qt ABI mismatch: Visual Studio/MSVC generator is using a MinGW Qt package.\n"
            "Use a Qt MSVC kit, for example C:/Qt/6.11.0/msvc2022_64, or configure with a MinGW generator:\n"
            "  cmake -S . -B build-mingw -G Ninja "
            "-DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe "
            "-DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe "
            "-DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe "
            "-DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64"
        )
    endif()
    if(MINGW AND MYCELLS_QT_HINT_LOWER MATCHES "msvc")
        message(FATAL_ERROR
            "[MyCells] Compiler/Qt ABI mismatch: MinGW compiler is using an MSVC Qt package.\n"
            "Use C:/Qt/6.11.0/mingw_64, or switch to a Visual Studio/MSVC generator."
        )
    endif()

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    qt_standard_project_setup()

    if(COMMAND qt_policy)
        qt_policy(SET QTP0001 NEW)
        qt_policy(SET QTP0004 NEW)
    endif()
endif()

# ------------------------------------------------------------
# Helper: append only files that actually exist
# ------------------------------------------------------------
function(mycells_append_existing out_var)
    set(tmp "${${out_var}}")
    foreach(path IN LISTS ARGN)
        if(EXISTS "${path}")
            list(APPEND tmp "${path}")
        else()
            message(STATUS "[MyCells] Skipping missing file: ${path}")
        endif()
    endforeach()
    set(${out_var} "${tmp}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------
# Common compile settings
# ------------------------------------------------------------
function(mycells_apply_common_settings target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_20)
    target_include_directories(${target_name}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ------------------------------------------------------------
# Core library
# ------------------------------------------------------------
set(MYCELLS_CORE_SOURCES)
mycells_append_existing(MYCELLS_CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/update/GrowthRules.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/update/StressRules.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/update/CellUpdater.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/time/FastForwardEngine.cpp"
)

if(MYCELLS_CORE_SOURCES STREQUAL "")
    message(FATAL_ERROR "[MyCells] No core source files were found.")
endif()

add_library(acell_core STATIC ${MYCELLS_CORE_SOURCES})
add_library(MyCells::Core ALIAS acell_core)
mycells_apply_common_settings(acell_core)

# ------------------------------------------------------------
# SQLite3 dependency
# Priority:
#   1) find_package(SQLite3)
#   2) vendored sqlite3.c/sqlite3.h under third_party/sqlite/
# ------------------------------------------------------------
set(MYCELLS_SQLITE_TARGET "")

if(MYCELLS_USE_SYSTEM_SQLITE3)
    find_package(SQLite3 QUIET)
    if(SQLite3_FOUND)
        set(MYCELLS_SQLITE_TARGET SQLite::SQLite3)
        message(STATUS "[MyCells] Using system SQLite3")
    endif()
endif()

if(NOT MYCELLS_SQLITE_TARGET)
    set(SQLITE_VENDOR_C "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite/sqlite3.c")
    set(SQLITE_VENDOR_H "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite/sqlite3.h")

    if(EXISTS "${SQLITE_VENDOR_C}" AND EXISTS "${SQLITE_VENDOR_H}")
        add_library(sqlite3_vendor STATIC "${SQLITE_VENDOR_C}")
        target_include_directories(sqlite3_vendor PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite"
        )
        if(MSVC)
            target_compile_definitions(sqlite3_vendor PRIVATE _CRT_SECURE_NO_WARNINGS)
        endif()
        if(WIN32)
            target_compile_definitions(sqlite3_vendor PRIVATE _WIN32_WINNT=0x0600)
        endif()

        set(MYCELLS_SQLITE_TARGET sqlite3_vendor)
        message(STATUS "[MyCells] Using vendored SQLite3 from third_party/sqlite")
    else()
        message(FATAL_ERROR
            "[MyCells] SQLite3 not found.\n"
            "Either install SQLite3 so find_package(SQLite3) works,\n"
            "or place sqlite3.c and sqlite3.h in third_party/sqlite/."
        )
    endif()
endif()

# ------------------------------------------------------------
# Storage library
# ------------------------------------------------------------
set(MYCELLS_STORAGE_SOURCES)
mycells_append_existing(MYCELLS_STORAGE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/storage/Database.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/storage/CellRepository.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/storage/EnvironmentRepository.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/storage/EventLogRepository.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/storage/UserActionRepository.cpp"
)

if(MYCELLS_STORAGE_SOURCES)
    add_library(acell_storage STATIC ${MYCELLS_STORAGE_SOURCES})
    add_library(MyCells::Storage ALIAS acell_storage)

    mycells_apply_common_settings(acell_storage)

    target_link_libraries(acell_storage
        PUBLIC
            acell_core
            ${MYCELLS_SQLITE_TARGET}
    )
else()
    message(STATUS "[MyCells] No storage sources found. acell_storage target will not be created.")
endif()

# ------------------------------------------------------------
# Services library
# ------------------------------------------------------------
set(MYCELLS_SERVICE_SOURCES)
mycells_append_existing(MYCELLS_SERVICE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/SimulationService.cpp"
)

if(MYCELLS_SERVICE_SOURCES)
    add_library(acell_services STATIC ${MYCELLS_SERVICE_SOURCES})
    add_library(MyCells::Services ALIAS acell_services)

    mycells_apply_common_settings(acell_services)

    target_link_libraries(acell_services
        PUBLIC
            acell_core
    )

    if(TARGET acell_storage)
        target_link_libraries(acell_services PUBLIC acell_storage)
    endif()
else()
    message(STATUS "[MyCells] No service sources found. acell_services target will not be created.")
endif()

# ------------------------------------------------------------
# Optional app target
# Build only if src/app/main.cpp exists
# ------------------------------------------------------------
if(MYCELLS_BUILD_APP)
    set(APP_SOURCES)
    mycells_append_existing(APP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/app/main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/app/AppController.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/app/AppController.h"
    )

    if(APP_SOURCES)
        qt_add_executable(MyCells ${APP_SOURCES})
        mycells_apply_common_settings(MyCells)

        target_link_libraries(MyCells
            PRIVATE
                Qt6::Core
                Qt6::Gui
                Qt6::Quick
                Qt6::Qml
                Qt6::QuickControls2
                acell_core
        )

        if(TARGET acell_storage)
            target_link_libraries(MyCells PRIVATE acell_storage)
        endif()

        if(TARGET acell_services)
            target_link_libraries(MyCells PRIVATE acell_services)
        endif()

        qt_add_qml_module(MyCells
            URI MyCells
            VERSION 1.0
            NO_IMPORT_SCAN
            QML_FILES
                src/ui/qml/Main.qml
        )

        set_target_properties(MyCells PROPERTIES
            OUTPUT_NAME "MyCells"
        )
    else()
        message(STATUS "[MyCells] app sources not found. Skipping Qt app target.")
    endif()
endif()

# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------
if(BUILD_TESTING AND MYCELLS_BUILD_TESTS)
    find_package(Catch2 3 QUIET)

    if(NOT Catch2_FOUND)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/Catch2/CMakeLists.txt")
            add_subdirectory(external/Catch2)
            message(STATUS "[MyCells] Using vendored Catch2 from external/Catch2")
        else()
            message(STATUS "[MyCells] Catch2 v3 not found. Using local SimpleTest fallback.")
        endif()
    else()
        message(STATUS "[MyCells] Using system Catch2 v3")
    endif()

    set(MYCELLS_TEST_SOURCES)
    mycells_append_existing(MYCELLS_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/core/CellUpdaterTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/core/FastForwardEngineTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/SimulationServiceTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/storage/DatabaseTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/storage/RepositoryTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/SimulationPersistenceTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/OfflineResumeTest.cpp"
    )

    if(MYCELLS_TEST_SOURCES)
        if(NOT Catch2_FOUND AND NOT TARGET Catch2::Catch2WithMain)
            list(APPEND MYCELLS_TEST_SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/core/TestMain.cpp"
            )
        endif()

        add_executable(core_tests ${MYCELLS_TEST_SOURCES})
        mycells_apply_common_settings(core_tests)

        target_link_libraries(core_tests
            PRIVATE
                acell_core
        )

        if(TARGET Catch2::Catch2WithMain)
            target_link_libraries(core_tests PRIVATE Catch2::Catch2WithMain)
        endif()

        if(TARGET acell_services)
            target_link_libraries(core_tests PRIVATE acell_services)
        endif()

        if(TARGET acell_storage)
            target_link_libraries(core_tests PRIVATE acell_storage)
        endif()

        if(TARGET Catch2::Catch2WithMain)
            include(Catch)
            catch_discover_tests(core_tests)
        else()
            add_test(NAME core_tests COMMAND core_tests)
        endif()

        set_target_properties(core_tests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
            OUTPUT_NAME "core_tests"
        )
    else()
        message(STATUS "[MyCells] No test source files found. Skipping test target.")
    endif()
endif()

# ------------------------------------------------------------
# Nice startup summary
# ------------------------------------------------------------
message(STATUS "")
message(STATUS "========== MyCells Build Summary ==========")
message(STATUS "Project version       : ${PROJECT_VERSION}")
message(STATUS "C++ standard          : ${CMAKE_CXX_STANDARD}")
message(STATUS "Build tests           : ${MYCELLS_BUILD_TESTS}")
message(STATUS "Build app             : ${MYCELLS_BUILD_APP}")
message(STATUS "SQLite target         : ${MYCELLS_SQLITE_TARGET}")
if(TARGET acell_storage)
    message(STATUS "Storage target        : enabled")
else()
    message(STATUS "Storage target        : disabled")
endif()
if(TARGET acell_services)
    message(STATUS "Services target       : enabled")
else()
    message(STATUS "Services target       : disabled")
endif()
if(TARGET MyCells)
    message(STATUS "App target            : enabled")
else()
    message(STATUS "App target            : disabled")
endif()
if(TARGET core_tests)
    message(STATUS "Test target           : enabled")
else()
    message(STATUS "Test target           : disabled")
endif()
message(STATUS "==========================================")
message(STATUS "")
