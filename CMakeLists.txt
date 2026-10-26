cmake_minimum_required(VERSION 3.20)
project(TradeUpSearchReference VERSION 0.2.0 LANGUAGES CXX)

function(tradeup_diagnostics target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
    endif()
endfunction()

# Historical sources and binaries are never build targets.
add_library(tradeup_reference STATIC src/reference.cpp)
target_include_directories(tradeup_reference PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
tradeup_diagnostics(tradeup_reference)

add_library(tradeup_optimized STATIC src/optimized.cpp)
target_link_libraries(tradeup_optimized PUBLIC tradeup_reference)
tradeup_diagnostics(tradeup_optimized)

option(TRADEUP_BUILD_BENCHMARKS "Build private local synthetic benchmark" OFF)
if(TRADEUP_BUILD_BENCHMARKS)
    add_executable(search_benchmark benchmarks/search_benchmark.cpp)
    target_link_libraries(search_benchmark PRIVATE tradeup_optimized)
    if(WIN32)
        target_link_libraries(search_benchmark PRIVATE psapi)
    endif()
    tradeup_diagnostics(search_benchmark)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(baseline_examples tests/baseline_examples.cpp)
    tradeup_diagnostics(baseline_examples)
    add_test(NAME baseline_examples COMMAND baseline_examples)
    set_tests_properties(baseline_examples PROPERTIES TIMEOUT 10)

    add_executable(reference_tests tests/reference_tests.cpp)
    target_link_libraries(reference_tests PRIVATE tradeup_reference)
    tradeup_diagnostics(reference_tests)
    add_test(NAME reference_tests COMMAND reference_tests)
    set_tests_properties(reference_tests PROPERTIES TIMEOUT 20)

    add_executable(reference_fixture_driver tests/reference_fixture_driver.cpp)
    target_link_libraries(reference_fixture_driver PRIVATE tradeup_reference)
    tradeup_diagnostics(reference_fixture_driver)

    add_executable(optimized_tests tests/optimized_tests.cpp)
    target_link_libraries(optimized_tests PRIVATE tradeup_optimized)
    tradeup_diagnostics(optimized_tests)
    add_test(NAME optimized_tests COMMAND optimized_tests)
    set_tests_properties(optimized_tests PROPERTIES TIMEOUT 120)

    add_executable(optimized_fixture_driver tests/optimized_fixture_driver.cpp)
    target_link_libraries(optimized_fixture_driver PRIVATE tradeup_optimized)
    tradeup_diagnostics(optimized_fixture_driver)

    set(HISTORICAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/historical" CACHE PATH
        "Read-only location of the preserved historical directory")
    find_package(Python3 3.10 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME baseline_fixtures COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_baseline.py"
            --historical-root "${HISTORICAL_ROOT}")
        set_tests_properties(baseline_fixtures PROPERTIES TIMEOUT 30)
        add_test(NAME reference_differential COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_reference.py"
            --driver "$<TARGET_FILE:reference_fixture_driver>")
        set_tests_properties(reference_differential PROPERTIES TIMEOUT 30)
        add_test(NAME optimized_differential COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_optimized.py"
            --driver "$<TARGET_FILE:optimized_fixture_driver>")
        set_tests_properties(optimized_differential PROPERTIES TIMEOUT 60)
    else()
        message(STATUS "Python 3.10+ unavailable: archive and differential checks omitted")
    endif()
endif()
