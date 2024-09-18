//Deployment code:
//AKfycbxZ6U2Afg7i92aL91parhMBiAG7RL4J4UDH6ZdVWtUbktdmV2u6uC8lGxvphJVRBMnQ

// sicer pa SPIFFS : https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm

/*to do
 - uredi gScript (imena, zaporedje...)
 - potegne vse slike v spiffs ko se zamenja teden (dan 1 in nov datum)
 - naloži slike iz spiffs na ekran (test da vse naloži ciklično v buffer in pokaže)
 - tekst buffer
 - tipka
 */

  #define DEBUG 1

  #include <WiFi.h>
  #include "time.h"
  #include <HTTPClient.h>
  #include <Adafruit_GFX.h>    // Core graphics library
  #include <Adafruit_ST7735.h> // Hardware-specific library
  #include <SPI.h>

  #include "FS.h"
  #include "SPIFFS.h"

  #define TIPKA 2

  /*Things to change */
  
  const char * ssid_hotspot = "monika";
  const char * password_hotspot = "mladizmaji";

  const char * ssid_home = "test";
  const char * password_home = "test";

  String GOOGLE_SCRIPT_ID = "AKfycbxthn0D81Pdln1UvdInSiHdrAl5lZZhwWv0v3nLfKCvYEkKdY-tlO8prBDzzCjaFC8";

  const int sendInterval = 100;
  /* KONEC USER CHANGES */

  //--------------------------------------------------------------------

  /*  ZASLON STUFF  */
    #define TFT_CS         5  //case select connect to pin 5
    #define TFT_RST        25 //reset connect to pin 15
    #define TFT_DC         27 //AO connect to pin 32  (not sure that this is really used)  try pin 25
    #define TFT_MOSI       23 //Data = SDA connect to pin 23
    #define TFT_SCLK       18 //Clock = SCK connect to pin 18
    #define TFT_LED        26 
  //Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
  /*  KONEC ZASLON STUFF  */

  //------------------------------------------------------------

  /*    SLIKE STUFF     */
  #define IME_SLIKE 10 // slika1\r\n vzame prvih 10 znakov
  #define NUM_DAYS 7

  String imena_dir[8]={"/slika1.txt","/slika2.txt","/slika3.txt","/slika4.txt","/slika5.txt","/slika6.txt","/slika7.txt"," "};
  String spif_log="/log.txt";

  //locljivost slike:
  #define NUM_ROW 160
  #define NUM_COL 120

  int img_buffer[NUM_ROW+1][NUM_COL+1];

  #define MAX_TEXT_SPLITS 4
  String text_buffer[MAX_TEXT_SPLITS];
  /* KONEC SLIKE STUFF  */

  //------------------------------------------------------------
  //WiFiClientSecure client;

  /*       DATUM STUFF          */
  const char* ntpServer = "pool.ntp.org";
  const long  gmtOffset_sec = 0;
  const int   daylightOffset_sec = 3600;
  const uint8_t day_info=0;
  uint8_t week_day=0;
  uint8_t day_of_month=0;
  uint8_t month=0;
  uint8_t year=0;
  /* KONEC DATUM STUFFA         */

  //-------------------------------------------------------

  /*      SPIFFS      */
  bool spiffs_flag=1;
  //pazi ker je sou path iz const char * -> String
  #define FORMAT_SPIFFS_IF_FAILED true
  /*    KONEC SPIFFS    */

  //---------------------------------------------------------

bool readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);
  bool fail=0;
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return fail;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
  fail=1;
  return fail;
}

String readFile1Char(fs::FS &fs, const char *path, bool keep_open) {
  Serial.printf("Reading file: %s\r\n", path);
  bool fail=0;
  File file = fs.open(path);
  while (!file || file.isDirectory()) {
  delay(1);
  File file = fs.open(path);
  }
  String znak="";

  //while(znak=="")
  //{
  if (file.available()) {
    znak=file.read();
    if(!keep_open)file.close();
  }
  //}
  else
  {
    return "H";
    if(!keep_open)file.close();
  }
  return znak;
}

void writeFile(fs::FS &fs, /*const char * */ String path, /*const char * */String message) {
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

void SPIFF2BUFF(fs::FS &fs, String path)
  {
    File file = fs.open(path);
    if (!file || file.isDirectory()) {
      Serial.println("- failed to open file for reading");
      return;
    }

    Serial.println("- read from file:");
    
    for(uint8_t skip=0;skip<IME_SLIKE;skip++)
    {
      file.read(); //preskocis ime slike pol pa začneš pr podatkih
    }
    
      String temp;
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
        img_buffer[row][col]=string2header(beseda);
        col++;
        beseda="";
        str_rdy=0;
        char_count=0;
      }
      
      }
      Serial.println(" ");
    
  }
    file.close();
  }


void setup() {
  //tft.initR(INITR_BLACKTAB);
  //tft.setRotation(0);
  //tft.fillScreen(ST7735_BLACK);

  pinMode(TIPKA,INPUT_PULLUP);
  
  #if DEBUG
  Serial.begin(115200);
  delay(10);
  #endif

  WIFI();

  Serial.println(spiffs_boot());
  
  wifi_off();


  //zapiši pravilno sliko v img_buffer
  
}

void loop() {
  //gsheets2spiff();
  //delay(sendInterval);
}

uint8_t spiffs_boot() //VRNE cifro za SLIKO/TEXT ZA PRIKAZ
  { uint8_t state_code=0;
    uint8_t pic_of_the_day=0;
  /*
    0= neutral
    1= new/formated
    2= ready to read
  */
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    spiffs_flag=0;
  }
  /*
    Log.txt (brez \n):
    _ week_day (1-7)
    __ day_of_month (1-31)
    __ month (1-12)
    ____ year ()
  */
  if(spiffs_flag)
  {
    if(!readFile(SPIFFS,"/log.txt"))
    {//ce ni log.txt filea se ga naredi. 
    //Torej je nov boot in se naredi tud slike na novo
      writeFile(SPIFFS,"/log.txt","");
      Serial.println("naredu log");
      for(uint8_t name_list;name_list<NUM_DAYS;name_list++)
      {
        writeFile(SPIFFS,imena_dir[name_list],"");
      }
      state_code=1;
    }
    else
    { //če log.txt obstaja ga resetiraš, pred tem primerjaš datume
    uint8_t date_info[10];
    if(WiFi.status()==WL_CONNECTED)
    {
    Serial.print("updejtnu cajt ");
    struct tm timeinfo;    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println(getLocalTime(&timeinfo));
      //printLocalTime();
    wifi_off();
    
    date_info[0]=timeinfo.tm_wday;
    date_info[0]=date_info[0]-(date_info[0]!=0)+6*(date_info[0]==0);
    date_info[1]=(timeinfo.tm_mday)/10;
    date_info[2]=(timeinfo.tm_mday)%10;
    date_info[3]=(timeinfo.tm_mon+1)/10;
    date_info[4]=(timeinfo.tm_mon+1)%10;
    date_info[5]=(timeinfo.tm_year)/100-1;
    date_info[6]=((timeinfo.tm_year)%100)/10;
    date_info[7]=(((timeinfo.tm_year)%100)%10);
    //date_info[8]=timeinfo.tm_year-date_info[5]-date_info[6]-date_info[7];
    //Serial.println(date_info[0]);
    uint8_t date_state=0;
      for(uint8_t date_check=0;date_check<8;date_check++)
      { Serial.println(date_info[date_check]);
        String cifra=String(readFile1Char(SPIFFS,"/log.txt",date_check!=8));
        if(cifra.toInt()!=date_info[date_check])
          {
            if(date_check==0)date_state+=1;
            else date_state+=2;
          }
      }
    Serial.println(date_state);
    }
    deleteFile(SPIFFS,"/log.txt");  
    uint32_t vsota_datum=0;
    for(uint8_t datum=0;datum<8;datum++)vsota_datum+=(date_info[datum]*pow(10,7-datum));
    writeFile(SPIFFS,"/log.txt",String(vsota_datum));
    Serial.println(vsota_datum);
    readFile(SPIFFS,"/log.txt");
    return date_info[0];
    }
    return 10;
    }
  }


bool WIFI()
  {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_home, password_home);

  uint8_t wifi_attempt=0;
  bool wifi_success=0;
  while ((WiFi.status() != WL_CONNECTED) && (wifi_attempt<5)) {
    delay(500);
    wifi_attempt++;
    Serial.print(".");
  }
  wifi_attempt=0;
  if(WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(ssid_hotspot,password_hotspot);
    while ((WiFi.status() != WL_CONNECTED) && (wifi_attempt<5)) {
    delay(500);
    wifi_attempt++;
    Serial.print("."); 
  }
  if(WiFi.status() == WL_CONNECTED)wifi_success=1;
  }
  return wifi_success;
  }

void wifi_off()
  {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

void gsheets2spiff(void) 
  {
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


int string2header(String s) 
  {
  int x = 0;
  uint8_t cnt=0; // char count
  while(cnt!=4)//mora prebrati vse 4 podatke 0x _ _ _ _
  {
  cnt++;
  char c = s.charAt(cnt);
  
  if (c >= '0' && c <= '9')x += (c - '0')*pow(16,cnt); 
  else if (c >= 'A' && c <= 'F')x += ((c - 'A') + 10)*pow(16,cnt); 
  else if (c >= 'a' && c <= 'f')x += ((c - 'a') + 10)*pow(16,cnt);
  else break; 
  }
  return x;
  }
