#ifndef SERVER_H
#define SERVER_H

#include <ESPAsyncWebServer.h>

void setupWebServer();
void stopWebServer();
AsyncWebServer& getWebServer();

#endif

