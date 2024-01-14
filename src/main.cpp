#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <secrets.h>

//wifi stuff
const char* ssid     = STASSID;
const char* password = STAPSK;

//display stuff
#define OLED_RESET    -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DISPLAY_ADDRESS 0x3c

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//twitch stuff
String Streamer = "die_a_n_n_a";
String tokenUrls = "https://id.twitch.tv/oauth2/token";
String streamsUrls = "https://api.twitch.tv/helix/streams?user_login=";
String clientId = CLIENTID;
String clientSecret = CLIENTSECRET;

//some variables
unsigned long expires = 0;
String token;
WiFiClientSecure client;
HTTPClient https;

void setup() {
  //setup serial
  Serial.begin(9600);

  //setup display
  display.begin(DISPLAY_ADDRESS, true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.display();

  //setup wifi
  Serial.print("Connect to Wifi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //http client setup
  client.setInsecure();
}

void loop() {
  display.setCursor(0,0);
  int nextround = 300000;

  if (WiFi.status() == WL_CONNECTED) {
    if (millis() >= expires) {
      Serial.println("Get new Token");
      https.begin(client, tokenUrls.c_str());
      https.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String httpRequestData = "client_id=" + clientId + "&client_secret=" + clientSecret + "&grant_type=client_credentials";

      int status = https.POST(httpRequestData.c_str());
      if (status == 200) {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, https.getStream());

        token = doc["access_token"].as<String>();
        unsigned long expiresIn = doc["expires_in"];

        expires = millis() + (expiresIn * 1000);
      } else {
        Serial.println(status);
      }
      https.end();
    }
    https.setReuse(false);
    Serial.println("Check Status");

    //get status
    String baererToken = "Bearer ";
    baererToken += token;

    String Url = streamsUrls + Streamer;

    https.begin(client, Url.c_str());
    https.addHeader("Authorization", baererToken.c_str());
    https.addHeader("Client-Id", clientId.c_str());
    int status = https.GET();
    
    display.clearDisplay();

    if (status == 200) {
      Serial.println("Found");
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, https.getStream());
      if (doc["data"][0].containsKey("type")) {
        display.printf(Streamer.c_str());
        Serial.printf(Streamer.c_str());
        display.println(" ist online!");
        Serial.println(" ist online!");
        display.print("Viewer: ");
        Serial.print("Viewer: ");
        display.println(doc["data"][0]["viewer_count"].as<String>());
        Serial.println(doc["data"][0]["viewer_count"].as<String>());
        display.println(doc["data"][0]["game_name"].as<String>());
        Serial.println(doc["data"][0]["game_name"].as<String>());
        display.println(doc["data"][0]["title"].as<String>());
        Serial.println(doc["data"][0]["title"].as<String>());
        nextround = 60000;
      } else {
        display.printf(Streamer.c_str());
        Serial.printf(Streamer.c_str());
        display.println(" ist offline!");
        Serial.println(" ist offline!");
        nextround = 300000;
      }
    } else {
      Serial.println("Get Stream lief schief:");
      Serial.println(status);
      nextround = 3000000;
    }
    
    display.display();
    https.end();
  }
  delay(nextround);
}
