#pragma once

#include "heatmap_model.h"
#include <vector>
#include <string>

double vychislit_rasstoyanie(double lat1, double lon1, double lat2, double lon2);

double poluchit_znachenie_idw(
    double shirota, double dolgota,
    const std::vector<TochkaIzmereniya>& dannye,
    int radiusMetry
);

TsvetnayaMapa konvertirovat_v_tsvet(double znachenie, KriteriyTeplovoyKarty kriteriy);

void generateheatmap(const ParametryGeneracii& parametry);

void zapustit_generaciyu_v_potoke(const ParametryGeneracii& parametry);

void ostanavliv_generaciyu();
