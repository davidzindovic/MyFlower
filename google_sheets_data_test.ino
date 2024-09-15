//Deployment code:
//AKfycbxZ6U2Afg7i92aL91parhMBiAG7RL4J4UDH6ZdVWtUbktdmV2u6uC8lGxvphJVRBMnQ

//probi narest tko da mas 7x array in ob bootu prebere gSheets pa jih zafila
// sicer pa SPIFFS : https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm

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

//WiFiClientSecure client;

bool spiffs_flag=1;

#define FORMAT_SPIFFS_IF_FAILED true

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

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

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("- file renamed");
  } else {
    Serial.println("- rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  Serial.printf("Testing file I/O with %s\r\n", path);

  static uint8_t buf[512];
  size_t len = 0;
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }

  size_t i;
  Serial.print("- writing");
  uint32_t start = millis();
  for (i = 0; i < 2048; i++) {
    if ((i & 0x001F) == 0x001F) {
      Serial.print(".");
    }
    file.write(buf, 512);
  }
  Serial.println("");
  uint32_t end = millis() - start;
  Serial.printf(" - %u bytes written in %lu ms\r\n", 2048 * 512, end);
  file.close();

  file = fs.open(path);
  start = millis();
  end = start;
  i = 0;
  if (file && !file.isDirectory()) {
    len = file.size();
    size_t flen = len;
    start = millis();
    Serial.print("- reading");
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      if ((i++ & 0x001F) == 0x001F) {
        Serial.print(".");
      }
      len -= toRead;
    }
    Serial.println("");
    end = millis() - start;
    Serial.printf("- %u bytes read in %lu ms\r\n", flen, end);
    file.close();
  } else {
    Serial.println("- failed to open file for reading");
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
  uint8_t col=0;
  uint8_t row=0;
  //while (file.available()) {
    char temp;
    for(int i=0;i<100;i++)
    {temp=file.read();
    Serial.print(temp);
    buf[i]=temp;
    }
    Serial.println(" ");
    //Serial.write(file.read());
    //buf=temp;
    //buf++;
  //}
  file.close();
}

char str_buf[200][1000];

void setup() {
  //tft.initR(INITR_BLACKTAB);
  //tft.setRotation(0);
  //tft.fillScreen(ST7735_BLACK);
  Serial.begin(115200);
  delay(10);

    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    spiffs_flag=0;
    return;
  }
  if(spiffs_flag)
  {
    writeFile(SPIFFS, "/slika1.txt","Slika 1\r\n");
  }


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
   static uint8_t slika_num=0;
   static uint8_t slika_vrstica=0;
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
