cmake_minimum_required(VERSION 3.16)
project(TheGreatRecomp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W3 /fp:fast /MP)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -O2 -ffast-math)
endif()

# Logs any recompiled function that returns with a callee-saved register
# changed - i.e. a missing entry point. Off by default, it is not cheap.
option(ABI_CHECK "Check callee-saved registers across indirect calls" OFF)
if(ABI_CHECK)
    add_compile_definitions(DCRECOMP_ABI_CHECK)
endif()

# ========== dcrecomp framework (submodule) ==========
add_subdirectory(dcrecomp)

# ========== Recompiled game code ==========
file(GLOB GAME_SOURCES src/game/game_code_*.c)

if(GAME_SOURCES)
    list(APPEND GAME_SOURCES src/game/dispatch_table.c src/game/game_stubs.c)

    # Split into batches for parallel compilation
    set(GAME_LIBS)
    set(BATCH_SIZE 6)
    list(LENGTH GAME_SOURCES GAME_SOURCE_COUNT)
    math(EXPR NUM_BATCHES "(${GAME_SOURCE_COUNT} + ${BATCH_SIZE} - 1) / ${BATCH_SIZE}")

    set(idx 0)
    foreach(batch_num RANGE 0 ${NUM_BATCHES})
        set(BATCH_SOURCES)
        foreach(i RANGE 1 ${BATCH_SIZE})
            if(idx LESS GAME_SOURCE_COUNT)
                list(GET GAME_SOURCES ${idx} src)
                list(APPEND BATCH_SOURCES ${src})
                math(EXPR idx "${idx} + 1")
            endif()
        endforeach()
        if(BATCH_SOURCES)
            set(lib_name r2_game_${batch_num})
            add_library(${lib_name} STATIC ${BATCH_SOURCES})
            target_include_directories(${lib_name} PRIVATE
                ${CMAKE_SOURCE_DIR}/include
                ${DCRECOMP_INCLUDE_DIR}
            )
            if(MSVC)
                target_compile_options(${lib_name} PRIVATE /O1 /Zi)
            endif()
            list(APPEND GAME_LIBS ${lib_name})
        endif()
    endforeach()
    message(STATUS "Game source files: ${GAME_SOURCE_COUNT}")
else()
    message(STATUS "No recompiled game code yet - run tools first (see README.md)")
    set(GAME_LIBS)
endif()

# ========== Main executable ==========
add_executable(rayman2 src/main.c)
target_include_directories(rayman2 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${DCRECOMP_INCLUDE_DIR}
)
target_link_libraries(rayman2 dcrecomp_core ${GAME_LIBS})

# Large stack for deep recompiled call chains
if(MSVC)
    target_link_options(rayman2 PRIVATE /STACK:536870912)
else()
    target_link_options(rayman2 PRIVATE -Wl,-z,stacksize=67108864)
endif()

# SDL2main needed for WinMain entry point
find_package(SDL2 QUIET CONFIG)
if(SDL2_FOUND AND TARGET SDL2::SDL2main)
    target_link_libraries(rayman2 SDL2::SDL2main)
endif()

if(UNIX)
    target_link_libraries(rayman2 m)
endif()
