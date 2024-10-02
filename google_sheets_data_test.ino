//Deployment code:
//AKfycbxZ6U2Afg7i92aL91parhMBiAG7RL4J4UDH6ZdVWtUbktdmV2u6uC8lGxvphJVRBMnQ

// sicer pa SPIFFS : https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm
/*to do
  - defektne slike!!!!!!!!
 ? - error http request!!!!
  - če nima neta - pravilno naložen datum - ista zadeva iz spiffa
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

#define pushButton_pin   17

/*Things to change */

const char * ssid_hotspot = "monika";
const char * password_hotspot = "mladizmaji";

//AirTies_Air4920_844H
const char * ssid_home = "ALHN-F4DA";
const char * password_home = "c57nvgLUA2";

const String GOOGLE_SCRIPT_ID = "AKfycbwYkMkvborS_5SNyjGM7uDFZ0-i8JUyYT4Y2dDI7xBj6qJZeGnjEpV9wwGPVe_-uWE";

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
const String imena_dir[8] = {"/slika1.txt", "/slika2.txt", "/slika3.txt", "/slika4.txt", "/slika5.txt", "/slika6.txt", "/slika7.txt", " "};
const String spif_log = "/log.txt";

//locljivost slike:
#define NUM_ROW 160
#define NUM_COL 128

uint16_t img_buffer[NUM_ROW + 1][NUM_COL + 1];

#define MAX_CHAR_AT_ONCE 100
#define MAX_TEXT_SPLITS 4
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
uint8_t izbrani_dan=0;
/* KONEC DATUM STUFFA         */

//-------------------------------------------------------

/*      SPIFFS      */
bool spiffs_flag = 1;
//pazi ker je sou path iz const char * -> String
#define FORMAT_SPIFFS_IF_FAILED true
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

bool readFile(fs::FS &fs, /*const char * */ String path) {
  //Serial.printf("Reading file: %s\r\n", path);
  bool fail = 0;
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    //Serial.println("- failed to open file for reading");
    return fail;
  }

  //Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
  fail = 1;
  return fail;
}

String readFile1Char(fs::FS &fs, const char *path, uint8_t which_char) {
  //Serial.printf("Reading file: %s\r\n", path);
  bool fail = 0;
  File file = fs.open(path);
  while (!file || file.isDirectory()) {
    delay(1);
    File file = fs.open(path);
  }
  String znak = "";

  //while(znak=="")
  //{
  if (file.available()) {
    for (uint8_t bruh = 0; bruh < which_char; bruh++)file.read();
    znak = file.read();
    file.close();
  }
  //}
  else
  {
    return "H";
    file.close();
  }
  return znak;
}

void writeFile(fs::FS &fs, /*const char * */ String path) {//, /*const char * */String message
  //Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    //Serial.println("- failed to open file for writing");
    return;
  }
  /*
    if (file.print(message)) {
    Serial.println("- file written");
    } else {
    Serial.println("- write failed");
    }*/
  file.close();
}

void appendFile(fs::FS &fs, /*const char * */ String path, String message) {
  //Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    //Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    //Serial.println("- message appended");
  } else {
    //Serial.println("- append failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, String path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
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
{ //Serial.println(path);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    //Serial.println("- failed to open file for reading");
    return;
  }

  char temp='a';
  String beseda = "";
  bool str_rdy = 0;
  uint8_t char_count = 0;
  uint16_t col = 0;
  uint16_t row = 0;

  uint8_t bl=0;

  //while(row<NUM_ROW && (temp.toInt()!=(-1)))
  Serial.println("Zapisujem slike");
  while ((((temp - '0') <= 9) || ((temp - 'a') <= 5) || (temp == 'x')) && (temp != 'Y'))
  { //premisli ce rabis kje file.available()
  //Serial.print(row);Serial.print(" | ");Serial.println(col);
    temp = file.read();
    if ((((temp - '0') <= 9) || ((temp - 'a') <= 5) || (temp == 'x')) && (temp != 'Y'))
    {
      //char temp2 = temp.charAt(0);
      //Serial.print(col); Serial.print(" "); Serial.print(row); Serial.print(" "); Serial.println(temp);// Serial.print(" "); Serial.println(temp2);
      if (col == (NUM_COL-1))
      {
        col = 0;
        row++;
        bl=0;
      }
      else {
        if (temp == 'x')str_rdy = 1;
        else if (str_rdy == 1)
        { //Serial.println("-------"); Serial.println("");
          beseda.concat(temp);
          char_count++;
        }
        if (char_count == 4)
        {bl++;
          //if(bl==50){Serial.print(beseda); Serial.print(" | ");Serial.print(row);Serial.print(" | ");Serial.println(col);}
          //Serial.println(string2header(beseda));
          img_buffer[row][col] = string2header(beseda);
          col++;
          beseda = "";
          str_rdy = 0;
          char_count = 0;
        }
      }
    }
    else Serial.println("zajeb");
  }
  //Serial.print("racun:");Serial.println(row*120+col);
  //ko mine zapisovanje slike se zapiše še text:
  Serial.println("Konec zapisovanja slike");
  beseda = "";
  char_count = 0;
  bool at_least_one_text = 0;
  while (file.available() && (char_count < MAX_TEXT_SPLITS))
  { //Serial.print("GLEDAM TEXT ");
    temp = file.read();
    //Serial.println(temp);
    beseda.concat(temp);
    //Serial.println(beseda);
    if (beseda.length() == MAX_CHAR_AT_ONCE)
    {
      text_buffer[char_count] = beseda;
      //Serial.println(beseda);
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
  //Serial.print("Curent text pages: ");Serial.println(current_text_pages);
  file.close();
}


void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST7735_BLACK);
  dacWrite(TFT_LED, 255);

  pinMode(pushButton_pin, INPUT_PULLUP);
  attachInterrupt(pushButton_pin, isr, FALLING);

#if DEBUG
  Serial.begin(115200);
  delay(10);
#endif

  WIFI();
  izbrani_dan = spiffs_boot();
  //SPIFF2BUFF(SPIFFS, imena_dir[izbrani_dan]);
  SPIFF2BUFF(SPIFFS,slikca);
  //PrikazSlike();
  wifi_off();
  readFile(SPIFFS,"/slikca.txt");
/*
#if DEBUG
  Serial.println("");
  for (uint8_t vrstice = 0; vrstice < 160; vrstice++)
  {
    for (uint8_t stolpci = 0; stolpci < 120; stolpci++)
    {
      Serial.print(img_buffer[vrstice][stolpci]);
      Serial.print("|");
    }
  }
  Serial.println("konec slike");
  for (uint8_t txt = 0; txt < 4; txt++)
  {
    Serial.println(text_buffer[txt]);
  }
#endif
*/

}

void loop() {
if(tipka_change==1)
{ tipka_change=0;
  if(tipka>(current_text_pages))tipka=0;
  if(tipka==0)PrikazSlike();
  else PrikazTexta(tipka-1);
  if(tipka>6)tipka=0;
  //SPIFF2BUFF(SPIFFS, imena_dir[tipka]);
  //PrikazSlike();
}


}

uint8_t spiffs_boot(void) //VRNE cifro za SLIKO/TEXT ZA PRIKAZ
{ uint8_t state_code = 0;
  /*
    0= neutral
    1= new/formated
    2= ready to read
  */
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
#if DEBUG
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
#if DEBUG
      Serial.println("naredu log");
#endif
      state_code = 1;
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
        //date_info[8]=timeinfo.tm_year-date_info[5]-date_info[6]-date_info[7];
        //Serial.println(date_info[0]);
        uint8_t date_state = 0;
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
      else date_info[0]=readFile1Char(SPIFFS,"/log.txt",0).toInt();

/*      for (uint8_t name_list; name_list < NUM_DAYS; name_list++)
      {
        if (!availableFile(SPIFFS, imena_dir[name_list])) {
          writeFile(SPIFFS, imena_dir[name_list]);
        }
      }*/

      if (!availableFile(SPIFFS, slikca)) {
          writeFile(SPIFFS,slikca);}

      //če se je datum spremenil in je ponedeljek, updejta nabor:
      /*if(date_state!=0 && date_info[0]==0)*/gsheets2spiff();

      return date_info[0];
    }
    return 10;
  }
}


bool WIFI()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setTextWrap(true);
  tft.setCursor(0, 0);
  tft.print("WiFi Doma");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_home, password_home);

  
  uint8_t wifi_attempt = 0;
  bool wifi_success = 0;
  while ((WiFi.status() != WL_CONNECTED) && (wifi_attempt < 5)) {
    delay(500);
    wifi_attempt++;
    //Serial.print(".");
  }
  wifi_attempt = 0;
  if (WiFi.status() != WL_CONNECTED)
  {
  tft.setCursor(0, 20);
  tft.print("Hotspot");
  tft.setCursor(0, 40);
  tft.print("ime: Linc");
  tft.setCursor(0, 60);
  tft.print("geslo:PINC");
  
    WiFi.begin(ssid_hotspot, password_hotspot);
    while ((WiFi.status() != WL_CONNECTED) && (wifi_attempt < 5)) {
      delay(500);
      wifi_attempt++;
      //Serial.print(".");
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

//zakomentirano da louda samo 1 sliko za 1 dan
 // for (uint8_t slika_no = 0; slika_no < 7; slika_no++)
 // { //sheets sam ve kaj ti mora podat
    //deleteFile(SPIFFS, imena_dir[izbrani_dan]);
    //writeFile(SPIFFS, imena_dir[izbrani_dan]);

    if (WiFi.status() == WL_CONNECTED)
    {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.print("LoudaSliko");  
    deleteFile(SPIFFS,slikca);
    writeFile(SPIFFS, slikca);
    for (int slika_vrstica = 0; slika_vrstica < 4; slika_vrstica++)
    {
      http.begin(url.c_str()); //Specify the URL and certificate
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      String payload;
      int httpCode = 0;
      httpCode = http.GET();
      Serial.print("Pobral file dolg:");
        //ZAFUK VRJETNO TUKAJ
      if (httpCode > 0) { //Check for the returning code
        payload = http.getString();
        uint32_t substring_length = 500;
        Serial.println(payload.length());
        for (uint32_t sub = 0; sub < ((payload.length() / substring_length) + 1); sub++)
        { appendFile(SPIFFS, /*imena_dir[izbrani_dan]*/ slikca, payload.substring(sub * substring_length, (sub + 1)*substring_length));
/*
          #if DEBUG
          Serial.print(sub); Serial.print(" ");
          Serial.print(payload.substring(sub * substring_length, (sub + 1)*substring_length));
          Serial.println(" ");
          #endif*/
        }

      }
      else { //ne bi smelo priti do tega
      Serial.println("Error on HTTP request");
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
    tft.setCursor(0, 20);
    tft.print("za loudat"); 
    }
  //}
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
  for (uint32_t rows = 0 ; rows < NUM_ROW; rows++)
  {
    for (uint32_t cols = 0 ; cols < NUM_COL; cols++)
    {
      tft.drawPixel(cols + shift,rows + shift, img_buffer[rows][cols] );
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
  //char text[] = text_buffer[page];
  for(uint16_t i=0;i<MAX_CHAR_AT_ONCE;i++)
  {

    tft.print(text_buffer[page].charAt(i));
  }
}
