cmake_minimum_required(VERSION 3.14)
project(cell_info_monitor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к сторонним библиотекам
set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third_party)

# Поиск системных пакетов
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(CURL REQUIRED)

# Пути к заголовкам
include_directories(
    .
    /usr/include/SDL2
    /usr/include/nlohmann
    /usr/include/postgresql
    ${THIRD_PARTY_DIR}/imgui
    ${THIRD_PARTY_DIR}/imgui/backends
    ${THIRD_PARTY_DIR}/implot
    ${THIRD_PARTY_DIR}/stb
)

# Сборка ImGui (статическая библиотека)
add_library(imgui STATIC
    ${THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl2.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
target_compile_options(imgui PRIVATE -fPIC)

# Сборка ImPlot (статическая библиотека)
add_library(implot STATIC
    ${THIRD_PARTY_DIR}/implot/implot.cpp
    ${THIRD_PARTY_DIR}/implot/implot_demo.cpp
    ${THIRD_PARTY_DIR}/implot/implot_items.cpp
)
target_link_libraries(implot PRIVATE imgui)
target_compile_options(implot PRIVATE -fPIC)

# Основной исполняемый файл и все его модули
add_executable(cell_monitor
    main.cpp
    db.cpp
    json_loader.cpp
    zmq_server.cpp
    osm_map.cpp
    gui.cpp
    heatmap_engine.cpp
)

# Линковка всех зависимостей
target_link_libraries(cell_monitor PRIVATE
    imgui
    implot
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${CURL_LIBRARIES}
    zmq
    pq
    pthread
    m
    dl
)