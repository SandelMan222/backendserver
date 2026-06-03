#pragma once

#include <nlohmann/json.hpp>
#include "db.h"
#include "types.h"

void obrabotat_json(const nlohmann::json& j, DeviceData* data, PGconn* conn);
void zagruzit_fayl_json(DeviceData* data);