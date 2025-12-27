#ifndef API_SERVER_H
#define API_SERVER_H

#include <ESPAsyncWebServer.h>

void setupWebServer();
void stopWebServer();
AsyncWebServer& getWebServer();

#endif

