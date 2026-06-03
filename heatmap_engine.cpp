#include "heatmap_engine.h"
#include "db.h"
#include "types.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

std::mutex mtkaryty;
std::atomic<bool> zaprosStopaGeneration(false);
StatusGeneracii statusGeneracii{false, 0.0f, "Готово", {}};

double vychislit_rasstoyanie(double lat1, double lon1, double lat2, double lon2) {
    const double piRad = 3.14159265359 / 180.0;
    const double radiusZemli = 6371000.0;

    double dLat = (lat2 - lat1) * piRad;
    double dLon = (lon2 - lon1) * piRad;

    double sin_dLat_2 = sin(dLat / 2.0);
    double sin_dLon_2 = sin(dLon / 2.0);

    double a = sin_dLat_2 * sin_dLat_2 +
               cos(lat1 * piRad) * cos(lat2 * piRad) *
               sin_dLon_2 * sin_dLon_2;

    double c = 2.0 * asin(sqrt(a));
    return radiusZemli * c;
}

double poluchit_znachenie_idw(
    double shirota, double dolgota,
    const std::vector<TochkaIzmereniya>& dannye,
    int radiusMetry
) {
    if (dannye.empty()) return -999.0;

    double chislitel = 0.0;
    double znamenatel = 0.0;
    int schetchikTochekVRadiuse = 0;

    for (const auto& tochka : dannye) {
        double dist = vychislit_rasstoyanie(shirota, dolgota, tochka.shirota, tochka.dolgota);

        if (dist > radiusMetry) continue;

        schetchikTochekVRadiuse++;

        if (dist < 0.1) {
            return tochka.znachenie;
        }

        double ves = 1.0 / (dist * dist);
        chislitel += ves * tochka.znachenie;
        znamenatel += ves;
    }

    if (schetchikTochekVRadiuse == 0) return -999.0;
    if (znamenatel == 0.0) return -999.0;

    return chislitel / znamenatel;
}

TsvetnayaMapa konvertirovat_v_tsvet(double znachenie, KriteriyTeplovoyKarty kriteriy) {
    TsvetnayaMapa tsvet{0, 0, 0};

    if (znachenie < -900) {
        return {0, 0, 0};
    }

    switch (kriteriy) {
        case KriteriyTeplovoyKarty::RSRP: {
            if (znachenie > -80) {
                tsvet = {255, 0, 0};
            } else if (znachenie > -90) {
                tsvet = {255, 127, 0};
            } else if (znachenie > -100) {
                tsvet = {255, 255, 0};
            } else if (znachenie > -110) {
                tsvet = {0, 127, 255};
            } else {
                tsvet = {0, 0, 128};
            }
            break;
        }
        case KriteriyTeplovoyKarty::RSRQ: {
            double normalizovano = (znachenie + 20.0) / 20.0;
            normalizovano = std::max(0.0, std::min(1.0, normalizovano));

            tsvet.r = static_cast<int>(255 * (1.0 - normalizovano));
            tsvet.g = static_cast<int>(255 * normalizovano);
            tsvet.b = 127;
            break;
        }
        case KriteriyTeplovoyKarty::RSSI: {
            double normalizovano = (znachenie + 120.0) / 60.0;
            normalizovano = std::max(0.0, std::min(1.0, normalizovano));

            tsvet.r = static_cast<int>(255 * normalizovano);
            tsvet.g = static_cast<int>(255 * (1.0 - normalizovano));
            tsvet.b = 100;
            break;
        }
        case KriteriyTeplovoyKarty::Vysota: {
            double normalizovano = znachenie / 1000.0;
            normalizovano = std::max(0.0, std::min(1.0, normalizovano));

            tsvet.r = static_cast<int>(255 * normalizovano);
            tsvet.g = 150;
            tsvet.b = static_cast<int>(255 * (1.0 - normalizovano));
            break;
        }
    }

    return tsvet;
}

void generateheatmap(const ParametryGeneracii& parametry) {
    zaprosStopaGeneration = false;

    {
        std::lock_guard<std::mutex> lock(statusGeneracii.mtx);
        statusGeneracii.vypolnyaetsya = true;
        statusGeneracii.progress = 0.0f;
        statusGeneracii.soobshchenie = "Loading data...";
    }

    std::string kriteriyNazv;
    switch (parametry.kriteriy) {
        case KriteriyTeplovoyKarty::RSRP: kriteriyNazv = "rsrp"; break;
        case KriteriyTeplovoyKarty::RSRQ: kriteriyNazv = "rsrq"; break;
        case KriteriyTeplovoyKarty::RSSI: kriteriyNazv = "rssi"; break;
        case KriteriyTeplovoyKarty::Vysota: kriteriyNazv = "altitude"; break;
    }

    auto dannye = poluchit_dannye_iz_bd(parametry.earfcn, kriteriyNazv);
    if (dannye.empty()) {
        std::lock_guard<std::mutex> lock(statusGeneracii.mtx);
        statusGeneracii.vypolnyaetsya = false;
        statusGeneracii.soobshchenie = "No data for EARFCN";
        return;
    }

    // Вычисляем границы и параметры проекции (метры/градус)
    double minShirota = dannye[0].shirota, maxShirota = dannye[0].shirota;
    double minDolgota = dannye[0].dolgota, maxDolgota = dannye[0].dolgota;
    for (const auto& t : dannye) {
        minShirota = std::min(minShirota, t.shirota);
        maxShirota = std::max(maxShirota, t.shirota);
        minDolgota = std::min(minDolgota, t.dolgota);
        maxDolgota = std::max(maxDolgota, t.dolgota);
    }

    double midLat = (minShirota + maxShirota) / 2.0;
    double metersPerDegLon = 111320.0 * cos(midLat * 3.14159265359 / 180.0);
    double metersPerDegLat = 110574.0;

    // Пространственный индекс: сетка с размером ячейки = radius
    double cellSize = std::max(1.0, (double)parametry.radiusMetry);
    using Key = long long;
    auto make_key = [](int xi, int yi)->Key { return ( ( (long long)xi) << 32 ) | (unsigned long long)(yi & 0xffffffff); };
    std::unordered_map<Key, std::vector<int>> grid;
    grid.reserve(dannye.size() * 2);

    std::vector<double> proj_x(dannye.size()), proj_y(dannye.size());
    for (size_t i = 0; i < dannye.size(); ++i) {
        proj_x[i] = (dannye[i].dolgota - minDolgota) * metersPerDegLon;
        proj_y[i] = (dannye[i].shirota - minShirota) * metersPerDegLat;
        int gx = (int)std::floor(proj_x[i] / cellSize);
        int gy = (int)std::floor(proj_y[i] / cellSize);
        grid[make_key(gx, gy)].push_back((int)i);
    }

    int width = parametry.shirinaKarty;
    int height = parametry.vysotaKarty;
    int channels = 4;
    std::vector<unsigned char> izobrazh(width * height * channels);

    // Потоки
    unsigned threadsN = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    std::atomic<int> rowsDone{0};

    auto worker = [&](int y0, int y1, int threadId) {
        for (int y = y0; y < y1; ++y) {
            if (zaprosStopaGeneration) return;
            double shirota = minShirota + ( (double)(height - y) * (maxShirota - minShirota) / (double)height );
            for (int x = 0; x < width; ++x) {
                double dolgota = minDolgota + ( (double)x * (maxDolgota - minDolgota) / (double)width );

                // Проекция точки в метры
                double px = (dolgota - minDolgota) * metersPerDegLon;
                double py = (shirota - minShirota) * metersPerDegLat;

                int cgx = (int)std::floor(px / cellSize);
                int cgy = (int)std::floor(py / cellSize);
                int ncell = (int)std::ceil(parametry.radiusMetry / cellSize);

                // Собираем кандидатов из соседних ячеек
                double chisl = 0.0, znam = 0.0;
                int cnt = 0;
                for (int oy = -ncell; oy <= ncell; ++oy) {
                    for (int ox = -ncell; ox <= ncell; ++ox) {
                        Key k = make_key(cgx + ox, cgy + oy);
                        auto it = grid.find(k);
                        if (it == grid.end()) continue;
                        for (int idx : it->second) {
                            double dist = vychislit_rasstoyanie(shirota, dolgota, dannye[idx].shirota, dannye[idx].dolgota);
                            if (dist > parametry.radiusMetry) continue;
                            if (dist < 0.1) { chisl = dannye[idx].znachenie; znam = 1.0; cnt = 1; break; }
                            double w = 1.0 / (dist * dist);
                            chisl += w * dannye[idx].znachenie;
                            znam += w;
                            ++cnt;
                        }
                        if (cnt == 1 && znam == 1.0 && chisl == dannye[0].znachenie) break; // micro-optim
                    }
                }

                double znachenie = (znam == 0.0) ? -999.0 : (chisl / znam);
                TsvetnayaMapa tsvet = konvertirovat_v_tsvet(znachenie, parametry.kriteriy);
                int index = (y * width + x) * channels;
                izobrazh[index + 0] = (unsigned char)tsvet.r;
                izobrazh[index + 1] = (unsigned char)tsvet.g;
                izobrazh[index + 2] = (unsigned char)tsvet.b;
                izobrazh[index + 3] = (znachenie > -900) ? 255 : 0;
            }
            rowsDone.fetch_add(1, std::memory_order_relaxed);
        }
    };

    // Разбиваем по строкам
    int rowsPerThread = (height + threadsN - 1) / threadsN;
    for (unsigned t = 0; t < threadsN; ++t) {
        int y0 = t * rowsPerThread;
        int y1 = std::min(height, y0 + rowsPerThread);
        if (y0 >= y1) break;
        threads.emplace_back(worker, y0, y1, (int)t);
    }

    // Прогрессный монитор
    while (true) {
        if (zaprosStopaGeneration) break;
        {
            std::lock_guard<std::mutex> lock(statusGeneracii.mtx);
            statusGeneracii.progress = (float)rowsDone.load() / (float)height;
            statusGeneracii.soobshchenie = "Generating image...";
        }
        if (rowsDone.load() >= height) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (auto &th : threads) if (th.joinable()) th.join();

    fs::create_directories(parametry.putySohraneniya);
    std::string putiFayla = parametry.putySohraneniya + "/heatmap_" + kriteriyNazv + "_" + std::to_string(parametry.earfcn) + ".png";
    std::string putiMeta = parametry.putySohraneniya + "/heatmap_" + kriteriyNazv + "_" + std::to_string(parametry.earfcn) + ".json";
    if (stbi_write_png(putiFayla.c_str(), width, height, channels, izobrazh.data(), width * channels)) {
        // write metadata for GUI overlay (geo bounds)
        try {
            nlohmann::json meta;
            meta["minLat"] = minShirota;
            meta["maxLat"] = maxShirota;
            meta["minLon"] = minDolgota;
            meta["maxLon"] = maxDolgota;
            meta["width"] = width;
            meta["height"] = height;
            std::ofstream mf(putiMeta);
            if (mf.good()) mf << meta.dump();
        } catch (...) {
            // ignore metadata errors
        }
        std::lock_guard<std::mutex> lock(statusGeneracii.mtx);
        statusGeneracii.vypolnyaetsya = false;
        statusGeneracii.progress = 1.0f;
        statusGeneracii.soobshchenie = "Done: " + putiFayla;
    } else {
        std::lock_guard<std::mutex> lock(statusGeneracii.mtx);
        statusGeneracii.vypolnyaetsya = false;
        statusGeneracii.soobshchenie = "File write error";
    }
}

void zapustit_generaciyu_v_potoke(const ParametryGeneracii& parametry) {
    zaprosStopaGeneration = false;
    std::thread t(generateheatmap, parametry);
    t.detach();
}

void ostanavliv_generaciyu() {
    zaprosStopaGeneration = true;
}