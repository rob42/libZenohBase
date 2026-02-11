#ifndef SIGKNALK_H
#define SIGKNALK_H
// zenoh key that is published.
#define KEY_ENVIRONMENT_DEPTH_BELOWSURFACE  "environment/depth/belowSurface"
#define KEY_ENVIRONMENT_DEPTH_BELOWTRANSDUCER  "environment/depth/belowTransducer"
#define KEY_ENVIRONMENT_OUTSIDE_PRESSURE  "environment/outside/pressure"
#define KEY_ENVIRONMENT_OUTSIDE_TEMPERATURE  "environment/outside/temperature"
#define KEY_ENVIRONMENT_WATER_TEMPERATURE  "environment/water/temperature"
#define KEY_ENVIRONMENT_WIND_ANGLEAPPARENT  "environment/wind/angleApparent"
#define KEY_ENVIRONMENT_WIND_ANGLETRUEGROUND  "environment/wind/angleTrueGround"
#define KEY_ENVIRONMENT_WIND_ANGLETRUEWATER  "environment/wind/angleTrueWater"
#define KEY_ENVIRONMENT_WIND_SPEEDAPPARENT  "environment/wind/speedApparent"
#define KEY_ENVIRONMENT_WIND_SPEEDTRUE  "environment/wind/speedTrue"
#define KEY_NAVIGATION_COURSEOVERGROUNDTRUE  "navigation/courseOverGroundTrue"
#define KEY_NAVIGATION_GNSS_DIFFERENTIALAGE  "navigation/gnss/differentialAge"
#define KEY_NAVIGATION_GNSS_DIFFERENTIALREFERENCE  "navigation/gnss/differentialReference"
#define KEY_NAVIGATION_GNSS_GEOIDALSEPARATION  "navigation/gnss/geoidalSeparation"
#define KEY_NAVIGATION_GNSS_HORIZONTALDILUTION  "navigation/gnss/horizontalDilution"
#define KEY_NAVIGATION_GNSS_POSITIONDILUTION  "navigation/gnss/positionDilution"
#define KEY_NAVIGATION_GNSS_SATELLITES  "navigation/gnss/satellites"
#define KEY_NAVIGATION_GNSS_TYPE  "navigation/gnss/type"
#define KEY_NAVIGATION_HEADINGTRUE  "navigation/headingTrue"
#define KEY_NAVIGATION_LOG  "navigation/log"
#define KEY_NAVIGATION_POSITION_ALTITUDE  "navigation/position/altitude"
#define KEY_NAVIGATION_POSITION_LATITUDE  "navigation/position/latitude"
#define KEY_NAVIGATION_POSITION_LONGITUDE  "navigation/position/longitude"
#define KEY_NAVIGATION_SPEEDOVERGROUND  "navigation/speedOverGround"
#define KEY_NAVIGATION_SPEEDTHROUGHWATER  "navigation/speedThroughWater"
#define KEY_NAVIGATION_TRIP_LOG  "navigation/trip/log"
#define KEY_STEERING_RUDDERANGLE  "steering/rudderAngle"       

//watermaker
#define KEY_WATERMAKER_MAIN_BOOSTPUMP_TEMPERATURE
#define KEY_WATERMAKER_MAIN_HIGHPRESSUREPUMP_TEMPERATURE
#define KEY_WATERMAKER_MAIN_PREFILTERPRESSURE
#define KEY_WATERMAKER_MAIN_POSTFILTERPRESSURE
#define KEY_WATERMAKER_MAIN_PREMEMBRANEPRESSURE
#define KEY_WATERMAKER_MAIN_POSTMEMBRANEPRESSURE
#define KEY_WATERMAKER_MAIN_PRODUCTSALINITY
#define KEY_WATERMAKER_MAIN_STARTTIME
#define KEY_WATERMAKER_MAIN_STOPTIME
#define KEY_WATERMAKER_MAIN_STATUS
#define KEY_WATERMAKER_MAIN_RUNTIME                                      
#endif