#include "curl_zapros.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>

size_t onOtvet(void* data, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto& blob = *static_cast<std::vector<unsigned char>*>(userp);
    auto const* dataptr = static_cast<unsigned char*>(data);
    blob.insert(blob.cend(), dataptr, dataptr + realsize);
    return realsize;
}

bool skachatTajl(int z, int x, int y, std::vector<unsigned char>& blob) {
    std::ostringstream url;
    url << "https://a.tile.openstreetmap.org/" << z << "/" << x << "/" << y << ".png";

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&blob);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onOtvet);

    bool ok = (curl_easy_perform(curl) == CURLE_OK);
    curl_easy_cleanup(curl);

    if (!ok) std::cout << "Oshibka zagruzki tajla " << z << "/" << x << "/" << y << std::endl;
    return ok;
}
