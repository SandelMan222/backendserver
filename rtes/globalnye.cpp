#include "globalnye.h"

std::mutex mutex_dannyh;
std::atomic<bool> server_rabotaet{true};
std::map<int, HistoriyaPci> historiya_pci;
float schetchik_vremeni = 0.0f;
