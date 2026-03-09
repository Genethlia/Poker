cmake_minimum_required(VERSION 3.16)
project(POKER LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/raylib raylib_build)

# --- Shared sources used by both client and server ---
set(SHARED_SOURCES
  deck.cpp
)

# --- Server executable ---
add_executable(server
  server.cpp
  client_in_server.cpp
  ${SHARED_SOURCES}
)

# --- Client executable ---
add_executable(client
  client.cpp
  client_in_client.cpp
  raylib_client.cpp
  ${SHARED_SOURCES}
)

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