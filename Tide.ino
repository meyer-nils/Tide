/*

This sketch pulls tidal data from a german weather station and shows
it on AdaFriut NeoPixels. The data is obtained every 30 seconds and 
the update is indicated by an animation (that also shows the current
trend - rising or falling).

See www.pegelonline.wsv.de for more information on available tidal 
data. 

Circuit:
* Arduino MKR 1000
* 3 Adafruit NeoPixel Sticks connected to GND, 5V, and PIN 7

created 17 June 2020
by Nils Meyer
 */
#include <SPI.h>
#include <WiFi101.h>
#include <Adafruit_NeoPixel.h>
#include "arduino_secrets.h"

#define PIN        7 
#define NUMPIXELS 24

// Wifi connection settings
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Set up NeoPixel
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Set up Wifi client
WiFiClient client;
int status = WL_IDLE_STATUS;

// set server to pull water level data from
char server[] = "www.meyernils.de";

// send maximum and minimum water levels to map 
float max_value = 700.0;
float min_value = 300.0;


void setup() {
  // Inititialize serial connection for debugging
  // Serial.begin(9600);

  // Initialize NeoPixels
  pixels.begin(); 
  pixels.clear();
}


void loop() {
  // Repeat this forever...
  if (client.connect(server, 80) && status == WL_CONNECTED) {

    // Send request to Server 
    client.println("GET /pegel_knock.php HTTP/1.1");
    client.println("Host: www.meyernils.de");
    client.println();

    while (!client.available()) {}

    // Wait for Server response
    delay(1000);

    // Parse server response to variables
    float value = min_value;
    float trend = 0;
    while (client.available()) {
      String line = client.readStringUntil('\r');
      // If "timestep" is part of the string, its the JSON line
      if (line.indexOf("timestamp")>=0) {
        value = parse_JSON(line, "value");
        trend = parse_JSON(line, "trend");
      }
    }
    
    // Compute level
    float level = (value-min_value)/(max_value-min_value);
   
    // Set pixels
    animate_level(level, trend, 30, 100, 3000);
  }
  else {
    // Print red pixels
    animate_warning();

    // Try to (re-)connect
    status = WiFi.begin(ssid, pass);
    delay(10000);

    // Turn warning off, if connection was successfull
    if (status == WL_CONNECTED){
      pixels.clear();
    }
  }
}

float parse_JSON(String line, String var_name){
  /* Accepts a line and extracts the value after the
     search string "var_name", which is enclosed by 
     a ":" and ",".
     
     The expected JSON format is
     {
        "timestamp": "2020-06-17T21:07:00+02:00",
        "value": 633.0,
        "trend": 1,
        "stateMnwMhw": "unknown",
        "stateNswHsw": "unknown"
      } 
   */
   
  int s0 = line.indexOf(var_name);
  String trim0  = line.substring(s0);
  int s1 = trim0.indexOf(":") + 1;
  int e1 = trim0.indexOf(",");
  String trim1 = trim0.substring(s1, e1);
  return trim1.toFloat();
}

void animate_level(float level, float trend, int intensity, int repeat, int wait_time){
  for (int j; j<repeat; j++){
    // Treat extremly high tides with a red warning color
    int warn = 0;
    if(level > 1.0){warn = 255;}
  
    // Compute number of eluminated pixels and remaining intensity
    float n = level * NUMPIXELS;
    int N = floor(n);
    int intensity_fraction = floor((n-float(N))*intensity);
  
    // Set base level
    for(int i=0; i<N; i++) { 
      pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
    }
    // Set last pixel with fractional intensity
    pixels.setPixelColor(N, pixels.Color(0, 0, intensity_fraction));
    // Turn remaining pixels off
    for(int i=N+1; i<NUMPIXELS; i++) { 
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    pixels.show();
  
    // Animate trend
    if (trend > 0.0){
      for(int i=0; i<N; i++) { 
        pixels.setPixelColor(i, pixels.Color(warn, 0, 2*intensity));
        pixels.show();
        delay(50);
        pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
        pixels.show();
      }
    }
    else if (trend < 0.0){
      for(int i=N-1; i>=0; i--) { 
        pixels.setPixelColor(i, pixels.Color(warn, 0, 2*intensity));
        pixels.show();
        delay(50);
        pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
        pixels.show();
      }
    }
    delay(wait_time);
  }
}

void animate_warning(){
  // Reset pixels
  pixels.clear();

  // Animate rising red dots
  for(int i=0; i<NUMPIXELS; i++) { 
    pixels.setPixelColor(i, pixels.Color(40, 0, 0));
    pixels.show();
    delay(50);
  }  
}
