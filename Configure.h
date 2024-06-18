/*
A file for library inclusions and configs
*/

// Library includes

#include <SPI.h>
#include "Ucglib.h"
#include "icons.h"
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// Configs

#define ENABLE_WDT false
#define FORCE_FAIL_WIFI false
#define FORCE_PANIC false
#define ANTIALIASED_TEXT false // Experimental option 
// ^ not implemented (too lazy)
#define PRINT_INTERNAL_TIMER true
