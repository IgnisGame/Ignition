cmake_minimum_required(VERSION 3.20)
project(Ignition LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IGNITION_AS_SCRIPT_MODULE "Build Ignition as runtime-loaded script module (DLL/.so)" ON)
add_subdirectory(Ignis)

if(NOT TARGET Ignis::Ignis AND TARGET Ignis)
    add_library(Ignis::Ignis ALIAS Ignis)
endif()

file(GLOB_RECURSE IGNITION_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/scripts/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/scripts/*.cpp
)

if(IGNITION_AS_SCRIPT_MODULE)
    add_library(Ignition SHARED ${IGNITION_SOURCES})
    target_compile_definitions(Ignition PRIVATE IGNITION_DLL_EXPORT=1)
else()
    add_library(Ignition STATIC ${IGNITION_SOURCES})
endif()

set_target_properties(Ignition PROPERTIES
    DEBUG_POSTFIX ""
    RELWITHDEBINFO_POSTFIX ""
    MINSIZEREL_POSTFIX ""
)

target_link_libraries(Ignition PRIVATE Ignis::Ignis)

target_include_directories(Ignition PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts
)

if(MSVC)
    target_compile_options(Ignition PRIVATE /utf-8)
endif()

if(IGNITION_AS_SCRIPT_MODULE)
    set(_IGNITION_PLATFORM "$<IF:$<PLATFORM_ID:Windows>,Windows,$<IF:$<PLATFORM_ID:Darwin>,macOS,Linux>>")
    set(_IGNITION_OUTDIR "${CMAKE_SOURCE_DIR}/bin/${_IGNITION_PLATFORM}/$<CONFIG>")

    set_target_properties(Ignition PROPERTIES
        OUTPUT_NAME "Ignition"

        RUNTIME_OUTPUT_DIRECTORY "${_IGNITION_OUTDIR}"   # Windows .dll
        LIBRARY_OUTPUT_DIRECTORY "${_IGNITION_OUTDIR}"   # macOS .dylib / Linux .so
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${_IGNITION_PLATFORM}/$<CONFIG>/lib"
    )

    if(WIN32)
        set_target_properties(Ignition PROPERTIES PREFIX "")
    endif()
endif()