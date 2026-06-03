#pragma once
#include <vector>
#include <string>
#include <libpq-fe.h>
#include "types.h"
#include "heatmap_model.h"

PGconn* podklyuchit_bd();
std::string znachenie_sql(int v);
void vstavit_v_bd(PGconn* conn, const Location& loc, const std::vector<LteCell>& cells);
std::vector<TochkaIzmereniya> poluchit_dannye_iz_bd(int earfcn, const std::string& kriteriy);