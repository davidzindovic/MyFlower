//Deployment code:
//AKfycbxZ6U2Afg7i92aL91parhMBiAG7RL4J4UDH6ZdVWtUbktdmV2u6uC8lGxvphJVRBMnQ

// sicer pa SPIFFS : https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm

//to do: update po dnevih, kdaj potegne dol nov data (NTP)
//nujno test če zapiše stvari pravilno v img_buffer
//test če lahko zapisano prikaže


#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#include "FS.h"
#include "SPIFFS.h"

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

#define IME_SLIKE 10 // slika1\r\n vzame prvih 10 znakov

String imena_dir[8]=["/slika1.txt","/slika2.txt","/slika3.txt","/slika4.txt","/slika5.txt","/slika6.txt","/slika7.txt"];

int img_buffer[161][121];

//WiFiClientSecure client;

bool spiffs_flag=1;

#define FORMAT_SPIFFS_IF_FAILED true


void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, String message) {
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

void SPIFF2BUFF(fs::FS &fs, const char *path, char *buf)
{
  //Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");

  //while (file.available()) {
  
  for(uint8_t skip=0;skip<IME_SLIKE;skip++)
  {
    file.read(); //preskocis ime slike pol pa začneš pr podatkih
  }
  
    char temp;
    String beseda="";
    bool str_rdy=0;
    uint8_t char_count=0;
    uint8_t col=0;
    uint8_t row=0;

while(row<NUM_ROW || file.available())
{

    temp=file.read();
    
  if(col==NUM_COL && temp=="n")
  {
    col=0;
    row++;
  }  
  else{
    if(temp=="x")str_rdy=1;
    else if(str_rdy==1)
    {
      beseda.concat(temp);
      char_count++;
    }
    if(char_count==4)
    {
      buf[row][col]=string2header(beseda);
      col++:
      beseda="";
      str_rdy=0;
      char_count=0;
    }
    
    }
    Serial.println(" ");
  
}
  file.close();
}

char str_buf[200][1000];

void setup() {
  //tft.initR(INITR_BLACKTAB);
  //tft.setRotation(0);
  //tft.fillScreen(ST7735_BLACK);
  Serial.begin(115200);
  delay(10);

  //dodaj del da se ne formatira oz formatira samo prvic
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    spiffs_flag=0;
    return;
  }
  if(spiffs_flag)
  {
    writeFile(SPIFFS, "/slika1.txt","Slika 1\r\n");
  }
  //ntp datum
  //zapiši pravilno sliko v img_buffer

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  //dodaj da skico se povezuje
  //dodaj skico da louda data
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

void gsheets2spiff(void) {
  static uint8_t slika_num=0;
  static uint8_t slika_vrstica=0;
  HTTPClient http;
  String url="https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?read";

  Serial.print("Making a request  ");
  http.begin(url.c_str()); //Specify the URL and certificate
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  String payload;
    if (httpCode > 0) { //Check for the returning code
        payload = http.getString();
        
        

        //Serial.println(httpCode);
        Serial.println(payload);
        //if(spiffs_flag)
        //{
          appendFile(SPIFFS,"/slika1.txt",payload);
        
          appendFile(SPIFFS,"/slika1.txt","\r\n");
        //}
        /*
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
        */
        //tft.println(payload);
      }
    else {
      Serial.println("Error on HTTP request");
    }
  http.end();
  readFile(SPIFFS,"/slika1.txt");
}

int string2header(char *s) 
{
int x = 0;
uint8_t cnt=4; // char count
while(cnt!=0) 
  {
  cnt--;
  char c = *s;
  
  if (c >= '0' && c <= '9')x += (c - '0')*pow(16,cnt); 
  else if (c >= 'A' && c <= 'F')x += ((c - 'A') + 10)*pow(16,cnt); 
  else if (c >= 'a' && c <= 'f')x += ((c - 'a') + 10)*pow(16,cnt);
  else break;
  
  s++; 
  }
  return x;
}
