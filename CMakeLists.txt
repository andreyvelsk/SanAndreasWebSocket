cmake_minimum_required(VERSION 3.21)
project(SanAndreasWebSocket VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------- #
#  Зависимости через vcpkg                                                      #
#  nlohmann-json уже сейчас; boost-beast/boost-asio добавим в шаге WebSocket   #
# ---------------------------------------------------------------------------- #
find_package(nlohmann_json CONFIG REQUIRED)

# ---------------------------------------------------------------------------- #
#  plugin-sdk — статическая библиотека из git submodule                        #
# ---------------------------------------------------------------------------- #
set(PLUGIN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plugin-sdk")

file(GLOB_RECURSE PLUGIN_SA_SOURCES
    "${PLUGIN_SDK}/plugin_sa/game_sa/*.cpp"
    "${PLUGIN_SDK}/shared/*.cpp"
)

add_library(plugin_sa STATIC ${PLUGIN_SA_SOURCES})

target_include_directories(plugin_sa PUBLIC
    "${PLUGIN_SDK}/plugin_sa"
    "${PLUGIN_SDK}/plugin_sa/game_sa"
    "${PLUGIN_SDK}/shared"
    "${PLUGIN_SDK}/shared/game"
    "${PLUGIN_SDK}/injector"
)

target_compile_definitions(plugin_sa PUBLIC
    GTASA
    _CRT_SECURE_NO_WARNINGS
    _CRT_NON_CONFORMING_SWPRINTFS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
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
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".asi"
    PREFIX ""
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    plugin_sa
    nlohmann_json::nlohmann_json
    ws2_32
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GTASA
    _WIN32_WINNT=0x0601
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
