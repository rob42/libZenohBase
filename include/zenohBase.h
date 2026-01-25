#ifndef ZENOH_BASE_H
#define ZENOH_BASE_H

#include <Arduino.h>
#include <Arduino_JSON.h>
#include <ArduinoOTA.h>
#include <ZenohNode.h>
#include <WifiNode.h>
#include <WebServerNode.h>
#include <PicoSyslog.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <N2kTypes.h>
#include <N2kMessages.h>

// remote syslog server for logs
#define RSYSLOG_IP "192.168.1.125"

//zenoh
// Peer mode values (comment/uncomment as needed)
#define ZENOH_MODE "peer"
#define ZENOH_LOCATOR "udp/224.0.0.123:7447#iface=eth0" //in peer mode it MUST have #iface=eth0

// zenoh key that is published.
//#define KEYEXPR "null"

// Expose commonly used globals (implemented in zenohBase.cpp)
extern PicoSyslog::Logger syslog;
extern ZenohNode zenoh;
extern WifiNode wifiNode;
extern WebServerNode webServerNode;
extern JSONVar readings;

// zenoh publish timing (ms)
extern unsigned long zenohLastTime;
extern unsigned long zenohTimerDelay;

// Initialize base subsystems: WiFi, OTA, WebServer, Zenoh, Syslog
void baseInit();

// Run periodic base tasks (publishing, OTA handling, web updates)
void baseLoopTasks();

// NMEA2000 OnOpen callback (kept in base because it only references the scheduler)
void OnN2kOpen();

#endif
