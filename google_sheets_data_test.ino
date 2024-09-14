//Deployment code:
//AKfycbxZ6U2Afg7i92aL91parhMBiAG7RL4J4UDH6ZdVWtUbktdmV2u6uC8lGxvphJVRBMnQ

//probi narest tko da mas 7x array in ob bootu prebere gSheets pa jih zafila
// sicer pa SPIFFS : https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm

#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>


//Things to change
const char * ssid = "monika";
const char * password = "mladizmaji";
String GOOGLE_SCRIPT_ID = "AKfycbxthn0D81Pdln1UvdInSiHdrAl5lZZhwWv0v3nLfKCvYEkKdY-tlO8prBDzzCjaFC8";

const int sendInterval = 100; 
/********************************************************************************/
  #define TFT_CS         5  //case select connect to pin 5
  #define TFT_RST        25 //reset connect to pin 15
  #define TFT_DC         27 //AO connect to pin 32  (not sure that this is really used)  try pin 25
  #define TFT_MOSI       23 //Data = SDA connect to pin 23
  #define TFT_SCLK       18 //Clock = SCK connect to pin 18
  #define TFT_LED        26 
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);



//WiFiClientSecure client;



void setup() {
  //tft.initR(INITR_BLACKTAB);
  //tft.setRotation(0);
  //tft.fillScreen(ST7735_BLACK);
  Serial.begin(115200);
  delay(10);


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Started");
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
Serial.println("Ready to go");
//testdrawstyles();
}

void loop() {
  spreadsheet_comm();
  delay(sendInterval);
}

void spreadsheet_comm(void) {
   HTTPClient http;
   String url="https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?read";
//   Serial.print(url);
  Serial.print("Making a request  ");
  http.begin(url.c_str()); //Specify the URL and certificate
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  String payload;
    if (httpCode > 0) { //Check for the returning code
        payload = http.getString();
        
        //Serial.println(httpCode);
        Serial.println(payload);

        if(payload.length()>20)
        {
          for(int a=0;a<120;a++)
          {
            Serial.print(payload.substring(a*8+2,a*8+4+2));
            Serial.print("  ");
            char beseda[5];
            String payload_temp=payload.substring(a*8+2,a*8+4+2);
            payload_temp.toCharArray(beseda,5);
            //payload.substring(a*8+2,a*8+6+2);
            Serial.println(string2header(beseda));
            //int num = (int)strtol(hex, NULL, 16);
            //Serial.print("   ");
            //Serial.println(num); 
            //vzame samo kar je za 0x
          }
        }
        
        //tft.println(payload);
      }
    else {
      Serial.println("Error on HTTP request");
    }
  http.end();
}

int string2header(char *s) //TEST!!!
{
    int x = 0;
    uint8_t cnt=4;
  while(cnt!=0) {
    cnt--;
    //Serial.print(cnt);
    //Serial.print(" ");
    char c = *s;
    Serial.print(c);
    //Serial.print(" ");
    //Serial.println()
    if (c >= '0' && c <= '9') {
     // x *= pow(16,cnt);
      x += (c - '0')*pow(16,cnt); 
    }
    else if (c >= 'A' && c <= 'F') {
     // x *= 16;
      x += ((c - 'A') + 10)*pow(16,cnt); 
    }
    else if (c >= 'a' && c <= 'f') {
     // x *= 16;
      x += ((c - 'a') + 10)*pow(16,cnt);
    }
    else break;
    //Serial.println(x);
    s++;
    
  }
  return x;
}
