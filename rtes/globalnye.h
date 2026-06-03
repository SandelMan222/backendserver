#pragma once
#include <mutex>
#include <atomic>
#include <map>
#include "struktury.h"

extern std::mutex mutex_dannyh;
extern std::atomic<bool> server_rabotaet;
extern std::map<int, HistoriyaPci> historiya_pci;
extern float schetchik_vremeni;

const int MAKS_TOCHEK = 100000;
