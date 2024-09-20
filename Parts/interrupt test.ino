#define pushButton_pin   17
uint8_t neki=0;
 
void IRAM_ATTR isr()
{
  static uint32_t timek=0;
  if((millis()-timek)>=300)neki++;
  if(neki>=5)neki=0;
  timek=millis();
}
 
void setup()
{
  Serial.begin(115200);
  pinMode(pushButton_pin, INPUT_PULLUP);
  attachInterrupt(pushButton_pin, isr, FALLING);
}
 
void loop()
{
  Serial.println(neki);
  delay(100);
}
