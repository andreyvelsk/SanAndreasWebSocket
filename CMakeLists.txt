cmake_minimum_required(VERSION 3.21)
project(SanAndreasWebSocket VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------- #
#  Зависимости через vcpkg                                                      #
# ---------------------------------------------------------------------------- #
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# ---------------------------------------------------------------------------- #
#  plugin-sdk — статическая библиотека из git submodule                        #
# ---------------------------------------------------------------------------- #
set(PLUGIN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plugin-sdk")

file(GLOB_RECURSE PLUGIN_SA_SOURCES
    "${PLUGIN_SDK}/plugin_sa/game_sa/*.cpp"
    "${PLUGIN_SDK}/shared/*.cpp"
    "${PLUGIN_SDK}/safetyhook/safetyhook.cpp"
    "${PLUGIN_SDK}/safetyhook/Zydis.c"
)

add_library(plugin_sa STATIC ${PLUGIN_SA_SOURCES})

target_include_directories(plugin_sa PUBLIC
    "${PLUGIN_SDK}/plugin_sa"
    "${PLUGIN_SDK}/plugin_sa/game_sa"
    "${PLUGIN_SDK}/plugin_sa/game_sa/rw"
    "${PLUGIN_SDK}/plugin_sa/game_sa/enums"
    "${PLUGIN_SDK}/plugin_sa/game_sa/meta"
    "${PLUGIN_SDK}/shared"
    "${PLUGIN_SDK}/shared/game"
    "${PLUGIN_SDK}/shared/extensions"
    "${PLUGIN_SDK}/shared/extender"
    "${PLUGIN_SDK}/injector"
    "${PLUGIN_SDK}/safetyhook"
)

target_compile_definitions(plugin_sa PUBLIC
    GTASA
    RW
    _WIN32_WINNT=0x0601
    _CRT_SECURE_NO_WARNINGS
    _CRT_NON_CONFORMING_SWPRINTFS
    NOMINMAX
)

# winsock2.h ДОЛЖЕН идти раньше windows.h (иначе конфликт winsock1/winsock2 с boost::asio)
# windows.h без WIN32_LEAN_AND_MEAN даёт byte через rpcndr.h (нужен game_sa)
# Используем /FI<file> без пробела — CMake иначе дедуплицирует два одинаковых /FI
target_compile_options(plugin_sa PUBLIC
    "/FIwinsock2.h"
    "/FIwindows.h"
)

# plugin-sdk использует старые касты указателей — отключаем соответствующие предупреждения
target_compile_options(plugin_sa PRIVATE
    /wd4302 /wd4311 /wd4312  # pointer truncation / cast
    /wd4244 /wd4267          # conversion / size_t narrowing
    /wd4100                  # unreferenced formal parameter
    /wd4189                  # local variable initialized but not referenced
)

# ---------------------------------------------------------------------------- #
#  Наш мод — DLL с расширением .asi                                            #
# ---------------------------------------------------------------------------- #
add_library(${PROJECT_NAME} SHARED
    src/Main.cpp
    src/GameThread.cpp
    src/server/WsServer.cpp
    src/server/WsSession.cpp
    src/protocol/FieldRegistry.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".asi"
    PREFIX ""
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/protocol"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    plugin_sa
    nlohmann_json::nlohmann_json
    Boost::system
    ws2_32
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GTASA
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
