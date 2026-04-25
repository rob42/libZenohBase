#ifndef WEBSERVERNODE_H
#define WEBSERVERNODE_H
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <functional>
#include <WiFi.h>
#include <PicoSyslog.h>
#include <SimpleVector.h> 

extern PicoSyslog::Logger syslog;

class WebServerNode
{
public:
    
    // Create AsyncWebServer object on port 80
    AsyncWebServer server;

    // Create an Event Source on /events
    AsyncEventSource events;
    // Json Variable to hold data (Sensor Readings)
    JsonDocument webReadings = JsonDocument();
    JsonDocument jsonHosts = JsonDocument();

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
            int n = measureJson(webReadings);
            char json[n];
            serializeJson(webReadings, json ,n+1);
            request->send(200, "application/json", json); });
        
        server.on("/menu", HTTP_GET, [this](AsyncWebServerRequest *request)
            {    
            int n = measureJson(jsonHosts);
            char json[n];
            serializeJson(jsonHosts, json ,n+1);
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
            int n = measureJson(webReadings);
            char json[n];
            serializeJson(webReadings, json ,n+1);
            events.send(json, "new_readings", millis());
            syslog.debug.println(json);
            webLastTime = millis();
        }
    }

    void addHost(const char* host){
        jsonHosts.as<JsonArray>().add(host);
    }

    void setSensorData(const char *key, double value){
        int s = strlen(key);
        char str[s+1];
        strcpy(str,key);
        char *ref = &str[0];
        char *tok;
        char *prevTok;
       
        //syslog.debug.printf("Key: %s, size: %d \n",str, s+1);
        JsonObject tmp = webReadings.as<JsonObject>();
        JsonObject prev; 
        tok = strtok_r(ref, "/",&ref);
         while (tok != NULL){
            //first one
            //syslog.debug.println(tok);
            prevTok = tok;
           // if(!tmp.isNull()) {
                prev = tmp;
                if(tmp[tok].is<JsonObject>()){
                    tmp = tmp[tok];
                }else{
                    tmp = tmp[tok].to<JsonObject>();
                }
                
           // }
            tok = strtok_r(NULL, "/",&ref);
        }
        //tmp should now be the leaf
        prev[prevTok] = value;
        //syslog.debug.printf("done: %s = %f\n", prevTok, value);
    }


};
#endif
