cmake_minimum_required(VERSION 3.20)
project(TradeUpSearchReference VERSION 0.2.0 LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Use an out-of-source build: cmake -S . -B build/debug")
endif()

option(TRADEUP_ENABLE_SANITIZERS "Run with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(TRADEUP_REQUIRE_PYTHON "Require Python 3.10+ verification when BUILD_TESTING is enabled" OFF)
if(TRADEUP_ENABLE_SANITIZERS)
    if(WIN32 OR NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Combined ASan/UBSan requires a supported Linux GCC/Clang runtime; use the Linux CI configuration")
    endif()
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address,undefined -fno-sanitize-recover=all")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address,undefined")
    check_cxx_source_compiles("int main() { return 0; }" TRADEUP_SANITIZERS_LINK)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
    if(NOT TRADEUP_SANITIZERS_LINK)
        message(FATAL_ERROR "ASan/UBSan runtimes do not link with this compiler")
    endif()
endif()

function(tradeup_diagnostics target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
        target_compile_definitions(${target} PRIVATE $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
    endif()
    if(TRADEUP_ENABLE_SANITIZERS)
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

# Historical sources and binaries are never build targets.
add_library(tradeup_reference STATIC src/reference.cpp)
target_include_directories(tradeup_reference PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
tradeup_diagnostics(tradeup_reference)

add_library(tradeup_optimized STATIC src/optimized.cpp)
target_link_libraries(tradeup_optimized PUBLIC tradeup_reference)
tradeup_diagnostics(tradeup_optimized)

add_library(tradeup_application STATIC src/sha256.cpp src/catalog_io.cpp src/historical.cpp
    src/application.cpp src/serialization.cpp)
target_link_libraries(tradeup_application PUBLIC tradeup_optimized)
tradeup_diagnostics(tradeup_application)
add_executable(tradeup src/cli.cpp)
target_link_libraries(tradeup PRIVATE tradeup_application)
if(MINGW)
    target_link_options(tradeup PRIVATE -municode)
endif()
tradeup_diagnostics(tradeup)

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

    add_executable(application_tests tests/application_tests.cpp)
    target_link_libraries(application_tests PRIVATE tradeup_application)
    tradeup_diagnostics(application_tests)
    add_test(NAME application_tests COMMAND application_tests)
    set_tests_properties(application_tests PROPERTIES TIMEOUT 30)

    set(HISTORICAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/historical" CACHE PATH
        "Read-only location of the preserved historical directory")
    find_package(Python3 3.10 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        message(STATUS "Python verification: ${Python3_EXECUTABLE} (${Python3_VERSION})")
        add_test(NAME repository_consistency COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_repository.py")
        set_tests_properties(repository_consistency PROPERTIES TIMEOUT 30)
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
        add_test(NAME cli_end_to_end COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_cli.py" --app "$<TARGET_FILE:tradeup>"
            --historical-root "${HISTORICAL_ROOT}")
        set_tests_properties(cli_end_to_end PROPERTIES TIMEOUT 120)
        add_test(NAME historical_metadata COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/reconstruct_historical_metadata.py" --check
            --historical-root "${HISTORICAL_ROOT}")
        set_tests_properties(historical_metadata PROPERTIES TIMEOUT 30)
    elseif(TRADEUP_REQUIRE_PYTHON)
        message(FATAL_ERROR "Python 3.10+ is required for the full verification configuration")
    else()
        message(STATUS "Python 3.10+ unavailable: archive and differential checks omitted")
    endif()
endif()
