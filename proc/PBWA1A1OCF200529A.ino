#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define VERSION "PBWA1A1OCF200529A"

// EEPROM OFFSET
#define EE_NEOPIXEL_OFFSET 0
#define EE_NEOPIXEL_NUM 6

// Atmega328 Pin Mapping
#define BATTERY_PIN A1
#define TOUCH_PIN 2
#define DC_CONN_PIN 3
#define BUTTON_PIN 4
#define LED_PIN 5
#define PIR_PIN 8
#define POWER_EN_PIN 17

// OPCODE
#define MSG_TYPE_VERSION 10
#define MSG_TYPE_HALT 11
#define MSG_TYPE_RESET 12
#define MSG_TYPE_BUTTON 13
#define MSG_TYPE_DC_CONN 14
#define MSG_TYPE_BATTERY 15
#define MSG_TYPE_REBOOT 17

#define MSG_TYPE_NEOPIXEL 20
#define MSG_TYPE_NEOPIXEL_FADE 21
#define MSG_TYPE_NEOPIXEL_BRIGHTNESS 22
#define MSG_TYPE_NEOPIXEL_EACH 23
#define MSG_TYPE_NEOPIXEL_FADE_EACH 24
#define MSG_TYPE_NEOPIXEL_LOOP 25
#define MSG_TYPE_NEOPIXEL_OFFSET_SET 26
#define MSG_TYPE_NEOPIXEL_OFFSET_GET 27
#define MSG_TYPE_NEOPIXEL_EACH_ORG 28

#define MSG_TYPE_PIR 30
#define MSG_TYPE_TOUCH 31

#define MSG_TYPE_SYSTEM 40

#define MSG_TYPE_TEST 97
#define MSG_TYPE_INVALID_PKT 98
#define MSG_TYPE_MGMT 99

// Interval (ms)
#define SENSOR_CHK_INTV 1000
#define BAT_CHK_INTV 5000

// interval 10s / 1s log
unsigned long t_bat_chk, t_sensor_chk;

// Count while button clicked. (1s -> 1 count )
int gButton_clicked_cnt;

int gTOUCH_en;
int gPIR_en;

// value
int gDC_value;
int gDC_prior_value = -1;
int gPIR_value;
int gPIR_prior_value = -1;
int gButton_value;
int gHalt_value;
int gReset_value;
int gTouch_value;
long gBattery_value;

// Neopixel
#define L_EYE_IDX 1
#define R_EYE_IDX 0 
int gRed_R, gGreen_R, gBlue_R, gRed_L, gGreen_L, gBlue_L;
float gColorOffset[EE_NEOPIXEL_NUM];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, LED_PIN, NEO_GRB + NEO_KHZ800);
SoftwareSerial serial(10, 9); // RX, TX
String gSysMsg;
String gPkt;

void setup(){
  pinMode(POWER_EN_PIN, OUTPUT);
  digitalWrite(POWER_EN_PIN, HIGH);
  
  pinMode(PIR_PIN, INPUT);
  pinMode(DC_CONN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DC_CONN_PIN), eventDC, CHANGE);

  pinMode(BATTERY_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TOUCH_PIN), eventTouch, FALLING);
  pinMode(BUTTON_PIN, INPUT);

  for(int i=EE_NEOPIXEL_OFFSET;i<EE_NEOPIXEL_NUM;i++){
    gColorOffset[i] = EEPROM.read(i)/255.0;
  }

  strip.begin();
  strip.setBrightness(64);
  // Initialation Profile
  initConfig();

  // To Raspberrypi 
  serial.begin(9600);
  serial.setTimeout(10);
}

void loop(){
  if(serial.available()){
    gPkt = "";
    gPkt = serial.readStringUntil('!');
    gPkt.trim();

    if(gPkt[0] != '#'){
      sendMSG(MSG_TYPE_INVALID_PKT, gPkt);
    }
    else{
      gPkt.remove(gPkt.indexOf('#'),1);
  
      int first = gPkt.indexOf(':');
      int second = gPkt.indexOf(':', first+1);
      int opcode = gPkt.substring(0, first).toInt();
      String data = gPkt.substring(first+1, second);

      switch(opcode){
        case MSG_TYPE_VERSION:
          sendMSG(MSG_TYPE_VERSION, VERSION);
          break;
        case MSG_TYPE_HALT:
          sendMSG(MSG_TYPE_HALT, "ok");
          halt();
          break;
        case MSG_TYPE_RESET:
          sendMSG(MSG_TYPE_RESET, "ok");
          break;
        case MSG_TYPE_BUTTON:
          sendMSG(MSG_TYPE_BUTTON, "ok");
          break;
        case MSG_TYPE_DC_CONN:
          sendMSG(MSG_TYPE_DC_CONN,(gDC_value == HIGH?"on":"off"));
          break;
        case MSG_TYPE_BATTERY:
          sendMSG(MSG_TYPE_BATTERY, String(gBattery_value));
          break;
        case MSG_TYPE_REBOOT:
          sendMSG(MSG_TYPE_REBOOT, "ok");
          initConfig();
          break;
        case MSG_TYPE_NEOPIXEL:
          if(gButton_value){
            sendMSG(MSG_TYPE_NEOPIXEL, "button is pressed");      
          }
          else{
            setNeopixel(data);
            sendMSG(MSG_TYPE_NEOPIXEL, "ok");
          }
          break;
        case MSG_TYPE_NEOPIXEL_FADE:
          if(gButton_value){
            sendMSG(MSG_TYPE_NEOPIXEL_FADE, "button is pressed");      
          }
          else{        
            setNeopixelFade(data);
            sendMSG(MSG_TYPE_NEOPIXEL_FADE, "ok");
          }
          break;
        case MSG_TYPE_NEOPIXEL_BRIGHTNESS:
          strip.setBrightness(data.toInt());
          strip.show();
          sendMSG(MSG_TYPE_NEOPIXEL_BRIGHTNESS, "ok");
          break;
        case MSG_TYPE_NEOPIXEL_EACH:
          if(gButton_value){
            sendMSG(MSG_TYPE_NEOPIXEL_EACH, "button is pressed");      
          }
          else{
            setNeopixelEach(data);
            sendMSG(MSG_TYPE_NEOPIXEL_EACH, "ok");
          }
          break;
        case MSG_TYPE_NEOPIXEL_FADE_EACH:
          if(gButton_value){
            sendMSG(MSG_TYPE_NEOPIXEL_FADE_EACH, "button is pressed");      
          }
          else{
            setNeopixelFadeEach(data);
            sendMSG(MSG_TYPE_NEOPIXEL_FADE_EACH, "ok");
          }
          break;
        case MSG_TYPE_NEOPIXEL_LOOP:
          if(gButton_value){
            sendMSG(MSG_TYPE_NEOPIXEL_LOOP, "button is pressed");      
          }
          else{
            setNeopixelLoop(data.toInt());
            sendMSG(MSG_TYPE_NEOPIXEL_LOOP, "ok");
          }
          break;
        case MSG_TYPE_NEOPIXEL_OFFSET_SET:
          setNeopixeOffset(data);
          sendMSG(MSG_TYPE_NEOPIXEL_OFFSET_SET, "ok");
          break;
        case MSG_TYPE_NEOPIXEL_OFFSET_GET:
          gPkt = "";
          for(int i=0;i<6;i++){
            gPkt += String(int(gColorOffset[i]*255.0));
            if(i != 5)
              gPkt += ",";
          }
          sendMSG(MSG_TYPE_NEOPIXEL_OFFSET_GET, gPkt);
          break;
        case MSG_TYPE_NEOPIXEL_EACH_ORG:
          setNeopixelEachOrg(data);
          sendMSG(MSG_TYPE_NEOPIXEL_EACH_ORG, "ok");
          break;  
        case MSG_TYPE_PIR:
          gPIR_en = data=="on"?1:0;
          sendMSG(MSG_TYPE_PIR, "ok");
          break;
        case MSG_TYPE_TOUCH:
          gTOUCH_en = data=="on"?1:0;
          sendMSG(MSG_TYPE_TOUCH, "ok");
          break;
        case MSG_TYPE_INVALID_PKT:
          sendMSG(MSG_TYPE_INVALID_PKT, "ok");
          break;
        case MSG_TYPE_SYSTEM:
          // gPIR_value, gVIB_value, gDC_value, gButton_value, gReset_value, gHalt_value
          gSysMsg += gPIR_en?(gPIR_value?"person-":(gPIR_prior_value!=gPIR_value?"nobody-":"-")):"-";
          gPIR_prior_value = gPIR_value;
          gSysMsg += gTouch_value?"touch-":"-";
          gTouch_value = 0;
          gSysMsg += gDC_prior_value!=gDC_value?(gDC_value?"on-":"off-"):"-";
          gDC_prior_value = gDC_value;
          gSysMsg += gButton_value?"on-":"-";
          gSysMsg += gReset_value?"on-":"-";
          gSysMsg += gHalt_value?"on":"";
          sendMSG(MSG_TYPE_SYSTEM, gSysMsg);

          if(gReset_value){
            for(int i=0; i < 6; i++){
              setNeopixel("0,255,0");
              delay(500);
              setNeopixel("0,0,0");
              delay(500);
            }
          }

          if(gHalt_value){
            halt();
          }
          gReset_value = 0;
          gHalt_value = 0;
          gSysMsg = "";
          break;
        default:
          sendMSG(MSG_TYPE_INVALID_PKT, (String(opcode) + ":" + String(data)));
          break;
      }
    }
  }

  unsigned long t_current = millis();
  // Sensor check
  if(t_current - t_sensor_chk > SENSOR_CHK_INTV){
    if(gPIR_en){
      gPIR_value = digitalRead(PIR_PIN)==HIGH?1:0;
    }
    
    // for offline
    if(gHalt_value){
      halt();
    }

    eventButton();
    t_sensor_chk = t_current;
  }

  // Battery check
  if(t_current - t_bat_chk > BAT_CHK_INTV){
    updateBattery();
    t_bat_chk = t_current;
  }
}

void sendMSG(int msg_type, String msg){
  serial.println("#" + String(msg_type) + ":" + msg + "!");
}

void initConfig(){
  // Initialization
  // log, sensor, button time 
  t_sensor_chk = 0;
  t_bat_chk = 0;

  // Count while button clicked. (1s -> 1 count )
  gButton_clicked_cnt = 0;

  // Previous PIR value
  gPIR_value = LOW;

  gPIR_en = 0;
  gRed_R = 0;
  gGreen_R = 0;
  gBlue_R = 0;
  gRed_L = 0;
  gGreen_L = 0;
  gBlue_L = 0;

  // DC_CONN state
  gDC_value = digitalRead(DC_CONN_PIN);

  // Battery value init
  gBattery_value = getBattery();

  setNeopixel("0,0,0");
  setNeopixelFade("236,56,67,10");
  setNeopixelFade("247,193,0,10");  
  setNeopixelFade("0,186,201,10");
  setNeopixelFade("140,124,91,10");
  setNeopixelFade("178,209,53,10");
  setNeopixelFade("124,89,119,10");
  setNeopixelFade("220,220,220,30");
}

void eventDC(){
  gDC_value = digitalRead(DC_CONN_PIN)==HIGH?1:0;
}

void eventTouch(){
  //gTouch_value = digitalRead(TOUCH_PIN)==LOW?1:0; // LOW -> Touch
  gTouch_value = 1;
}

void eventButton(){
  gButton_value = digitalRead(BUTTON_PIN)==LOW?1:0; // LOW -> Button-on
  if(gButton_value == 1){ 
    gButton_clicked_cnt++;

    if(gButton_clicked_cnt == 1){
      setNeopixel("0,0,0");
    }

    if(gButton_clicked_cnt == 5){
      setNeopixel("255,0,0");
    }

    if(gButton_clicked_cnt == 20){
      setNeopixel("0,255,0");
      gReset_value = 1;
    }
  }
  else{
    if(gButton_clicked_cnt >= 5 && gButton_clicked_cnt < 20){
      gHalt_value = 1;
    }      
    gButton_clicked_cnt = 0;      
  }
}

void updateBattery(){
  long prev_bat = gBattery_value;

  gBattery_value = getBattery();
  gBattery_value = (gBattery_value/5)*5;
  // battery 0% --> halt 
  if(prev_bat == 0 && gBattery_value == 0){
    gHalt_value = 1;
  }
}

long getBattery(){
/*
  new: 400 - 530 (0 - 100)

   adc_val   | battery
  ======================
   530 ~     | 100%
   530 ~ 441 | 100% ~ 10%
   440 ~ 400 | 10%  ~  0%
*/
  long bat;
  int adc_val = analogRead(BATTERY_PIN);
  if (gDC_value == HIGH) adc_val -= 8; // for DC-CONN
  
  if(adc_val > 440){ // 530 ~ 441
    bat = adc_val - 430;
  }
  else { // 440 ~ 400
    bat = (adc_val - 400)/4;
  }

  if(bat > 100) bat = 100;
  if(bat < 0 ) bat = 0;

  return bat;
}

void setPixelColor(int sel, int r, int g, int b){
  if(sel == 0){
    strip.setPixelColor(sel, strip.Color(r*gColorOffset[0],g*gColorOffset[1],b*gColorOffset[2]));
  }
  else{
    strip.setPixelColor(sel, strip.Color(r*gColorOffset[3],g*gColorOffset[4],b*gColorOffset[5]));    
  }
}

void setNeopixel(String str){
  int r, g, b;
  int first = str.indexOf(",");
  int second = str.indexOf(",",first+1);
  int strlength = str.length();
  
  r = str.substring(0, first).toInt();
  g = str.substring(first+1, second).toInt();
  b = str.substring(second+1,strlength).toInt();

  gRed_R = gRed_L = r;
  gGreen_R = gGreen_L = g;
  gBlue_R = gBlue_L = b;

  setPixelColor(0, gRed_R, gGreen_R, gBlue_R);
  setPixelColor(1, gRed_L, gGreen_L, gBlue_L);
  strip.show();  
}

void setNeopixelEach(String str){
  int r1, g1, b1;
  int r2, g2, b2;
  int first = str.indexOf(",");
  int second = str.indexOf(",",first+1);
  int third = str.indexOf(",",second+1);
  int fourth = str.indexOf(",",third+1);
  int fifth = str.indexOf(",",fourth+1);
  int strlength = str.length();
  
  r1 = str.substring(0, first).toInt();
  g1 = str.substring(first+1, second).toInt();
  b1 = str.substring(second+1,strlength).toInt();
  r2 = str.substring(third+1, fourth).toInt();
  g2 = str.substring(fourth+1, fifth).toInt();
  b2 = str.substring(fifth+1,strlength).toInt();

  gRed_R = r1;
  gGreen_R = g1;
  gBlue_R = b1;
  gRed_L = r2;
  gGreen_L = g2;
  gBlue_L = b2;

  setPixelColor(0, gRed_R, gGreen_R, gBlue_R);
  setPixelColor(1, gRed_L, gGreen_L, gBlue_L);
  strip.show();  
}

void setNeopixelFade(String str){
  int r, g, b, d;
  int first = str.indexOf(",");
  int second = str.indexOf(",",first+1);
  int third = str.indexOf(",",second+1);
  int strlength = str.length();
  
  r = str.substring(0, first).toInt();
  g = str.substring(first+1, second).toInt();
  b = str.substring(second+1,third).toInt();
  d = str.substring(third+1,strlength).toInt();

  while(1){
    if(r != gRed_R){
      if(r > gRed_R)gRed_R++;
      else gRed_R--;        
    }
    
    if(g != gGreen_R){
      if(g > gGreen_R)gGreen_R++;
      else gGreen_R--;        
    }

    if(b != gBlue_R){
      if(b > gBlue_R)gBlue_R++;
      else gBlue_R--;
    }
    
    if(r != gRed_L){
      if(r > gRed_L)gRed_L++;
      else gRed_L--;        
    }
    
    if(g != gGreen_L){
      if(g > gGreen_L)gGreen_L++;
      else gGreen_L--;
    }

    if(b != gBlue_L){
      if(b > gBlue_L)gBlue_L++;
      else gBlue_L--;
    }
    
    setPixelColor(0, gRed_R, gGreen_R, gBlue_R);
    setPixelColor(1, gRed_L, gGreen_L, gBlue_L);
    strip.show();    

    if(gRed_R == r && gGreen_R == g && gBlue_R == b && gRed_L == r && gGreen_L == g && gBlue_L == b)
      break;
    delay(d);
  }
}

void setNeopixelFadeEach(String str){
  int d;
  int r1, g1, b1;
  int r2, g2, b2;
  int first = str.indexOf(",");
  int second = str.indexOf(",",first+1);
  int third = str.indexOf(",",second+1);
  int fourth = str.indexOf(",",third+1);
  int fifth = str.indexOf(",",fourth+1);
  int sixth = str.indexOf(",",fifth+1);
  int strlength = str.length();
  
  r1 = str.substring(0, first).toInt();
  g1 = str.substring(first+1, second).toInt();
  b1 = str.substring(second+1,strlength).toInt();
  r2 = str.substring(third+1, fourth).toInt();
  g2 = str.substring(fourth+1, fifth).toInt();
  b2 = str.substring(fifth+1, sixth).toInt();
  d = str.substring(sixth+1,strlength).toInt();

  while(1){
    if(r1 != gRed_R){
      if(r1 > gRed_R)gRed_R++;
      else gRed_R--;        
    }
    
    if(g1 != gGreen_R){
      if(g1 > gGreen_R)gGreen_R++;
      else gGreen_R--;        
    }

    if(b1 != gBlue_R){
      if(b1 > gBlue_R)gBlue_R++;
      else gBlue_R--;
    }
    
    if(r2 != gRed_L){
      if(r2 > gRed_L)gRed_L++;
      else gRed_L--;        
    }
    
    if(g2 != gGreen_L){
      if(g2 > gGreen_L)gGreen_L++;
      else gGreen_L--;
    }

    if(b2 != gBlue_L){
      if(b2 > gBlue_L)gBlue_L++;
      else gBlue_L--;
    }
    
    setPixelColor(0, gRed_R, gGreen_R, gBlue_R);
    setPixelColor(1, gRed_L, gGreen_L, gBlue_L);
    strip.show();    

    if(gRed_R == r1 && gGreen_R == g1 && gBlue_R == b1 && gRed_L == r2 && gGreen_L == g2 && gBlue_L == b2)
      break;
    delay(d);
  }
}

void setNeopixelLoop(int d){
  int r,g,b;
  int _r = gRed_R = gRed_L = 100;
  int _g = gGreen_R = gGreen_L = 100;
  int _b = gBlue_R = gBlue_L = 100;
  int i = 0;
  int color_arr[][3] = {{200,50,30}, {150,200,50}, {100,50,100}, {gRed_R,gGreen_R,gBlue_R}};
  
  while(1){
   r = color_arr[i][0];
   g = color_arr[i][1];
   b = color_arr[i][2];

    if(r != _r){
      if(r > _r)_r++;
      else _r--;     
    }
    
    if(g != _g){
      if(g > _g)_g++;
      else _g--;     
    }

    if(b != _b){
      if(b > _b)_b++;
      else _b--;
    }

    setPixelColor(0, _r, _g, _b);
    setPixelColor(1, _r, _g, _b);
    strip.show();

    if(r == _r && g == _g && b == _b){
      i++;
      if (i == 4){
        break;
      }
    }
    delay(d);
  }
}

void setNeopixeOffset(String str){
  int _o[6];
  int first = str.indexOf(",");
  int second = str.indexOf(",",first+1);
  int third = str.indexOf(",",second+1);
  int fourth = str.indexOf(",",third+1);
  int fifth = str.indexOf(",",fourth+1);
  int strlength = str.length();
  
  _o[0] = str.substring(0, first).toInt();
  _o[1] = str.substring(first+1, second).toInt();
  _o[2] = str.substring(second+1,strlength).toInt();
  _o[3] = str.substring(third+1, fourth).toInt();
  _o[4] = str.substring(fourth+1, fifth).toInt();
  _o[5] = str.substring(fifth+1,strlength).toInt();

  for(int i=EE_NEOPIXEL_OFFSET;i<EE_NEOPIXEL_NUM;i++){
    EEPROM.write(i, _o[i]);
    gColorOffset[i] = EEPROM.read(i)/255.0;
  }
}

void setNeopixelEachOrg(String str){
  int r1, g1, b1;
  int r2, g2, b2;
  int first = str.indexOf(",");
  int second = str.indexOf(",",first+1);
  int third = str.indexOf(",",second+1);
  int fourth = str.indexOf(",",third+1);
  int fifth = str.indexOf(",",fourth+1);
  int strlength = str.length();
  
  r1 = str.substring(0, first).toInt();
  g1 = str.substring(first+1, second).toInt();
  b1 = str.substring(second+1,strlength).toInt();
  r2 = str.substring(third+1, fourth).toInt();
  g2 = str.substring(fourth+1, fifth).toInt();
  b2 = str.substring(fifth+1,strlength).toInt();

  gRed_R = r1;
  gGreen_R = g1;
  gBlue_R = b1;
  gRed_L = r2;
  gGreen_L = g2;
  gBlue_L = b2;

  strip.setPixelColor(0, strip.Color(gRed_R, gGreen_R, gBlue_R));
  strip.setPixelColor(1, strip.Color(gRed_L, gGreen_L, gBlue_L));
  strip.show();  
}

void halt(){
  for(int i=0; i < 3; i++)
  {
    setNeopixel("255,0,0");
    delay(500);
    setNeopixel("0,0,0");
    delay(500);
  }
  digitalWrite(POWER_EN_PIN, LOW); 
}
