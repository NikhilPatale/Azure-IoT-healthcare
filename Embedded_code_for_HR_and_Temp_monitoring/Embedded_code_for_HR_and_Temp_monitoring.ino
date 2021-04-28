
#include <Adafruit_GFX.h>        
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "MAX30105.h"           
#include "heartRate.h"          
#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

const char* ssid = "Patale"; 
const char* password = "Patale#1#1@2@2"; 

MAX30105 particleSensor;

const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0; 
float beatsPerMinute;
int beatAvg;
int c;
char reqBody[100];

const String AzureIoTHubURI="https://IoTTechnocrats.azure-devices.net/devices/HR/messages/events?api-version=2020-03-13"; 
//openssl s_client -servername myioteventhub.azure-devices.net -connect myioteventhub.azure-devices.net:443 | openssl x509 -fingerprint -noout //
const String AzureIoTHubFingerPrint="E0:9F:C0:BE:80:0A:A2:36:AB:73:8C:FB:CA:B2:3E:0B:E1:5F:5A:B9"; 
//az iot hub generate-sas-token --device-id {YourIoTDeviceId} --hub-name {YourIoTHubName} 
const String AzureIoTHubAuth="SharedAccessSignature sr=IoTTechnocrats.azure-devices.net%2Fdevices%2FHR&sig=B7ZWZAsi551ElziWukz5lRrn2lP35J63lVVQXEy4ezQ%3D&se=1619507076";
 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 

void setup() {  
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  display.display();
  delay(3000);
  // Initialize sensor
  particleSensor.begin(Wire, I2C_SPEED_FAST); 
  particleSensor.setup(); 
  particleSensor.setPulseAmplitudeRed(0x0A); 
  particleSensor.enableDIETEMPRDY();
 
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) 
  {
     display.clearDisplay(); 
     display.setTextSize(1);
     display.setTextColor(WHITE);
     display.setCursor(0,5);
     display.print("Connecting to Wifi...");
     display.display();
     Serial.print("*");
     delay(200);
  }
  
  Serial.println("");
  Serial.println("WiFi connection Successful");
  display.clearDisplay(); 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(30,5);
  display.print("Connected");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(30,15);
  display.print("Successfully :)");
  display.display();
  Serial.print("The IP Address of ESP8266 Module is: ");
  Serial.print(WiFi.localIP());
  delay(5000);
  Serial.print("\n");


}

void loop() {
 long irValue = particleSensor.getIR();   
                                         
if(irValue > 7000)
{ 
  float temperature = particleSensor.readTemperature();
   if(c<5)
 {
   Serial.println("Computing.... Please wait");
   c++;
   delay(1000);                                         
 } 
  if (checkForBeat(irValue) == true)                        
  {   
    long delta = millis() - lastBeat;                   
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0)+60;          
    delay(50);
    if (beatsPerMinute < 255 && beatsPerMinute > 20)               
    {
      rates[rateSpot++] = (byte)beatsPerMinute; 
      rateSpot %= RATE_SIZE; 

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE; 
    }
    display.clearDisplay(); 
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(10,5);
    display.print("HR");
    display.setTextSize(1);
    display.setCursor(45,5);
    display.println(beatAvg);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(62,5);
    display.print("BPM");
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(10,15);
    display.print("Temp ");
    display.setTextSize(1);
    display.setCursor(45,15 );
    display.println(temperature);
    display.setCursor(78,15);
    display.setTextSize(1);
    display.cp437(true);
    display.write(167);
    display.setTextSize(1);
    display.print("C");
    display.display();
    Serial.print("Status :     BPM= ");
    Serial.print(beatAvg, 1); 
    Serial.print("\t");
    Serial.print("temperatureC= ");
    Serial.print(temperature, 1);
    Serial.println();

     String PostData="{ \"DeviceId\":\"HR\",\"HR\":"+String(beatAvg)+",\"Temp\":"+String(temperature)+"}";
     Serial.println(PostData);
     RestPostData(AzureIoTHubURI,AzureIoTHubFingerPrint,AzureIoTHubAuth,PostData); 
  }

}
  if (irValue < 7000){       
     beatAvg=0;
     c=0;
     display.clearDisplay();
     display.setTextSize(1);                    
     display.setTextColor(WHITE);             
     display.setCursor(0,5);                
     display.println("Place your finger ");
     Serial.print("Place your finger ");
     display.setCursor(0,15);
     display.println("and kindly wait ");
     Serial.print("and kindly wait "); 
     Serial.print("\n"); 
     display.display();
     }

}

int RestPostData(String URI, String fingerPrint, String Authorization, String PostData)
{
    HTTPClient http;
    http.begin(URI,fingerPrint);
    http.addHeader("Authorization",Authorization);
    http.addHeader("Content-Type", "application/atom+xml;type=entry;charset=utf-8");
    int returnCode=http.POST(PostData);
    if(returnCode<0) 
  {
    Serial.println("RestPostData: Error sending data: "+String(http.errorToString(returnCode).c_str()));
  }
    http.end();
  return returnCode;
}
