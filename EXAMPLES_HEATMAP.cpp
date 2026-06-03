#include "heatmap_engine.h"
#include "db.h"

// ============================================================
// ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ API ГЕНЕРАЦИИ ТЕПЛОВЫХ КАРТ
// ============================================================

// ПРИМЕР 1: Генерация тепловой карты RSRP с параметрами по умолчанию
void primer_1_prostoy_zapusk() {
    ParametryGeneracii parametry;
    parametry.kriteriy = KriteriyTeplovoyKarty::RSRP;
    parametry.earfcn = 100;
    parametry.radiusMetry = 25;
    parametry.shirinaKarty = 512;
    parametry.vysotaKarty = 512;
    parametry.dlyaKazhdogoTaylya = false;
    parametry.putySohraneniya = "./build";

    zapustit_generaciyu_v_potoke(parametry);
}

// ПРИМЕР 2: Генерация с разными критериями последовательно
void primer_2_multikriteriy() {
    std::vector<KriteriyTeplovoyKarty> kriterity = {
        KriteriyTeplovoyKarty::RSRP,
        KriteriyTeplovoyKarty::RSRQ,
        KriteriyTeplovoyKarty::RSSI,
        KriteriyTeplovoyKarty::Vysota
    };

    for (auto kriteriy : kriterity) {
        ParametryGeneracii par;
        par.kriteriy = kriteriy;
        par.earfcn = 100;
        par.radiusMetry = 30;
        par.shirinaKarty = 512;
        par.vysotaKarty = 512;
        par.putySohraneniya = "./build";

        zapustit_generaciyu_v_potoke(par);

        // Ждём завершения текущей генерации перед следующей
        while (statusGeneracii.vypolnyaetsya) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

// ПРИМЕР 3: Высокое разрешение с бóльшим радиусом
void primer_3_vysokoye_razresheniye() {
    ParametryGeneracii parametry;
    parametry.kriteriy = KriteriyTeplovoyKarty::RSRP;
    parametry.earfcn = 100;
    parametry.radiusMetry = 40;  // Большой радиус
    parametry.shirinaKarty = 1024;  // Высокое разрешение
    parametry.vysotaKarty = 1024;
    parametry.dlyaKazhdogoTaylya = false;
    parametry.putySohraneniya = "./build/high_res";

    zapustit_generaciyu_v_potoke(parametry);
}

// ПРИМЕР 4: Работа со статусом генерации
void primer_4_monitoring_statusa() {
    ParametryGeneracii parametry;
    parametry.kriteriy = KriteriyTeplovoyKarty::RSRQ;
    parametry.earfcn = 6175;
    parametry.radiusMetry = 25;
    parametry.shirinaKarty = 512;
    parametry.vysotaKarty = 512;
    parametry.putySohraneniya = "./build";

    zapustit_generaciyu_v_potoke(parametry);

    // Отслеживаем прогресс
    while (true) {
        {
            std::lock_guard<std::mutex> lock(statusGeneracii.mtx);
            if (!statusGeneracii.vypolnyaetsya) {
                printf("Генерация завершена: %s\n", statusGeneracii.soobshchenie.c_str());
                break;
            }
            printf("Прогресс: %.1f%%\n", statusGeneracii.progress * 100.0f);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// ПРИМЕР 5: Останавливаем генерацию если она заняла слишком много времени
void primer_5_timeoutgeneration() {
    ParametryGeneracii parametry;
    parametry.kriteriy = KriteriyTeplovoyKarty::RSSI;
    parametry.earfcn = 100;
    parametry.radiusMetry = 25;
    parametry.shirinaKarty = 1024;
    parametry.vysotaKarty = 1024;
    parametry.putySohraneniya = "./build";

    zapustit_generaciyu_v_potoke(parametry);

    // Таймаут: 10 секунд
    auto nachalo = std::chrono::high_resolution_clock::now();
    while (statusGeneracii.vypolnyaetsya) {
        auto teper = std::chrono::high_resolution_clock::now();
        auto proshlo = std::chrono::duration_cast<std::chrono::seconds>(teper - nachalo).count();

        if (proshlo > 10) {
            printf("Таймаут! Останавливаем генерацию...\n");
            ostanavliv_generaciyu();
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

// ПРИМЕР 6: Получение данных из БД и проверка перед генерацией
void primer_6_proverka_dannyh() {
    int earfcn = 100;
    std::string kriteriy = "RSRP";

    auto dannye = poluchit_dannye_iz_bd(earfcn, kriteriy);

    if (dannye.empty()) {
        printf("ОШИБКА: Нет данных для EARFCN=%d, критерий=%s\n", earfcn, kriteriy.c_str());
        return;
    }

    printf("Загружено %zu точек\n", dannye.size());

    // Найти диапазон значений
    double minVal = dannye[0].znachenie;
    double maxVal = dannye[0].znachenie;
    for (const auto& t : dannye) {
        minVal = std::min(minVal, t.znachenie);
        maxVal = std::max(maxVal, t.znachenie);
    }

    printf("Диапазон значений: %.1f до %.1f\n", minVal, maxVal);

    // Теперь генерируем карту
    ParametryGeneracii parametry;
    parametry.kriteriy = KriteriyTeplovoyKarty::RSRP;
    parametry.earfcn = earfcn;
    parametry.radiusMetry = 25;
    parametry.shirinaKarty = 512;
    parametry.vysotaKarty = 512;
    parametry.putySohraneniya = "./build";

    zapustit_generaciyu_v_potoke(parametry);
}

// ПРИМЕР 7: Обработка разных EARFCN
void primer_7_neskolko_earfcn() {
    std::vector<int> earfcny = {100, 6175, 6400};

    for (int earfcn : earfcny) {
        ParametryGeneracii par;
        par.kriteriy = KriteriyTeplovoyKarty::RSRP;
        par.earfcn = earfcn;
        par.radiusMetry = 25;
        par.shirinaKarty = 512;
        par.vysotaKarty = 512;
        par.putySohraneniya = "./build";

        printf("Генерируем для EARFCN = %d\n", earfcn);
        zapustit_generaciyu_v_potoke(par);

        while (statusGeneracii.vypolnyaetsya) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

// ПРИМЕР 8: Прямое использование функций интерполяции
void primer_8_direktnoe_ispolzovanie() {
    // Получаем данные
    auto dannye = poluchit_dannye_iz_bd(100, "RSRP");
    if (dannye.empty()) return;

    // Вычисляем значение в конкретной точке
    double shirota = 55.0;
    double dolgota = 82.9;
    int radiusMetry = 25;

    double znachenie = poluchit_znachenie_idw(shirota, dolgota, dannye, radiusMetry);
    printf("Значение в (%.4f, %.4f): %.1f dBm\n", shirota, dolgota, znachenie);

    // Конвертируем в цвет
    TsvetnayaMapa tsvet = konvertirovat_v_tsvet(znachenie, KriteriyTeplovoyKarty::RSRP);
    printf("Цвет: RGB(%d, %d, %d)\n", tsvet.r, tsvet.g, tsvet.b);
}

// ПРИМЕР 9: Расстояние между точками
void primer_9_rasstoyanie() {
    // Новосибирск и Москва (примерно)
    double lat1 = 55.0396, lon1 = 82.9107;  // Новосибирск
    double lat2 = 55.7558, lon2 = 37.6173;  // Москва

    double distMetry = vychislit_rasstoyanie(lat1, lon1, lat2, lon2);
    printf("Расстояние: %.0f км\n", distMetry / 1000.0);
}

// ПРИМЕР 10: Параллельная генерация для нескольких критериев (без ожидания)
void primer_10_parallelnyy_zapusk() {
    std::vector<KriteriyTeplovoyKarty> kriterity = {
        KriteriyTeplovoyKarty::RSRP,
        KriteriyTeplovoyKarty::RSRQ,
        KriteriyTeplovoyKarty::RSSI,
        KriteriyTeplovoyKarty::Vysota
    };

    // Запускаем все генерации одновременно (если хватает памяти)
    for (auto kriteriy : kriterity) {
        ParametryGeneracii par;
        par.kriteriy = kriteriy;
        par.earfcn = 100;
        par.radiusMetry = 25;
        par.shirinaKarty = 512;
        par.vysotaKarty = 512;
        par.putySohraneniya = "./build";

        // Запускаем без ожидания
        zapustit_generaciyu_v_potoke(par);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Ждём завершения последней
    while (statusGeneracii.vypolnyaetsya) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================

// Получить текущий статус генерации
void pokazat_status() {
    std::lock_guard<std::mutex> lock(statusGeneracii.mtx);
    printf("Активна: %s\n", statusGeneracii.vypolnyaetsya ? "Да" : "Нет");
    printf("Прогресс: %.1f%%\n", statusGeneracii.progress * 100.0f);
    printf("Сообщение: %s\n", statusGeneracii.soobshchenie.c_str());
}

// Проверить наличие файла результата
bool rezultat_gotov(const std::string& kriteriyNazv, int earfcn) {
    std::string putiFile = "./build/heatmap_" + kriteriyNazv + "_" + std::to_string(earfcn) + ".png";
    return std::ifstream(putiFile).good();
}

// ============================================================
// ИСПОЛЬЗОВАНИЕ В MAIN
// ============================================================
/*
int main() {
    // Выбираем один из примеров:
    
    primer_1_prostoy_zapusk();
    // или
    // primer_2_multikriteriy();
    // или
    // primer_3_vysokoye_razresheniye();
    // или
    // primer_4_monitoring_statusa();
    // и т.д.

    return 0;
}
*/
