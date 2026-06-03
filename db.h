#pragma once

#include <libpq-fe.h>
#include "types.h"

PGconn* podklyuchit_bd();
std::string znachenie_sql(int v);
void vstavit_v_bd(PGconn* conn, const Location& loc, const std::vector<LteCell>& cells);