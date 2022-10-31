#include "defines.h"
#include "html.h"
#include <ESP32Servo.h>


int status = WL_IDLE_STATUS;     // the Wifi radio's status
// int servo1Pin = 12;
// int servo2Pin = 13;
// int servo3Pin = 14;

Servo myservo1;
Servo myservo2;
Servo myservo3;

WiFiWebServer server(80);

int lastAngles[3];

void handleRoot()
{
  server.send(200, F("text/html"), html);
}

void handleNotFound()
{
  server.send(404, F("text/plain"), F("Not here."));
}

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  /// ********* START SETUP MOTORS *************

  Serial.print(F("Setting up motors..."));

  // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  
  myservo1.setPeriodHertz(50);    // standard 50 hz servo
  myservo2.setPeriodHertz(50);    // standard 50 hz servo
  myservo3.setPeriodHertz(50);    // standard 50 hz servo

	myservo1.attach(12, 400, 2400); 
	myservo2.attach(13, 400, 2400);
	myservo3.attach(4, 1000, 2000);

  lastAngles[0] = 0;
  lastAngles[1] = 0;
  lastAngles[2] = 0;

  Serial.println(F(" Done."));

  /// ********* END SETUP MOTORS *************

  Serial.print(F("\nStarting HelloServer on ")); Serial.print(BOARD_NAME);
  Serial.print(F(" with ")); Serial.println(SHIELD_TYPE); 
  Serial.println(WIFI_WEBSERVER_VERSION);

#if WIFI_USING_ESP_AT

  // initialize serial for ESP module
  EspSerial.begin(115200);
  // initialize ESP module
  WiFi.init(&EspSerial);

  Serial.println(F("WiFi shield init done"));
  
#endif

#if !(ESP32 || ESP8266)
  
  // check for the presence of the shield
  #if USE_WIFI_NINA
    if (WiFi.status() == WL_NO_MODULE)
  #else
    if (WiFi.status() == WL_NO_SHIELD)
  #endif
    {
      Serial.println(F("WiFi shield not present"));
      // don't continue
      while (true);
    }

  #if USE_WIFI_NINA
    String fv = WiFi.firmwareVersion();
    
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
      Serial.println(F("Please upgrade the firmware"));
    }
  #endif
  
#endif

  Serial.print(F("Connecting to SSID: "));
  Serial.println(ssid);
  
  status = WiFi.begin(ssid, pass);

  delay(1000);
   
  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED)
  {
    delay(500);
        
    // Connect to WPA/WPA2 network
    status = WiFi.status();
  }

  //server.begin();

  server.on(F("/"), handleRoot);

  server.on(F("/inline"), []()
  {
    server.send(200, F("text/plain"), F("This works as well"));
  });

  server.on(F("/echo"), [](){
    server.send(200, F("text/json"),"{" + server.argName(0) +":'" + server.arg(0) + "'}");
  });

  server.on(F("/move"), [&](){
    // -1 means don't move
    int motor1 = server.arg(0).toInt();
    int motor2 = server.arg(1).toInt();
    int motor3 = server.arg(2).toInt();

    for (float i=0.0; i<1.0; i+=0.02){
      if (motor1 != -1){
          myservo1.write(lastAngles[0] + (motor1 - lastAngles[0]) * i);
      }   

      if (motor2 != -1){
          myservo2.write(lastAngles[1] + (motor2 - lastAngles[1]) * i);
      }   

      if (motor3 != -1){
          myservo3.write(lastAngles[2] + (motor3 - lastAngles[2]) * i);
      }  
      delay(3);
    }

    lastAngles[0] = motor1;
    lastAngles[1] = motor2;
    lastAngles[2] = motor3;

    // DEBUG
    String debugJSON = "{motor1:" + String(motor1) + ", motor2: " + String(motor2) + ", motor3: " + String(motor3) + "}";
    Serial.println(debugJSON);

    server.send(200, F("text/json"),debugJSON);
  });

  server.onNotFound(handleNotFound);

  server.begin();

  Serial.print(F("HTTP server started @ "));
  Serial.println(WiFi.localIP());


}

void loop()
{
  server.handleClient();
}
