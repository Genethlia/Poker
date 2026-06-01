cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0091 NEW)

project(POKER LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use static MSVC runtime: /MT in Release, /MTd in Debug
# This removes MSVCP140.dll / VCRUNTIME140.dll dependencies
if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Warnings
if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Helper for static MinGW builds
function(make_mingw_static target_name)
  if (MINGW)
    target_link_options(${target_name} PRIVATE
      -static
      -static-libgcc
      -static-libstdc++
    )
  endif()
endfunction()

# ---- raylib ----
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/raylib raylib_build)

# --- Shared sources used by both client and server ---
set(SHARED_SOURCES
  deck.cpp
)

# --- Server executable ---
add_executable(server
  server.cpp
  client_in_server.cpp
  server_helpers.cpp
  server_icon.rc
  ${SHARED_SOURCES}
)

# --- Client executable ---
add_executable(client
  client.cpp
  client_in_client.cpp
  game.cpp
  chat.cpp
  cards.cpp
  images.cpp
  buttons.cpp
  uibuttons.cpp
  app_icon.rc
  mainmenu.cpp
  ${SHARED_SOURCES}
)

# Force static MSVC runtime on your targets too
if (MSVC)
  set_property(TARGET server PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )

  set_property(TARGET client PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )
endif()

# Threads
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(client PRIVATE Threads::Threads)

# Boost headers
set(BOOST_ROOT "C:\\boost\\boost_1_84_0")
target_include_directories(server PRIVATE "${BOOST_ROOT}")
target_include_directories(client PRIVATE "${BOOST_ROOT}")

# Link raylib to client
target_link_libraries(client PRIVATE raylib)

set_target_properties(client PROPERTIES OUTPUT_NAME "AllInPoker")
set_target_properties(server PROPERTIES OUTPUT_NAME "AllInPokerServer")

# Windows networking libs
if (WIN32)
  target_link_libraries(server PRIVATE ws2_32 mswsock)
  target_link_libraries(client PRIVATE ws2_32 mswsock)

  target_compile_definitions(server PRIVATE
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )

  target_compile_definitions(client PRIVATE
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )
endif()

# Static MinGW executables
make_mingw_static(server)
make_mingw_static(client)