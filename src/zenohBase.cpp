#include "zenohBase.h"

PicoSyslog::Logger syslog("base");
ZenohNode zenoh;

WifiNode wifiNode;
WebServerNode webServerNode;

JSONVar readings;
unsigned long zenohLastTime = 0;
unsigned long zenohTimerDelay = 1000;
Preferences preferences;

#ifdef LIB_COMPILE_ONLY
//we need this (in arduino) to be able to compile the lib standalone during lib dev.
//not included when building a project using the lib.
 void setup()
 {
    //dummy
 }

// // *****************************************************************************
 void loop()
 {
    //dummy
 }
 #endif
 
// Simple message callback matching ZenohMessageCallback
void onZenohMessage(const char *topic, const char *payload, size_t len)
{
  syslog.debug.print("Received on [");
  syslog.debug.print(topic);
  syslog.debug.print("]: ");

  // Print payload as text (safe only for text payloads)
  for (size_t i = 0; i < len; ++i)
  {
    syslog.debug.print(payload[i]);
  }
  syslog.debug.println();
}

void initZenoh()
{
  if (!zenoh.begin(ZENOH_LOCATOR, ZENOH_MODE, KEYEXPR))
  {
    syslog.error.println("Zenoh setup failed!");
    return;
  }
  // Subscribe to a topic
  if (zenoh.subscribe("navigation/courseOverGround", onZenohMessage) && zenoh.subscribe("navigation/speedOverGround", onZenohMessage))
  {
    syslog.information.println("Subscribed to navigation/courseOverGround");
    syslog.information.println("Subscribed to navigation/speedOverGround");
  }
  else
  {
    syslog.error.println("Subscribe failed");
  }
}

void processZenoh()
{
  if ((millis() - zenohLastTime) > zenohTimerDelay)
  {
    if (!zenoh.publish(KEYEXPR, JSON.stringify(readings).c_str()))
    {
      syslog.error.println("Publish failed (node not running?)");
      if (!zenoh.isRunning())
      {
        initZenoh();
      }
    }
    zenohLastTime = millis();
  }
}

void initOTA()
{
  ArduinoOTA.setHostname("base");

  ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      syslog.information.println("Start updating " + type);
    })
    .onEnd([]() {
      syslog.information.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      syslog.information.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      syslog.error.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) syslog.error.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) syslog.error.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) syslog.error.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) syslog.error.println("Receive Failed");
      else if (error == OTA_END_ERROR) syslog.error.println("End Failed");
    });

  ArduinoOTA.begin();
}

void saveCredentials(const char* ssid, const char* password) {
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}

void configWifi(){
  WiFi.softAP("EspZenohBase");

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  //start an async web server
  LittleFS.begin();
  AsyncWebServer server(80);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/config.html");
  });
  //handle a POST
  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request){
    String ssid = request->arg("ssid");
    String password = request->arg("password");
    saveCredentials(ssid.c_str(),password.c_str());
    request->send(200, "text/plain", "Success! IP: " + WiFi.localIP().toString());
  });
  server.begin();
  //now wait for config to be done
  while(!wifiNode.config){
    //wait a bit
    sleep(1);
  }
  //done now
  server.end();
}

void baseInit()
{
  Serial.begin(115200);
  syslog.server = RSYSLOG_IP;
  syslog.default_loglevel = PicoSyslog::LogLevel::debug;
  //check if wifi is configured
  String ssid = preferences.getString("ssid", "");
  String passwd = preferences.getString("passwd", "");

  if(ssid.length()<1){
    //jump to web config
    configWifi();
  }
  
  wifiNode.setAccessPoint(ssid.c_str(),passwd.c_str());
  wifiNode.init();
  // wait for connection
  while (!wifiNode.isConnected() || !wifiNode.ready)
  {
    delay(10);
  }
  delay(1000);
  syslog.print("Wifi connected : ");
  syslog.println(wifiNode.getIP());

  initOTA();
  webServerNode.init();

  initZenoh();
}


void baseLoopTasks()
{
  webServerNode.update();
  processZenoh();
  ArduinoOTA.handle();
}
