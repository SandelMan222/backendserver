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