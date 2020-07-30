


/*
This is a basic example that will print out the header and the content of an ArtDmx packet.
This example uses the read() function and the different getter functions to read the data.
This example may be copied under the terms of the MIT license, see the LICENSE file for details
This works with ESP8266 and ESP32 based boards

Winning finally
*/

#include <Artnet.h>
#include <WiFiUdp.h>
#include <SD.h>
#include <SPI.h>
#include <FastLED.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>


const int pixelMultiplier = 3; //How big is each cluster of leds to have on the same DMX settings/channel
const int numLeds = 36*24; // Change if your setup has more or less LED's
const int universes = 2;
const int numberOfChannels = ((numLeds * 3)/pixelMultiplier)/universes; // Total number of DMX channels you want to receive (1 led = 3 channels)
#define DATA_PIN 27 //The data pin that the WS2812 strips are connected to.
CRGB leds[numLeds];

//const char* ssid     = "P&Z";
//const char* password = "Barney123";
const char* ssid     = "Phil&JopsGlitch2E";
const char* password = "123456789";
IPAddress ip(172,16,0,2);
IPAddress gateway(172,16,0,2);
IPAddress subnet(255,255,0,0); 
//const char* html =
//#include "html.h";

const int startUniverse = 0;
uint8_t dataFrame[432] = {};
byte dataFrame2[433] = {};
uint8_t universe = 0;

Artnet artnet;

AsyncWebServer server(80);
const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";
bool record = false;
bool play = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>Jop's Glitch</h2>
  <h3>Camgirl sucked into CyberSpace!</h3>
  %BUTTONPLACEHOLDER%
  <h4>File Recording Length(s):<span id="ADCValue">0</span></h4><br>
  <div>%FILELIST%</div>
  
<script>
setInterval(function() {
  // Call a function repetatively with 2 Second interval
  getData();
}, 2000);

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("ADCValue").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "readADC", true);
  xhttp.send();
}
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Record The DMX</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"22\" " + outputState(22) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>PLAYBACK THE DMX</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"24\" " + outputState(24) + "><span class=\"slider\"></span></label>";
    //buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + outputState(33) + "><span class=\"slider\"></span></label>";
    //buttons += "<h4>Recording 1</h4><label><input type=\"radio\" onchange=\"changeFile(this)\" id=\"file\" " + outputFile(1) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  //Pincontrol
  pinMode(22, OUTPUT);
  digitalWrite(22, HIGH);
  delay(200);
  digitalWrite(22,LOW);
  pinMode(24, OUTPUT);
  digitalWrite(24, LOW);
  pinMode(33, OUTPUT);
  digitalWrite(33, LOW);
  //
  delay(5000);
  SPI.begin(14, 2, 15);
  if (! SD.begin(13, SPI, 6000000)) {
    Serial.println(F("Card Mount Failed"));
    return;
  }
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, numLeds);
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }

  
  uint32_t us = micros();
  //get the files ready to record

    // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  delay(200);
  WiFi.softAPConfig(ip, gateway, subnet);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  server.on("/readADC", HTTP_GET, [](AsyncWebServerRequest *request){
    int a = touchRead(T9);
    String adcValue = String(a);
    request->send(200, "text/plane", adcValue);
  });
  server.begin();
  Serial.println(IP);
  
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(250);
//    Serial.print(".");
//  }
//  Serial.println("");
//  Serial.print("Connected to ");
//  Serial.println(ssid);
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());

  // Route for root / web page

  //make a Data File
  File dataFile = SD.open("/hello.txt", FILE_WRITE);
    if (dataFile){
        Serial.println("hello.txt is ready for data");
      }
    dataFile.close();
    
  //Check current file list
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
      if ((inputMessage1.toInt() == 22) && (inputMessage2.toInt() ==  1)){
        record = true;
      }
      if ((inputMessage1.toInt() == 22) && (inputMessage2.toInt() ==  0)){
        record = false;
      }

      if ((inputMessage1.toInt() == 24) && (inputMessage2.toInt() ==  1)){
        play = true;
        Serial.println("Play is on");
      }
     if  ((inputMessage1.toInt() == 24) && (inputMessage2.toInt() ==  0)){
        play = false;
        Serial.println("Play is off");
      }
      
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
  artnet.begin();
}

void writeFile()
{
    uint32_t us = micros();
    Serial.println("Starting Write");
    File dataFile = SD.open("/hello.txt", FILE_APPEND);
    if (dataFile){
        dataFile.write(dataFrame,432);
        dataFile.write(universe);
      }
    dataFile.close();
    us = micros() - us;
    Serial.println(0.000001*us); 
}


void loop()
{
  if ((artnet.read() == ART_DMX) && (play == false))
  {
    // print out our data
    Serial.println(play);
    Serial.print("universe number = ");
    universe = (artnet.getUniverse());
    Serial.print(universe);
    Serial.print("\tdata length = ");
    Serial.print(artnet.getLength());
    Serial.print("\tsequence n0. = ");
    Serial.println(artnet.getSequence());
    Serial.print("DMX data: ");
    //Split get DataFrame from the DMX Frame
    for (int i = 0 ; i < 432 ; i++)
    {
      dataFrame[i]=(artnet.getDmxFrame()[i]);
      //Serial.print(dataFrame[i]);
      //Serial.print("  ");
    }

    //Split Universe 1 and 2 into correct CRGB Array
    for (int i = 0; i < 432/3; i++)
    {
      int led = i + (universe - startUniverse) * (432 / 3);
      
      if ((led < 144) & (universe == 0)){
        {
          for (int p = 0; p < pixelMultiplier; p++){
            leds[led*pixelMultiplier + p] = CRGB((dataFrame[i * 3]), (dataFrame[i * 3 + 1]), dataFrame[i * 3 + 2]);
          }
        }
      }
  
      
      if ((led >= 144) & (universe == 1)){
        {
          for (int p = 0; p < pixelMultiplier; p++){
            leds[led*pixelMultiplier + p] = CRGB((dataFrame[432 - (i*3)-3]), (dataFrame[432 - (i*3)-2]), (dataFrame[432 - (i*3)-1]));
          }
        }
      }
    }
  //Record that shit!
    FastLED.setBrightness(255);
    FastLED.show();
    
    Serial.println();
    Serial.println();
    if (record == true) writeFile();
    
  }
    
  //Get the dataFrame from hello.txt
    if (play == true){
      Serial.println("WE ARE PLAYING");
      //Get the Frame from hello.txt
      File dataFile = SD.open("/hello.txt");
      //File dataFile = SD.open("/hello.txt", FILE_READ);
      while(dataFile.available()){
        dataFile.read(dataFrame2,433);
            for (int i = 0; i<432; i++){
              Serial.print(i);
              Serial.print("U ");
              Serial.print(dataFrame2[432]);
              Serial.print(" ");
              Serial.print(dataFrame2[i]);
              Serial.print(" ");
            }
        universe = dataFrame2[432];
        Serial.print("universe playing now :");
        Serial.println(universe); 

        //Split Universe 1 and 2 into correct CRGB Array 864 is both Universes
        for (int i = 0; i < 432/3; i++)
        {
          int led = i + (universe - startUniverse)*(432 / 3);
          if (led == 143)Serial.print("LED");
          if ((led < 432/3) && (universe == 0))
            {
              for (int p = 0; p < pixelMultiplier; p++){
                leds[led*pixelMultiplier + p] = CRGB((dataFrame[i * 3]), (dataFrame[i * 3 + 1]), dataFrame[i * 3 + 2]);
              }
            }
          
          if ((led >= 432/3) && (universe == 1))
            {
              for (int p = 0; p < pixelMultiplier; p++){
                leds[led*pixelMultiplier + p] = CRGB((dataFrame[432 - (i*3)-3]), (dataFrame[432 - (i*3)-2]), (dataFrame[432 - (i*3)-1]));
              }
            }
        }
        FastLED.setBrightness(255);
        FastLED.show();
      }
//      if (dataFile.available())
//      {          
//          dataFile.read(dataFrame2,433);
//          //delay(50);
//                
//      }
//        else
//      {
//        dataFile.seek(0);
//      }

    //push out the frame to the LEDS
    //Frame timing
    delay(1800);
    dataFile.close();
    }  
}
