**Вот улучшенная и красиво оформленная версия файла `README.md`:**

```markdown
# Cell Info Monitor

**Backend-сервер** для сбора, логирования и визуализации данных о сотовых сетях (LTE, GSM, NR) с Android-устройств с интерактивной картой OpenStreetMap.

---

## Возможности

- Приём данных LTE/GSM/NR в реальном времени по протоколу **ZeroMQ**
- Отображение координат, высоты и точности GPS
- Интерактивные графики **RSRP, RSSI, RSRQ** для каждого PCI через **ImPlot**
- Интерактивная карта **OpenStreetMap** в отдельном окне ImGui
- Динамическая подгрузка тайлов в зависимости от масштаба и размера окна
- Кэширование тайлов на диске (работает без интернета)
- Сохранение всей истории измерений в **PostgreSQL**
- **Генерация тепловых карт** методом IDW интерполяции (RSRP, RSRQ, RSSI, Altitude)

---

## Архитектура

Проект разделён на независимые модули:

| Модуль                  | Описание |
|------------------------|----------|
| `types.h`              | Общие структуры данных (`LteCell`, `GsmCell`, `NrCell`, `Location` и др.) |
| `db.h / db.cpp`        | Работа с PostgreSQL (подключение и вставка данных) |
| `json_loader.h / cpp`  | Парсинг JSON и работа с историей |
| `zmq_server.h / cpp`   | ZeroMQ-сервер (порт 5555) |
| `osm_map.h / cpp`      | Движок карты: загрузка, кэширование и отрисовка тайлов |
| `gui.h / cpp`          | Графический интерфейс (SDL2 + ImGui + ImPlot) |
| `heatmap_model.h`      | Структуры и параметры для генерации тепловых карт |
| `heatmap_engine.h / cpp` | Генерация тепловых карт методом IDW интерполяции |
| `main.cpp`             | Запуск сервера и GUI в отдельных потоках |

---

## Установка зависимостей

```bash
sudo apt update
sudo apt install -y \
    libzmq3-dev \
    libsdl2-dev \
    libglew-dev \
    nlohmann-json3-dev \
    libpq-dev \
    postgresql \
    libcurl4-openssl-dev \
    build-essential \
    cmake
```

### Клонирование библиотек интерфейса

```bash
mkdir -p third_party && cd third_party

git clone https://github.com/ocornut/imgui.git
git clone https://github.com/epezent/implot.git

# stb_image (если нет)
mkdir -p stb && cd stb
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

---

## Сборка проекта

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Запуск

### 1. Настройка базы данных

```bash
# Создание базы данных (один раз)
sudo -u postgres createdb cellinfo
sudo -u postgres psql -c "ALTER USER postgres WITH PASSWORD 'postgres';"
```

### 2. Запуск приложения

```bash
cd build
./cell_monitor
```

---

## Кэш OSM-тайлов

Тайлы автоматически сохраняются по пути:

```
tiles/<zoom>/<x>/<y>.png
```

- При наличии файла — используется кэш (работает оффлайн).
- При отсутствии — загружается с зеркала `tile.openstreetmap.fr/osmfr`.
- Загрузка происходит асинхронно и не тормозит интерфейс.

---

## Технологии

- **C++17**
- **ZeroMQ** — передача данных
- **ImGui + ImPlot** — интерфейс и графики
- **SDL2 + OpenGL** — рендеринг
- **PostgreSQL** — хранение истории
- **libcurl** — загрузка тайлов
- **OpenStreetMap** — картографическая подложка

---

## Генерация тепловых карт

### Алгоритм IDW (Inverse Distance Weighting)

Тепловые карты генерируются методом обратно взвешенных расстояний:

```
value(x) = Σ(w_i * value_i) / Σ(w_i)
где w_i = 1 / distance(x, x_i)²
```

### Поддерживаемые критерии

1. **RSRP** (Received Signal Strength Power)
   - Отличный > -80 dBm (красный)
   - Хороший -80 до -90 dBm (оранжевый)
   - Нормальный -90 до -100 dBm (жёлтый)
   - Слабый -100 до -110 dBm (голубой)
   - Очень слабый < -110 dBm (тёмно-синий)

2. **RSRQ** (Reference Signal Received Quality)
   - Градиент от красного (плохое качество) к зелёному (отличное качество)

3. **RSSI** (Received Signal Strength Indicator)
   - Цветовой градиент на основе интенсивности

4. **Altitude** (Высота)
   - Градиент на основе высоты расположения точки

### Параметры генерации

| Параметр          | Диапазон | По умолчанию | Описание |
|------------------|----------|-------------|---------|
| Kriteriy          | —        | RSRP       | Критерий для тепловой карты |
| EARFCN            | > 0      | 100        | Частотный канал |
| Radius (m)        | 10-100   | 25         | Радиус учёта точек в метрах |
| Width (px)        | 256-1024 | 512        | Ширина генерируемого изображения |
| Height (px)       | 256-1024 | 512        | Высота генерируемого изображения |
| Generate per Tile | ON/OFF   | OFF        | Генерировать для каждого тайла |

### Использование интерфейса

1. Выберите критерий (RSRP/RSRQ/RSSI/Altitude)
2. Введите EARFCN из доступных LTE-ячеек
3. Настройте радиус интерполяции (10-40 метров рекомендуется)
4. Выберите размер изображения (512x512 или 1024x1024)
5. Нажмите "Start Generation"

### Сохранение результатов

Тепловые карты сохраняются в формате **PNG** с поддержкой альфа-канала:

```
./build/heatmap_RSRP_100.png
./build/heatmap_RSRQ_100.png
./build/heatmap_RSSI_6175.png
./build/heatmap_Vysota_100.png
```

---