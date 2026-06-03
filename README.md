# Cell Info Monitor

Backend-сервер для сбора и отображения данных о сотовых сетях с Android-устройства.

Проект теперь также поддерживает просмотр OpenStreetMap-тайлов в отдельном окне ImGui/ImPlot.

## Зависимости

```bash
sudo apt install -y libzmq3-dev libsdl2-dev libglew-dev nlohmann-json3-dev libpq-dev postgresql libcurl4-openssl-dev
```

ImGui и ImPlot клонируются вручную:

```bash
mkdir third_party && cd third_party
git clone https://github.com/ocornut/imgui.git
git clone https://github.com/epezent/implot.git
```

## Компиляция

```bash
mkdir build && cd build
cmake ..
make
```

## Запуск

```bash
cd build
./main
```

## Кэш OSM-таилов

- Тайлы сохраняются в папке `build/<zoom>/<x>/<y>.png`
- Если тайл уже найден на диске, он берётся из кэша и не скачивается повторно
- Если тайл отсутствует, он загружается с OSM-сервера `https://a.tile.openstreetmap.org/<z>/<x>/<y>.png`
- Загрузка выполняется асинхронно в фоне, а отрисовка обновляется, когда тайл доступен

## Архитектура

- **server.cpp** — приём данных от Android по ZMQ и обновление общей структуры `DannyeUstrojstva`
- **grafika.cpp** — создание окна SDL/OpenGL, инициализация ImGui/ImPlot и запуск визуализации
- **karta.cpp** — расчёт номера тайла, управление кэшем, загрузка PNG и отображение OSM-карты
- **curl_zapros.cpp** — загрузка тайлов с OpenStreetMap через libcurl
- **tajly.cpp** — работа с дисковым кэшем PNG и преобразование `.png` в RGBA-пиксели через stb_image
- **globalnye.cpp / globalnye.h** — общие мьютексы, атомарные флаги и разделяемые данные

## Возможности

- Приём данных LTE/GSM/NR с Android в реальном времени
- Отображение координат, высоты, точности GPS
- Графики RSRP, RSSI, RSRQ для каждого PCI отдельно
- Просмотр OpenStreetMap в окне ImGui с динамической подсветкой текущего центра карты
- Автоматическая подгрузка нескольких тайлов в зависимости от размера окна
- Кэширование тайлов на диске и повторное использование без повторного скачивания
- Асинхронная загрузка PNG-изображений в фоне
