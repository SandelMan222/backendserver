# Cell Info Monitor

Backend-сервер для сбора и отображения данных о сотовых сетях с Android-устройства.

## Зависимости

```bash
sudo apt install -y libzmq3-dev libsdl2-dev libglew-dev nlohmann-json3-dev libpq-dev postgresql
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

## База данных

```bash
sudo service postgresql start
sudo -u postgres psql -c "CREATE DATABASE cellinfo;"
sudo -u postgres psql -d cellinfo -c "CREATE TABLE lte_cells (id SERIAL PRIMARY KEY, timestamp TEXT, latitude DOUBLE PRECISION, longitude DOUBLE PRECISION, altitude DOUBLE PRECISION, accuracy DOUBLE PRECISION, band TEXT, cid INTEGER, earfcn INTEGER, mcc TEXT, mnc TEXT, pci INTEGER, tac INTEGER, asu INTEGER, cqi INTEGER, rsrp INTEGER, rsrq INTEGER, rssi INTEGER, rssnr INTEGER, timing_advance INTEGER);"
```

## Запуск

```bash
cd build
./main
```

## Архитектура

- **run\_server** — ZMQ REP-сокет, принимает JSON от Android, пишет в PostgreSQL и `locations.json`
- **run\_gui** — ImGui + ImPlot, отображает текущие данные и графики сигнала по PCI
- Общая структура `DeviceData` защищена `std::mutex`
- При старте автоматически загружает `locations.json` если файл существует

## Возможности

- Приём данных LTE/GSM/NR с Android в реальном времени
- Отображение координат, высоты, точности GPS
- Графики RSRP, RSSI, RSRQ для каждого PCI отдельным цветом
- Сохранение всех данных в PostgreSQL и JSON
- Загрузка накопленного JSON для анализа без телефона
