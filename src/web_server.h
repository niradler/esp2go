#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>

void setupWebServer();
void stopWebServer();
AsyncWebServer& getWebServer();

#endif

