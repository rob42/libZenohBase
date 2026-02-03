#include "zenohBase.h"

PicoSyslog::Logger syslog("base");
ZenohNode zenoh;

WifiNode wifiNode;
WebServerNode webServerNode;

JSONVar readings = new JSONVar();
unsigned long zenohLastTime = 0;
unsigned long zenohTimerDelay = 1000;
Preferences preferences;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 60*60*12;    // (NZ) Adjust this for your timezone, in secs
const int daylightOffset_sec = 3600; 
ESP32Time rtc(gmtOffset_sec);

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

void saveCredentials(String ssid, String password) {
  preferences.begin("nvs", false);
  preferences.clear();
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  
  Serial.print("SSID saved: ");
  Serial.println(ssid);
  Serial.print("SSID retrieved: ");
  Serial.println(preferences.getString("ssid"));

  preferences.end();
}

void configWifi(){
  WiFi.disconnect();
  wifiNode.config=false;
  WiFi.softAP("EspZenohBase");

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  //start an async web server, format if it fails to mount
  LittleFS.begin(true);
  AsyncWebServer server(80);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/config.html");
  });
  //handle a POST
  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request){
    String ssid = request->arg("ssid");
    String password = request->arg("password");
    saveCredentials(ssid,password);
    request->send(200, "text/plain", "Success! IP: " + WiFi.localIP().toString());
    wifiNode.config=true;
  });
  server.begin();
  Serial.println("Config server started...");
  //now wait for config to be done
  while(!wifiNode.config){
    //wait a bit
    sleep(1);
  }
  //done now
  server.end();
  Serial.println("Config server stopped");
}

void baseInit()
{
  Serial.begin(115200);
  syslog.server = RSYSLOG_IP;
  syslog.default_loglevel = PicoSyslog::LogLevel::debug;
  //check if wifi is configured
  preferences.begin("nvs", false);
  String ssid = preferences.getString("ssid", "");
  Serial.print("SSID: ");
  Serial.println(ssid);
  String passwd = preferences.getString("password", "");

  if(ssid.length()<1){
    //jump to web config
    configWifi();
  }
  
  wifiNode.setAccessPoint(ssid.c_str(),passwd.c_str());
  wifiNode.init();
  // wait for connection
  // try for 5 minutes fallback to host mode.
  int count = 0;
  while (!wifiNode.isConnected() || !wifiNode.ready)
  {
    delay(100);
    count++;
    if(count>300){  //30 sec
      count = 0;
      configWifi();
      //need to reboot now
      ESP.restart();
    }
  }
  delay(1000);
  syslog.print("Wifi connected : ");
  syslog.println(wifiNode.getIP());

  initOTA();

  // Configure NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  //now update rtc
  rtc.setTimeStruct(timeinfo);

  syslog.print("NTP time configured: ");
  syslog.println(rtc.getDateTime());

  webServerNode.init();

  initZenoh();
}


void baseLoopTasks()
{
  webServerNode.update();
  processZenoh();
  ArduinoOTA.handle();
}
