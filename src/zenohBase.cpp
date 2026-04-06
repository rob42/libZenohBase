#include "zenohBase.h"

PicoSyslog::Logger syslog("base");
ZenohNode zenoh;

WifiNode wifiNode;
WebServerNode webServerNode;

JsonDocument readings = JsonDocument();
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
 
 bool setMdns(const char* name){
  if (!MDNS.begin(name)) {
    Serial.println("Error setting up MDNS responder!");
    return false;
  }
  MDNS.addService("_http", "_tcp", 80);
  return true;

}

void mdns_print_results(mdns_result_t * results) {
	mdns_result_t * r = results;
	mdns_ip_addr_t * a = NULL;
	int i = 1, t;
	while (r) {
		if (r->instance_name) {
			syslog.debug.printf("  PTR : %s\n", r->instance_name);
		}
		if (r->hostname) {
			syslog.debug.printf("  SRV : %s.local:%u\n", r->hostname, r->port);
		}
		if (r->txt_count) {
			syslog.debug.printf("  TXT : [%u] ", r->txt_count);
			for (t = 0; t<r->txt_count; t++) {
				syslog.debug.printf("%s=%s; ", r->txt[t].key, r->txt[t].value);
			}
			syslog.debug.printf("\n");
		}
		a = r->addr;
		while (a) {
			if (a->addr.type==IPADDR_TYPE_V6) {
				syslog.debug.printf("  IPV6: " IPV6STR "\n", IPV62STR(a->addr.u_addr.ip6));
			}
			else {
				syslog.debug.printf("  IPV4   : " IPSTR "\n", IP2STR(&(a->addr.u_addr.ip4)));
			}
			a = a->next;
		}
		r = r->next;
	}
}

void getMDNShosts(){
//esp_err_t mdns_query_ptr(const char *service_type, const char *proto, uint32_t timeout, size_t max_results, mdns_result_t **results)

  mdns_result_t *results = NULL;
  mdns_query_ptr("_http", "_tcp", 5000, 20, &results);
  mdns_print_results(results);
  mdns_query_results_free(results);

}

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

void initZenoh(const char *hostname)
{
  zenoh.setHostname(hostname);
  if (!zenoh.begin(ZENOH_LOCATOR, ZENOH_MODE))
  {
    syslog.error.println("Zenoh setup failed!");
    return;
  }
  zenoh.declareHostnameQuery();
  
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

void baseInit(const char *hostname)
{
  Serial.begin(115200);
  syslog.server = RSYSLOG_IP;
  syslog.default_loglevel = PicoSyslog::LogLevel::critical;
  
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
  ArduinoOTA.setHostname(hostname);
  initZenoh(hostname);
  setMdns(hostname);

}

long baseLast = millis();
long memLast = millis();

void baseLoopTasks()
{
  //every 30 secs print memory free
  if( (millis() - memLast)>60000){
    u_int32_t mem = esp_get_free_heap_size();
    syslog.debug.printf( "Free Memory: %u\n", mem);
    memLast = millis();
  }
  webServerNode.update();
  ArduinoOTA.handle();
}
