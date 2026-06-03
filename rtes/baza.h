#pragma once
#include <string>
#include <vector>
#include <libpq-fe.h>
#include "struktury.h"

PGconn* podklyuchitBazu();
std::string sqlZnachenie(int v);
void vstavitDannye(PGconn* soedinenie, const Lokatsiya& lok, const std::vector<LteYacheika>& yacheyki);
