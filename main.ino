
#define DEBUG 0
#define DEBUG_EXTRA 0

#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#include "FS.h"
#include "SPIFFS.h"

#define pushButton_pin   17

/*Things to change */

const char * ssid_hotspot = "neki_ssid";
const char * password_hotspot = "neki_pass";

const char * ssid_home = "AirTies_Air4920_844H";
const char * password_home = "phpmcy3979";

const String GOOGLE_SCRIPT_ID = "AKfycby_4va09_qYUCze3cwfkT5aQeSjw8uSs2n5UBpIPokcSbSvVQitVcZoLKlDdHaTFasM";

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
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
/*  KONEC ZASLON STUFF  */

//------------------------------------------------------------

/*    SLIKE STUFF     */
#define IME_SLIKE 10 // slika1\r\n vzame prvih 10 znakov
#define NUM_DAYS 7

const String slikca= "/slikca.txt";
const String spif_log = "/log.txt";

//locljivost slike:
#define NUM_ROW 160
#define NUM_COL 128

uint16_t img_buffer[NUM_ROW + 1][NUM_COL + 1];
#define NUM_PARTS_SLIKE 5

#define MAX_CHAR_AT_ONCE 100
#define MAX_TEXT_SPLITS 6
String text_buffer[MAX_TEXT_SPLITS];
uint8_t current_text_pages=0;
uint16_t text_char_count=0;
/* KONEC SLIKE STUFF  */

//------------------------------------------------------------
//WiFiClientSecure client;

/*       DATUM STUFF          */
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
/* KONEC DATUM STUFFA         */

//-------------------------------------------------------

/*      SPIFFS      */
bool spiffs_flag = 1;
//pazi ker je sou path iz const char * -> String
#define FORMAT_SPIFFS_IF_FAILED true
uint32_t ELEMENTS_IN_BUFF=0;
/*    KONEC SPIFFS    */

//---------------------------------------------------------
uint8_t tipka = 0;
bool tipka_change=1;
uint32_t timek = 0;

void IRAM_ATTR isr()
{
  if ((millis() - timek) >= 300){tipka++;tipka_change=1;}
  timek = millis();
}

bool readFile(fs::FS &fs, String path) {
  bool fail = 0;
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    return fail;
  }

  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
  fail = 1;
  return fail;
}

String readFile1Char(fs::FS &fs, const char *path, uint8_t which_char) {
  bool fail = 0;
  File file = fs.open(path);
  while (!file || file.isDirectory()) {
    delay(1);
    File file = fs.open(path);
  }
  String znak = "";

  if (file.available()) {
    for (uint8_t bruh = 0; bruh < which_char; bruh++)file.read();
    znak = file.read();
    file.close();
  }
  else
  {
    return "H";
    file.close();
  }
  return znak;
}

void writeFile(fs::FS &fs, String path) {

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    return;
  }

  file.close();
}

void appendFile(fs::FS &fs, String path, String message) {

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    return;
  }
  if (file.print(message)) {
  } else {
  }
  file.close();
}

void deleteFile(fs::FS &fs, String path) {
  #if DEBUG_EXTRA
  Serial.printf("Deleting file: %s\r\n", path);
  #endif
  if (fs.remove(path)) {
    #if DEBUG_EXTRA
    Serial.println("- file deleted");
    #endif
  } else {
    #if DEBUG_EXTRA
    Serial.println("- delete failed");
    #endif
  }
}

bool availableFile(fs::FS &fs, String path)
{
  File file = fs.open(path);
  if (!file || file.isDirectory() || !file.available()) {
    file.close();
    return 0;
  }
  if (file.available()) {
    file.close();
    return 1;
  }
  else {
    file.close();
    return 0;
  }
}
void SPIFF2BUFF(fs::FS &fs, String path)//TEST
{ 
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    return;
  }

  char temp='a';
  String beseda = "";
  bool str_rdy = 0;
  uint8_t char_count = 0;
  uint16_t col = 0;
  uint16_t row = 0;

  #if DEBUG_EXTRA
  Serial.println("Zapisujem slike");
  #endif
  
  while ((((temp - '0') <= 9) || ((temp - 'a') <= 5) || (temp == 'x')) && (temp != 'Y'))
  {
    temp = file.read();
    if ((((temp - '0') <= 9) || ((temp - 'a') <= 5) || (temp == 'x')) && (temp != 'Y'))
    {
      #if DEBUG_EXTRA
      Serial.print(col); Serial.print(" "); Serial.print(row); Serial.print(" "); Serial.println(temp);
      #endif
      
      if (col == (NUM_COL-1))
      {
        col = 0;
        row++;
      }
      else {
        if (temp == 'x')str_rdy = 1;
        else if (str_rdy == 1)
        {
          beseda.concat(temp);
          char_count++;
        }
        if (char_count == 4)
        {
          img_buffer[row][col] = string2header(beseda);
          ELEMENTS_IN_BUFF++;
          col++;
          beseda = "";
          str_rdy = 0;
          char_count = 0;
        }
      }
    }
    else 
    {
      #if DEBUG_EXTRA
      Serial.println("zajeb");
      #endif
    }
  }
  //ko mine zapisovanje slike se zapiše še text:
  #if DEBUG_EXTRA
  Serial.println("Konec zapisovanja slike");
  #endif DEBUG_EXTRA
  
  beseda = "";
  char_count = 0;
  bool at_least_one_text = 0;
  while (file.available() && (char_count < MAX_TEXT_SPLITS))
  {
    temp = file.read();
    beseda.concat(temp);

    if (beseda.length() == MAX_CHAR_AT_ONCE)
    {
      text_buffer[char_count] = beseda;
      beseda = "";
      char_count++;
      current_text_pages++;
      at_least_one_text = 1;
    }
  }
  if(!(char_count==MAX_TEXT_SPLITS))
  {
  text_buffer[char_count] = beseda; //zapiše ostanek texta
  current_text_pages++;
  }
  file.close();
}


void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);
  tft.fillScreen(ST7735_BLACK);
  dacWrite(TFT_LED, 255);

  BatteryCheck();
  
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setTextWrap(true);

  pinMode(pushButton_pin, INPUT_PULLUP);
  attachInterrupt(pushButton_pin, isr, FALLING);

#if DEBUG
  Serial.begin(115200);
  delay(10);
#endif
  
  if(digitalRead(pushButton_pin)!=0)
  {
  WIFI();
  uint8_t update_mby=spiffs_boot(); //ce vrne 0 se datum ni spremenil
  if((update_mby>0&&update_mby<100)&& (WiFi.status() == WL_CONNECTED))gsheets2spiff();      //če se je datum spremenil updejta sliko
  else if(update_mby==0 && (WiFi.status() == WL_CONNECTED))
  {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.print("Danasnja");
    tft.setCursor(0, 20);
    tft.print("slika");
    tft.setCursor(0, 40);
    tft.print("je ze");
    tft.setCursor(0, 60);
    tft.print("prenesena");
    tft.setCursor(0, 80);
    tft.setTextColor(ST77XX_BLUE);
    tft.print("Prikazujem");
    delay(2000);
  }
  if(update_mby!=100)SPIFF2BUFF(SPIFFS,slikca);
  else{while(1){}}
  wifi_off();
  }
  else 
  {
    spiffs_boot();
    SPIFF2BUFF(SPIFFS,slikca);
  }

}

void loop() {
if(tipka_change==1)
  { tipka_change=0;
    if(tipka>(current_text_pages))tipka=0;
    if(tipka==0)PrikazSlike();
    else PrikazTexta(tipka-1);
    if(tipka>MAX_TEXT_SPLITS)tipka=0;
  }
}

uint8_t spiffs_boot(void)
{uint8_t date_state = 0;

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
#if DEBUG_EXTRA
    Serial.println("SPIFFS Mount Failed");
#endif
    spiffs_flag = 0;
  }
  /*
    Log.txt (brez \n):
    _ week_day (1-7)
    __ day_of_month (1-31)
    __ month (1-12)
    ____ year ()
  */
  if (spiffs_flag)
  {
    if (!readFile(SPIFFS, "/log.txt"))
    { //ce ni log.txt filea se ga naredi.
      //Torej je nov boot in se naredi tud slike na novo
      writeFile(SPIFFS, "/log.txt");
    #if DEBUG_EXTRA
      Serial.println("naredu log");
    #endif
    }
    else
    { //če log.txt obstaja ga resetiraš, pred tem primerjaš datume
      int date_info[10];
      if (WiFi.status() == WL_CONNECTED)
      {
        struct tm timeinfo;
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        getLocalTime(&timeinfo);

        date_info[0] = timeinfo.tm_wday;
        date_info[0] = date_info[0] - (date_info[0] != 0) + 6 * (date_info[0] == 0);
        date_info[1] = (timeinfo.tm_mday) / 10;
        date_info[2] = (timeinfo.tm_mday) % 10;
        date_info[3] = (timeinfo.tm_mon + 1) / 10;
        date_info[4] = (timeinfo.tm_mon + 1) % 10;
        date_info[5] = (timeinfo.tm_year) / 100 - 1;
        date_info[6] = ((timeinfo.tm_year) % 100) / 10;
        date_info[7] = (((timeinfo.tm_year) % 100) % 10);
        
        for (uint8_t date_check = 0; date_check < 8; date_check++)
        {
          String cifra = readFile1Char(SPIFFS, "/log.txt", date_check);
          if ((cifra.toInt() - 48) != date_info[date_check])
          {
            if (date_check == 0)date_state += 1;
            else date_state += 2;
          }
        }
      deleteFile(SPIFFS, "/log.txt");
      uint32_t vsota_datum = 0;
      for (uint8_t datum = 0; datum < 8; datum++)vsota_datum += (date_info[datum] * pow(10, 7 - datum));
      writeFile(SPIFFS, "/log.txt");
      appendFile(SPIFFS, "/log.txt", String(vsota_datum));
      }
      else 
      {
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(0, 0);
        tft.print("Nimam wifi");
        tft.setCursor(0, 40);
        tft.setTextColor(ST77XX_BLUE);
        tft.print("Prikazujem");
        tft.setCursor(0, 80);
        tft.print("Staro");
        tft.setCursor(0, 100);
        tft.print("sliko");
        delay(2000);
      }
      bool fresh_pic_spiffs=0;
      if (!availableFile(SPIFFS, slikca)) {
          writeFile(SPIFFS,slikca);
          fresh_pic_spiffs=1;}

      if((WiFi.status() != WL_CONNECTED)&&(fresh_pic_spiffs==1))
      {
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(0, 0);
        tft.print("Nimam slike");
        tft.setCursor(0, 40);
        tft.print("Nimam wifi");
        tft.setCursor(0, 60);
        tft.setTextColor(ST77XX_BLUE);
        tft.print("Resetiraj");
        tft.setCursor(0, 80);
        tft.print("Napravo");
        delay(3000);
      }

      return date_state!=0;
    }
  }
  #if DEBUG_EXTRA
  Serial.println("Spiffs zajeb");
  #endif

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.print("Resetiraj");
  tft.setCursor(0, 40);
  tft.print("napravo");
  
  return 100;
}


bool WIFI()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.print("WiFi Doma");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_home, password_home);

  
  uint8_t wifi_attempt = 0;
  bool wifi_success = 0;
  while ((WiFi.status() != WL_CONNECTED) && (wifi_attempt < 5)) {
    delay(500);
    wifi_attempt++;
  }
  wifi_attempt = 0;
  if (WiFi.status() != WL_CONNECTED)
  {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.print("Naredi");
  tft.setCursor(0, 20);
  tft.print("Hotspot");
  tft.setCursor(0, 40);
  tft.print("ime:");
  tft.setCursor(0, 60);
  tft.setTextColor(ST77XX_GREEN);
  tft.print("Lincica"); 
  tft.setCursor(0, 80);
  tft.setTextColor(ST77XX_RED);
  tft.print("geslo:");
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(0, 100);
  tft.print("inlincnik");
  delay(2000);
    
    WiFi.begin(ssid_hotspot, password_hotspot);
    while ((WiFi.status() != WL_CONNECTED) && (wifi_attempt < 5)) {
      delay(500);
      wifi_attempt++;
    }
    if (WiFi.status() == WL_CONNECTED)wifi_success = 1;
  }
  return wifi_success;
}

void wifi_off()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void gsheets2spiff(void)//TEST
{
  HTTPClient http;
  const String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?read";

    if (WiFi.status() == WL_CONNECTED)
    {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.print("Louda");
    tft.setCursor(0, 40);
    tft.print("Sliko");  
    deleteFile(SPIFFS,slikca);
    writeFile(SPIFFS,slikca);
    for (int slika_vrstica = 0; slika_vrstica < NUM_PARTS_SLIKE; slika_vrstica++)
    {
      http.begin(url.c_str()); //Specify the URL and certificate
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      String payload;
      int httpCode = 0;
      httpCode = http.GET();
      
      #if DEBUG_EXTRA
      Serial.print("Pobral file dolg:");
      #endif
      
      if (httpCode > 0) { //Check for the returning code
        payload = http.getString();
        uint32_t substring_length = 500;
        
        #if DEBUG_EXTRA
        Serial.println(payload.length());
        #endif

        for (uint32_t sub = 0; sub < ((payload.length() / substring_length) + 1); sub++)
        { 
          appendFile(SPIFFS, slikca, payload.substring(sub * substring_length, (sub + 1)*substring_length));
        }

      }
      else { //ne bi smelo priti do tega
      
      #if DEBUG_EXTRA
      Serial.println("Error on HTTP request");
      #endif
      
      http.POST(String(slika_vrstica));
      slika_vrstica--;
      }
      http.end(); delay(100);
    }
    }
    else
    {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.print("Ni Wifija");
    tft.setCursor(0, 40);
    tft.print("za loudat"); 
    }
}


uint16_t string2header(String s)
{
  uint16_t x = 0;
  uint16_t cnt = 0; // char count
  while (cnt != 4) //mora prebrati vse 4 podatke 0x _ _ _ _
  {
    cnt++;
    char c = s.charAt(cnt - 1);

    if (c >= '0' && c <= '9')x += (c - '0') * pow(16, 4 - cnt);
    else if (c >= 'A' && c <= 'F')x += ((c - 'A') + 10) * pow(16, 4 - cnt);
    else if (c >= 'a' && c <= 'f')x += ((c - 'a') + 10) * pow(16, 4 - cnt);
    else break;
  }
  return x;
}

void PrikazSlike(void)
{ tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  uint32_t shift = 0;
  
  #if DEBUG_EXTRA
  Serial.print("Elements in buff: ");
  Serial.println(ELEMENTS_IN_BUFF);
  #endif
  
  for (uint32_t rows = 0 ; rows < NUM_ROW; rows++)
  {
    for (uint32_t cols = 0 ; cols < NUM_COL; cols++)
    {
      if((rows*NUM_COL+cols)<(ELEMENTS_IN_BUFF))tft.drawPixel(cols + shift,rows + shift, img_buffer[rows][cols]);
    }
  }
}

void PrikazTexta(uint8_t page)
{ 
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setTextWrap(true);
  tft.setCursor(0, 0);
  for(uint16_t i=0;i<MAX_CHAR_AT_ONCE;i++)
  {
    tft.print(text_buffer[page].charAt(i));
  }
}

void BatteryCheck()
{
  const uint8_t stevilo_vzorcev=10;
  float kriticna_napetost=3.4;
  pinMode(A0, INPUT);
  uint32_t napetost_baterije=0;
  
  for(uint8_t i=0;i<stevilo_vzorcev;i++){napetost_baterije+=analogRead(A0);delay(1);}
  napetost_baterije=napetost_baterije/stevilo_vzorcev;
  if(napetost_baterije<=(kriticna_napetost/5.0*1023))
    {
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_GREEN);
      tft.setTextSize(2);
      tft.setTextWrap(true);
      tft.setCursor(30, 30);
      tft.print("Prosim");
      tft.setCursor(20, 60);
      tft.print("napolni");
      tft.setCursor(15, 90);
      tft.print("baterijo");
    }
  delay(2000);
}

