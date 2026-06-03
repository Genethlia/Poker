cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0091 NEW)

project(POKER LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use static MSVC runtime: /MT in Release, /MTd in Debug
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

# ---- One combined executable ----
add_executable(AllInPoker
  # New main file for the full app
  main.cpp

  # Server-side code
  # Only include server.cpp if it does NOT contain int main()
  server.cpp
  client_in_server.cpp
  server_helpers.cpp

  # Client-side code
  client_in_client.cpp
  game.cpp
  chat.cpp
  cards.cpp
  images.cpp
  buttons.cpp
  uibuttons.cpp
  mainmenu.cpp

  # Shared code
  deck.cpp

  # Icon
  app_icon.rc
)

# Force static MSVC runtime
if (MSVC)
  set_property(TARGET AllInPoker PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )
endif()

# Threads
find_package(Threads REQUIRED)
target_link_libraries(AllInPoker PRIVATE Threads::Threads)

# Boost headers
set(BOOST_ROOT "C:\\boost\\boost_1_84_0")
target_include_directories(AllInPoker PRIVATE "${BOOST_ROOT}")

# Link libraries
target_link_libraries(AllInPoker PRIVATE raylib)

if (WIN32)
  target_link_libraries(AllInPoker PRIVATE
    ws2_32
    mswsock
    iphlpapi
  )

  target_compile_definitions(AllInPoker PRIVATE
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )
endif()

# Static MinGW executable
make_mingw_static(AllInPoker)