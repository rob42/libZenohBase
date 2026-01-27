#ifndef WEBSERVERNODE_H
#define WEBSERVERNODE_H
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>
#include <functional>
#include <WiFi.h>
#include <PicoSyslog.h>

extern PicoSyslog::Logger syslog;

class WebServerNode
{
public:
    
    // Create AsyncWebServer object on port 80
    AsyncWebServer server;

    // Create an Event Source on /events
    AsyncEventSource events;
    // Json Variable to hold data (Sensor Readings)
    JSONVar webReadings = JSONVar();

    unsigned long webLastTime = 0;
    unsigned long webTimerDelay = 1000;
    WebServerNode() : server(80), events("/events") {}
    void init()
    {
        // Initialize LittleFS
        if (!LittleFS.begin())
        {
            syslog.error.println("An error has occurred while mounting LittleFS");
            return;
        }
        syslog.information.println("LittleFS mounted successfully");


        syslog.information.print("Starting webserver...");
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(LittleFS, "/index.html", "text/html"); });
        server.serveStatic("/", LittleFS, "/");
        server.on("/readings", HTTP_GET, [this](AsyncWebServerRequest *request)
                  {
            String json = JSON.stringify(webReadings);
            request->send(200, "application/json", json); });
        events.onConnect([](AsyncEventSourceClient *client)
                {
            if(client->lastId()) {
                syslog.debug.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
            }
            client->send("hello!", NULL, millis(), 10000); });
        server.addHandler(&events);
        server.begin();
        syslog.information.println("OK");
    }
    void update()
    {
        if ((millis() - webLastTime) > webTimerDelay)
        {
            events.send("ping", NULL, millis());
            events.send(JSON.stringify(webReadings).c_str(), "new_readings", millis());
            webLastTime = millis();
        }
    }

    void setSensorData(const char* key, double value){
        webReadings[key] = value; // String(bme.readTemperature());
    }

};
#endif
