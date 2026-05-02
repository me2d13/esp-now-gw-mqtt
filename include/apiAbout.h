#ifndef API_ABOUT_H
#define API_ABOUT_H

#include <ArduinoJson.h>

/**
 * Populates a JsonArray with device-specific information including
 * network status, hardware details, and MQTT configuration.
 */
void getAboutData(JsonArray& array);

#endif // API_ABOUT_H
