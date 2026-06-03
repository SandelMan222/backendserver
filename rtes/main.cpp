#include <thread>
#include <curl/curl.h>
#include "struktury.h"
#include "obrabotka.h"
#include "server.h"
#include "grafika.h"

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    static DannyeUstrojstva dannyeUstrojstva;

    zagruzitFajl(&dannyeUstrojstva);

    std::thread potok_servera(zapustitServer, &dannyeUstrojstva);
    std::thread potok_grafiki(zapustitGrafiku, &dannyeUstrojstva);

    potok_grafiki.join();
    potok_servera.join();

    curl_global_cleanup();
    return 0;
}
