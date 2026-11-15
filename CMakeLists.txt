cmake_minimum_required(VERSION 3.14)
project(SpeedSpeech VERSION 0.1.0 LANGUAGES C CXX)

option(SPEEDSPEECH_BUILD_SHARED "Build the shared library (used by the Unity plugin)" ON)
option(SPEEDSPEECH_BUILD_CLI "Build the command line tool" ON)
option(SPEEDSPEECH_BUILD_TESTS "Build the tests" ON)

set(SS_SOURCES
    core/src/ss_engine.c
    core/src/ss_klatt.c
    core/src/ss_lexicon.c
    core/src/ss_phonemes.c
    core/src/ss_prosody.c
    core/src/ss_rules.c
    core/src/ss_text.c
)

if(MSVC)
    set(SS_WARNINGS /W4)
else()
    set(SS_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)
endif()

find_library(SS_LIBM m)

function(ss_configure target)
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>
        $<INSTALL_INTERFACE:include>)
    target_compile_options(${target} PRIVATE ${SS_WARNINGS})
    set_target_properties(${target} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
    if(SS_LIBM)
        target_link_libraries(${target} PUBLIC ${SS_LIBM})
    endif()
endfunction()

# Static library: link into your own engine / tools.
add_library(speedspeech_static STATIC ${SS_SOURCES})
ss_configure(speedspeech_static)
set_target_properties(speedspeech_static PROPERTIES OUTPUT_NAME speedspeech_static POSITION_INDEPENDENT_CODE ON)

# Shared library: the native plugin loaded by Unity (speedspeech.dll / libspeedspeech.so / .dylib).
if(SPEEDSPEECH_BUILD_SHARED)
    add_library(speedspeech SHARED ${SS_SOURCES})
    ss_configure(speedspeech)
    target_compile_definitions(speedspeech PRIVATE SS_BUILD_SHARED)
    set_target_properties(speedspeech PROPERTIES
        C_VISIBILITY_PRESET hidden
        MACOSX_RPATH ON)
    if(WIN32)
        set_target_properties(speedspeech PROPERTIES PREFIX "")
    endif()
    if(APPLE)
        # Unity on macOS loads native plugins as .bundle (or .dylib)
        set_target_properties(speedspeech PROPERTIES SUFFIX ".dylib")
    endif()
endif()

if(SPEEDSPEECH_BUILD_CLI)
    add_executable(speedspeech-cli tools/speedspeech_cli.c)
    target_link_libraries(speedspeech-cli PRIVATE speedspeech_static)
    target_compile_options(speedspeech-cli PRIVATE ${SS_WARNINGS})
    set_target_properties(speedspeech-cli PROPERTIES C_STANDARD 99)
endif()

if(SPEEDSPEECH_BUILD_TESTS)
    enable_testing()
    add_executable(speedspeech-tests tests/test_main.c)
    target_link_libraries(speedspeech-tests PRIVATE speedspeech_static)
    target_include_directories(speedspeech-tests PRIVATE core/src)
    target_compile_options(speedspeech-tests PRIVATE ${SS_WARNINGS})
    set_target_properties(speedspeech-tests PROPERTIES C_STANDARD 99)
    add_test(NAME unit COMMAND speedspeech-tests)

    # The Unreal plugin compiles the core as C++ in one translation unit:
    # make sure that keeps working.
    add_executable(speedspeech-cpp-check tests/test_cpp_amalgamation.cpp)
    target_include_directories(speedspeech-cpp-check PRIVATE core/include core/src)
    target_compile_options(speedspeech-cpp-check PRIVATE ${SS_WARNINGS})
    set_target_properties(speedspeech-cpp-check PROPERTIES CXX_STANDARD 11)
    if(SS_LIBM)
        target_link_libraries(speedspeech-cpp-check PRIVATE ${SS_LIBM})
    endif()
    add_test(NAME cpp_amalgamation COMMAND speedspeech-cpp-check)
endif()

include(GNUInstallDirs)
install(TARGETS speedspeech_static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(SPEEDSPEECH_BUILD_SHARED)
    install(TARGETS speedspeech
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
if(SPEEDSPEECH_BUILD_CLI)
    install(TARGETS speedspeech-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(FILES core/include/speedspeech.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
