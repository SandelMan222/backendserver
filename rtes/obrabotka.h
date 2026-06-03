#pragma once
#include <libpq-fe.h>
#include <nlohmann/json.hpp>
#include "struktury.h"

void obrabotatJson(const nlohmann::json& j, DannyeUstrojstva* dannye, PGconn* soedinenie);
void zagruzitFajl(DannyeUstrojstva* dannye);
