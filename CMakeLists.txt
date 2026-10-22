cmake_minimum_required(VERSION 3.20)
project(TradeUpSearchBaseline VERSION 0.1.0 LANGUAGES CXX)

# Milestone 1 builds only isolated examples. Historical sources are never targets.
include(CTest)
if(BUILD_TESTING)
    add_executable(baseline_examples tests/baseline_examples.cpp)
    target_compile_features(baseline_examples PRIVATE cxx_std_20)
    set_target_properties(baseline_examples PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(baseline_examples PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(baseline_examples PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
    endif()
    add_test(NAME baseline_examples COMMAND baseline_examples)
    set_tests_properties(baseline_examples PROPERTIES TIMEOUT 10)

    set(HISTORICAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/historical" CACHE PATH
        "Read-only location of the preserved historical directory")
    find_package(Python3 3.10 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME baseline_fixtures
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_baseline.py"
                --historical-root "${HISTORICAL_ROOT}")
        set_tests_properties(baseline_fixtures PROPERTIES TIMEOUT 30)
    else()
        message(STATUS "Python 3.10+ unavailable: archive/fixture checks omitted")
    endif()
endif()
