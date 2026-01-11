#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Hardware Pins ---

// SPI Pins for MAX31865 (PT100)
#define PIN_SPI_MOSI 15
#define PIN_SPI_MISO 18
#define PIN_SPI_SCK 19
#define PIN_SPI_CS 14 // Chip Select for MAX31865

// SSR Output Pin
#define PIN_SSR 20 // Solid State Relay control

// --- PID Configuration ---
#define PID_KP_DEFAULT 30.0
#define PID_KI_DEFAULT 0.1
#define PID_KD_DEFAULT 50.0

// --- Safety & Limits ---
#define TEMP_MAX_LIMIT 130.0 // Celsius (Safety cutoff)
#define TEMP_MIN_LIMIT 0.0   // Celsius (Sanity check)
#define TEMP_EMA_ALPHA 0.2   // Exponential Moving Average coefficient

// --- WiFi & MQTT Configuration ---
// Note: In a real production environment, use secrets.h or WiFiManager
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MQTT_BROKER "homeassistant.local"
#define MQTT_PORT 1883
#define MQTT_USER "mqtt_user"
#define MQTT_PASS "mqtt_password"

#define MQTT_TOPIC_PREFIX "rancilio_silvia"

#endif // CONFIG_H
