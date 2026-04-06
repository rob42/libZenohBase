#include "ZenohNode.h"

z_owned_session_t s;

//z_owned_publisher_t pub;
//z_owned_subscriber_t sub;
Hashtable <String, z_owned_subscriber_t> subscribers;
//ZenohMessageCallback callback;
Hashtable <String, ZenohMessageCallback> subscriberCallback;
//need hashtable to hold key<>publisher map.
Hashtable <String, z_owned_publisher_t> publishers;

const char* hostname;

ZenohNode::ZenohNode()
  : running(false) //, callback(nullptr)
{
}

ZenohNode::~ZenohNode()
{
  end();
}

bool ZenohNode::begin(const char* locator, const char* mode)
{
  // Initialize Zenoh Session and other parameters
  syslog.information.print("Initialize Zenoh Session and other parameters...");
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_LISTEN_KEY, locator);  
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, mode);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MULTICAST_SCOUTING_KEY, "true");

    // if (strcmp(locator, "") != 0) {
    //     if (strcmp(mode, "client") == 0) {
    //         zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, locator);
    //     } else {
    //         zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_LISTEN_KEY, locator);
    //     }
    // }
    syslog.information.println("OK");
    
    
    // Open Zenoh session
    syslog.information.print("Opening Zenoh Session...");
    if (z_open(&s, z_config_move(&config), NULL) < 0) {
        syslog.error.println("Unable to open session!");
        return false;
    }
    syslog.information.println("OK");
    
    syslog.information.print("Start read and lease tasks...");
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&s), NULL) < 0) {
        syslog.error.println("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        return false;
    }
    syslog.println("OK");
   
    syslog.information.println("Zenoh setup finished!");

    delay(300);

  
  running = true;
  return true;
}

// Callback function to handle each peer ID  
volatile unsigned int zids = 0;  
void zid_handler(const z_id_t *id, void *arg) {  
    (void)arg;  // Unused context  
      
    // Convert the ID to a string for printing  
    z_owned_string_t id_str;  
    z_id_to_string(id, &id_str);  
      
    // Print the peer ID  
    syslog.debug.printf("Peer ID: %.*s\n", (int)z_string_len(z_string_loan(&id_str)), z_string_data(z_string_loan(&id_str)));  
      
    // Clean up the string  
    z_string_drop(z_string_move(&id_str));  
      
    zids++;  
}  

void hostnameQueryHandler(_z_query_rc_t *query, void *arg) {
    syslog.debug.println("hostname Query Handler");
    (void)query;
    (void)(arg);
    z_owned_bytes_t payload;  
    z_bytes_copy_from_str(&payload, hostname);  
    z_result_t r =  z_query_reply(query, z_query_keyexpr(query), z_bytes_move(&payload), NULL); 
    //payload dropped in z_query_reply
    syslog.debug.printf("  hostnameQueryHandler replied: %s\n", r ? "failed" : "OK");
}

void hostnameReplyHandler(z_loaned_reply_t *reply, void *arg ){
    syslog.debug.println("hostname Reply Handler");
    if (z_reply_is_ok(reply)) {  
        const z_loaned_sample_t *sample = z_reply_ok(reply);  
        
        z_owned_string_t value;  
        z_bytes_to_string(z_sample_payload(sample), &value);  
        
        const z_loaned_string_t *loaned = z_string_loan(&value);
        syslog.debug.printf("  Hostname : %.*s\n", (int)z_string_len(loaned), z_string_data(loaned));
        
        // Clean up the string  
        z_string_drop(z_string_move(&value)); 
        
    } else {  
        // Handle error reply  
        syslog.error.println(" hostname Reply Handler failed");
    }  
}

bool ZenohNode::getPeerHostnames(){
  syslog.debug.println("getPeerHostnames");

    z_get_options_t options;  
    z_get_options_default(&options);  
  //   options.target = Z_QUERY_TARGET_ALL;              // Target all matching queryables  
  //   options.consolidation.mode = Z_CONSOLIDATION_MODE_AUTO; // Reply consolidation  
     options.timeout_ms = 1000;                        // Timeout in milliseconds

  z_view_keyexpr_t ke1;
  z_view_keyexpr_from_str(&ke1, INFO_HOSTNAME);

  z_owned_closure_reply_t callback;

  z_result_t r = z_closure_reply(&callback, hostnameReplyHandler, NULL, NULL);
  //syslog.debug.printf("  create getPeerHostnames closure : %s\n", r ? "failed" : "OK");

  z_result_t res = z_get(z_session_loan(&s), z_view_keyexpr_loan(&ke1), "", z_closure_reply_move(&callback), &options);  
      
  syslog.debug.printf("Hostname query:%s\n",res ? "failed" : "OK");
  z_closure_reply_drop(z_closure_reply_move(&callback));
  return true;
}

bool ZenohNode::getZenohPeers(JsonArray peers){
  syslog.debug.println("getZenohPeers");
  zids=0;
      z_owned_closure_zid_t closure;  
    z_closure_zid(&closure, zid_handler, NULL, NULL);  
      
    // Fetch and print all peer IDs  
    syslog.debug.printf("Fetching peer IDs...\n");  
    z_result_t result = z_info_peers_zid(z_session_loan(&s), z_closure_zid_move(&closure));  
      
    if (result == 0) {  
        syslog.debug.printf("Found %u peer(s)\n", zids);  
    } else {  
        syslog.debug.printf("Error fetching peer IDs: %d\n", result);  
    }  
    z_closure_zid_drop(z_closure_zid_move(&closure));
    return true;
}

void ZenohNode::setHostname(const char* name){
  hostname = name;
}

bool ZenohNode::declareHostnameQuery(){
  z_view_keyexpr_t ke1;
  z_view_keyexpr_from_str(&ke1, INFO_HOSTNAME);

  //z_owned_queryable_t q1;
  z_owned_closure_query_t cb1;
  z_result_t rc = z_closure_query(&cb1, hostnameQueryHandler,NULL, NULL);
  syslog.debug.printf("Declared hostnameQueryHandler:%s\n",rc ? "failed" : "OK");

  // z_owned_fifo_handler_query_t h1;
  // z_result_t rf = z_fifo_channel_query_new(&cb1, &h1, 16);
  // syslog.debug.printf("Declared fifo:%s\n",rc ? "failed" : "OK");

  z_result_t r =  z_declare_background_queryable(z_session_loan(&s), z_view_keyexpr_loan(&ke1), z_closure_query_move(&cb1), NULL);
  syslog.debug.printf("Declared hostname query:%s\n",r ? "failed" : "OK");
    

  return true;
}

bool ZenohNode::declarePublisher(const char* keyExpr){
  // Declare Zenoh publisher
    syslog.information.print("Declaring publisher for ");
    syslog.information.print(keyExpr);
    syslog.information.println("...");
    if(publishers.containsKey(keyExpr)){
      //we already have it
      syslog.information.println("Already declared");
      return false;
    }
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, keyExpr);
    if (z_declare_publisher(z_session_loan(&s), &pub, z_view_keyexpr_loan(&ke), NULL) < 0) {
        syslog.error.println("Unable to declare publisher for key expression!");
        return false;
    }
    publishers.put(keyExpr, pub);
    syslog.information.println("OK");
    return true;
}


void ZenohNode::end()
{
  if (!running) return;
  // Placeholder cleanup logic.
  syslog.information.println("ZenohNode: shutting down");
  for(z_owned_publisher_t p : publishers.values()){
    z_undeclare_publisher(z_publisher_move(&p));
  }
  for(z_owned_subscriber_t s : subscribers.values()){
    z_undeclare_subscriber(z_subscriber_move(&s));
  }
  publishers.clear();
  subscribers.clear();
  subscriberCallback.clear();

  running = false;
}

bool ZenohNode::checkSession(){
   if (z_session_is_closed(z_session_loan(&s))) {
    syslog.error.println("Error: Zenoh is not running");
    return false;
  }
  return true;
}


bool ZenohNode::publishZbytes(const char* topic, const char* payloadStr){
  z_owned_bytes_t payload;
  z_bytes_copy_from_str(&payload, payloadStr);
  if (z_publisher_put(z_publisher_loan(publishers.get(topic)), z_bytes_move(&payload), NULL) < 0) {
      syslog.error.printf("Error while publishing data: %s\n", topic);
      return false;
  }
  // Assume publish succeeds.
   z_bytes_drop(z_bytes_move(&payload));
  return true;
}

bool ZenohNode::publish(const char* topic, const char* pLoad)
{
  if(!checkSession())return false;
 
  syslog.debug.printf("ZenohNode: publish to %s : %s\n", topic,pLoad);
  return publishZbytes(topic,pLoad);
}

// Convenience overload for null-terminated payloads (double).
bool ZenohNode::publish(const char* topic, double pLoad)
{
  if(!checkSession())return false;
 
  syslog.debug.printf("ZenohNode: publish to %s : %f \n", topic,pLoad);

  return publishZbytes(topic,String(pLoad).c_str());
}

// Convenience overload for null-terminated payloads (float).
bool ZenohNode::publish(const char* topic, float pLoad){
  if(!checkSession())return false;
 
  syslog.debug.printf("ZenohNode: publish to %s : %f \n", topic,pLoad);
  return publishZbytes(topic,String(pLoad).c_str());
}

// Convenience overload for null-terminated payloads (int).
bool ZenohNode::publish(const char* topic, int pLoad){
  if(!checkSession())return false;
 
  syslog.debug.printf("ZenohNode: publish to %s : %d \n", topic,pLoad);

  return publishZbytes(topic,String(pLoad).c_str());
}

// Convenience overload for null-terminated payloads (long).
bool ZenohNode::publish(const char* topic, long pLoad){
  if(!checkSession())return false;
 
  syslog.debug.printf("ZenohNode: publish to %s : %d \n", topic,pLoad);
  return publishZbytes(topic,String(pLoad).c_str());
}

// Convenience overload for null-terminated payloads (long).
bool ZenohNode::publish(const char* topic, bool pLoad){
  if(!checkSession())return false;
 
  syslog.debug.printf("ZenohNode: publish to %s : %d \n", topic,pLoad);
  return publishZbytes(topic,String(pLoad).c_str());
}

/*
  Always calls the callback with the char* of the payload. 
  
  The published value will be valid as the signalk type:
    values (primitives) will be simple text
    complex types will be json

  eg If the payload was 'navigation.position.latitude, holding a float
  then the callback should know the correct type and deal with that. 

  If the payload was 'navigation/position' then it will be json

*/
void ZenohNode::data_handler(z_loaned_sample_t *sample, void *arg) {
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);

    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);

    syslog.debug.printf(" >> [Subscription listener] Received ( %s, %s )\n", &keystr, &value);
  
    const char* key = z_string_data(z_view_string_loan(&keystr));
    
    ZenohMessageCallback* callback =  subscriberCallback.get(key);
    if(callback == nullptr){
      syslog.debug.printf(" >> [Subscription listener] no callback for %s\n",key);
      
    }else{
      (*callback)(key,  
          z_string_data(z_string_loan(&value)),
          z_string_len(z_string_loan(&value)));
    }
    z_string_drop(z_string_move(&value));
  }

bool ZenohNode::subscribe(const char* topic, ZenohMessageCallback cb)
{

  if (!running) return false;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, topic);
  if(subscribers.containsKey(topic)){
      //we already have it
      syslog.information.println("Already declared");
      return false;
    }
  
    // Declare Zenoh subscriber
    syslog.information.printf("Declaring Subscriber on %s\n",topic);

    z_owned_subscriber_t sub;

    z_owned_closure_sample_t sample;
    
    z_closure_sample(&sample, data_handler, NULL, NULL);
    
    if (z_declare_subscriber(z_session_loan(&s), &sub, z_view_keyexpr_loan(&ke), z_closure_sample_move(&sample),
                             NULL) < 0) {
        syslog.error.println("Unable to declare subscriber.");
        return false;
    }
    //store subscriber
    subscribers.put(topic, sub);

    // Store callback 
    subscriberCallback.put(topic, cb);
    
    syslog.information.println("OK");
    
  return true;
}

bool ZenohNode::isRunning() const
{
  
  return running;
}
