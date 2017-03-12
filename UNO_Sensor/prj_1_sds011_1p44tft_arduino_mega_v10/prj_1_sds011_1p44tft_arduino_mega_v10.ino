#include <UTFT.h>
#include <dht11.h>

dht11 DHT11;

UTFT myGLCD(YYROBOT_TFT144,A2,A1,A5,A4,A3);  // Remember to change the model parameter to suit your display module!
//YYROBOT_TFT144  屏幕型号，不用修改
//SDA----A2
//SCL----A1
//CS-----A5
//RST----A4
//RS----A3
//LED---A0  UTFT库里面设定的，如果需要修改需要修改库文件

extern uint8_t SmallFont[];//原始文件在库文件的DefaultFonts.c中
extern uint8_t BigFont[];//原始文件在库文件的DefaultFonts.c中
extern uint8_t SevenSegNumFont[];//原始文件在库文件的DefaultFonts.c中
int display_rot = 0;

// PM Sensor Protocol
unsigned int start_sign = 0xAA;
unsigned int stop_sign =0xAB;
unsigned int id_sign = 0xC0;
unsigned int temp_sign =0x0; 

// Original Read Out of PM data
unsigned int pm25_h = 0;
unsigned int pm25_l = 0;
unsigned int pm100_h = 0;
unsigned int pm100_l = 0;

// State bit of FSM for Smog
unsigned int smog_state = 0; 

// Color Graded Values
unsigned int smog_front_r[10] = {255, 255, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int smog_front_g[10] = {255, 250, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int smog_front_b[10] = {255, 250, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int smog_back_r[10] = {0,   204, 255, 255, 244, 175, 90, 51,  32, 13};
unsigned int smog_back_g[10] = {102, 204, 128, 0,   13,  18,  13, 0,   32, 12};
unsigned int smog_back_b[10] = {0,   0,   0,   0,   100, 88,  67, 102, 32, 12};


word check_sum = 0;
word received_pkg = 0;
word check_failed = 0;

float temperature = 0;
float humidity = 0;

float tvoc = 0;
float ch2o = 0;

#define SETTLE_TIME 30

// Pin Assignment
#define DHT11VCC 2
#define DHT11PIN 3
#define DHT11GND 4

// Control bit of if a Display Update should be Applyied
boolean smog_updisp = true;
boolean temp_humid_updisp = true;
boolean tvoc_updisp = true;
boolean smog_in_sleep = false;

unsigned int somg_cmd_set_period_working [19]= {0xAA, 0xB4, 0x08, 0x01, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xFF, 0xFF, 0x8, 0xAB};
unsigned int somg_cmd_set_sleep_mode [19]= {0xAA, 0xB4, 0x06, 0x01, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xFF, 0xFF, 0x8, 0xAB};

float pm25  = 0.0;
float pm100 = 0.0;
float pm25_max  = 0.0;
float pm100_max = 0.0;
float pm25_avg  = 0.0;
float pm100_avg = 0.0;
double pm25_sum = 0.0;
double pm100_sum = 0.0;

void setup()
{
  randomSeed(analogRead(0));
  
  // Initial UART1 For Smog Sensor
  Serial.begin(9600);
  while(!Serial){;}
  // Setup the LCD
  myGLCD.InitLCD();//初始化液晶
  myGLCD.InitLCD();//初始化两次有利于系统稳定
  myGLCD.setFont(SmallFont);//设置字体为SmallFont格式
  // Power Supply to DHT11 
  pinMode (DHT11VCC, OUTPUT);
  pinMode (DHT11GND, OUTPUT);
}

void loop()
{ 
  digitalWrite (DHT11VCC, HIGH);
  digitalWrite (DHT11GND, LOW);
  
  // Display the Starting Page
  myGLCD.setColor(255, 255, 255);//设置字体颜色
  myGLCD.setBackColor(255, 0, 0);//设置背景颜色
  myGLCD.clrScr(); //清屏
  
  myGLCD.setFont(BigFont); //设置大字体BigFont（16*16字符）
  myGLCD.print("Smog is", LEFT, 12, display_rot);    
  myGLCD.print("coming.", LEFT, 32, display_rot);  
  myGLCD.setBackColor(0,0,255);
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.print("Smog Mirror V1.0 ", LEFT, 65, display_rot);    
  myGLCD.print("Developed by", LEFT, 90, display_rot);  
  myGLCD.print("Wang River Jiang", LEFT, 105, display_rot);   
  delay (1000);
  myGLCD.clrScr(); //清屏


    
  // Config the Smog Working Every 1 s
  SmogSensorSetWorkingPeriod (0);
  
  Displaylabel();
  
  // Main Loop
  while(1) {
    // Smog Sensor Polling for measurement result.
    SmogSensorRead();
    
    // Temperature and Humidity Sensor Polling for measurement result.
    TempHumidSensorRead();
    
 //   ReadDS3231();
    /*
    if (received_pkg == SETTLE_TIME){
      SmogSensorSetWorkingPeriod (1);
      received_pkg = 31;
    }
    */  
/*   
    if ((lowByte(received_pkg) > SETTLE_TIME) && (pm25 < 150) && !smog_in_sleep){
      SmogSensorSetWorkingPeriod (1);
      smog_in_sleep = true;
    } else {
      SmogSensorSetWorkingPeriod (0);
      smog_in_sleep = false;    
    }
 
    // Update the display
    DisplayUpdate();
    
*/
/*
    if ((lowByte(received_pkg) > SETTLE_TIME) && (pm25 < 150) && Fminute){
      SmogSensorSetSleepMode (0);
      smog_in_sleep = true;
    }
 
    // Update the display
    DisplayUpdate();
    
    if (smog_in_sleep && Fminute){
      //Wakeup the Smog
      SmogSensorSetSleepMode (1);
      smog_in_sleep = false;
    }  
*/
    // Update the display
    DisplayUpdate();
  }
}

void SmogSensorSetWorkingPeriod (unsigned int period) {
   word chk_sum =0;
   // Update the Working Period
   somg_cmd_set_period_working [4] = period;
   // Generate the check sum
   for(int i=2;i<=16;i++){
      chk_sum = chk_sum + somg_cmd_set_period_working [i];
   } 
   somg_cmd_set_period_working [17]=lowByte(chk_sum);   
   
   for (int i=0; i<19; i++){
     Serial.write (somg_cmd_set_period_working [i]);
     delay(10);
   }
}

void SmogSensorSetSleepMode (unsigned int wakeup) {
   word chk_sum =0;
   // Update the Working Period
   somg_cmd_set_sleep_mode [4] = wakeup;
   // Generate the check sum
   for(int i=2;i<=16;i++){
      chk_sum = chk_sum + somg_cmd_set_sleep_mode [i];
   } 
   somg_cmd_set_sleep_mode [17]=lowByte(chk_sum);   
   
   for (int i=0; i<19; i++){
     Serial.write (somg_cmd_set_sleep_mode [i]);
     delay(10);
   }
}
void TempHumidSensorRead(){
  int chk =0;
  if ((received_pkg%5)==0){
    chk =DHT11.read(DHT11PIN);
    if (chk == DHTLIB_OK){
      humidity = DHT11.humidity;
      temperature = DHT11.temperature;
      temp_humid_updisp=true;
    }
  }
}

void SmogSensorRead(){
    while (Serial.available() > 0) {
      temp_sign=Serial.read();
      switch (smog_state){
        
      case 0:  
        if (temp_sign == start_sign) {
          smog_state++;
          check_sum = 0;
        }
        break;
        
      case 1:   
        if (temp_sign == id_sign) {
          smog_state++;
        }
        break;
        
      case 2:
        pm25_l= temp_sign;
        smog_state++;
        check_sum = check_sum + temp_sign;        
        break;
        
      case 3:
        pm25_h= temp_sign;
        smog_state++;  
        check_sum = check_sum + temp_sign;        
        break;
        
      case 4:
        pm100_l= temp_sign;
        smog_state++;        
        check_sum = check_sum + temp_sign;        
        break;
        
      case 5:
        pm100_h= temp_sign;
        smog_state++;        
        check_sum = check_sum + temp_sign;        
        break;  

      case 6:
        smog_state++;        
        check_sum = check_sum + temp_sign;        
        break;
        
      case 7:
        smog_state++;        
        check_sum = check_sum + temp_sign;        
        break;  
        
      case 8:
        if (lowByte(check_sum) != temp_sign){
          smog_state=0;  
          check_failed++;          
        } else {
          smog_state++;
        }
        break;     
      
      case 9:  
        if (temp_sign == stop_sign) {
          smog_state++;
        }
        break;  
        
      default:
        smog_state = 0;  
     }
     delay (10);
    }
    
    if (smog_state == 10) {
      smog_state = 0;
      pm25 = (pm25_h*256.0 + pm25_l)/10;
      pm100  = (pm100_h*256.0 + pm100_l)/10;
      received_pkg++;
      
      if (received_pkg > 3600){
        pm25_avg=pm25_avg*0.9997+pm25*0.0003;
        pm100_avg=pm100_avg*0.9997+pm100*0.0003;
      } else {
        pm25_sum=pm25_sum+pm25;
        pm100_sum=pm100_sum+pm100;
        pm25_avg=pm25_sum/received_pkg;
        pm100_avg=pm100_sum/received_pkg;
      }
      smog_updisp=true;
      if (pm25>pm25_max){
        pm25_max=pm25;
      }
      if (pm100 > pm100_max){
        pm100_max=pm100;
      }
    }

}

void Displaylabel(){
    myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
    myGLCD.setBackColor(0,0,205);
    myGLCD.print("  PM2.5 ", LEFT, 22, display_rot);   
    myGLCD.setBackColor(139,69,19);
    myGLCD.print("   PM10 ", RIGHT, 22, display_rot);  
    
    myGLCD.setBackColor(130,57,53);
    myGLCD.print("   AVG  ", LEFT, 50, display_rot);    
    myGLCD.setBackColor(0,102,102);
    myGLCD.print("   AVG  ", RIGHT, 50, display_rot); 
        
    myGLCD.setBackColor(0,0,205);
    myGLCD.print("   MAX  ", LEFT, 78, display_rot);   
    myGLCD.setBackColor(139,69,19);
    myGLCD.print("   MAX  ", RIGHT, 78, display_rot);   
    myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
}

void DisplayPM (){
  unsigned int index;
  if(smog_updisp){

    myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
    
    // PM2.5 Display
    if (lowByte(received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm25/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[1], smog_front_g[1], smog_front_b[1]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm25, LEFT, 34, 4, ' ');  
    
      // PM10 Display
    if (lowByte(received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm100/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm100, RIGHT, 34, 4, ' ');
    
    // PM2.5 Average Display   
    if (lowByte(received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm25_avg/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm25_avg, LEFT, 62,  4, ' ');
    
    // PM10 Average Display
    if (lowByte(received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm100_avg/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm100_avg, RIGHT, 62, 4, ' ');

    // PM2.5 MAX Display
    if (lowByte(received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm25_max/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm25_max, LEFT, 90, 4, ' ');
    
    // PM10 MAX Display
    if (lowByte(received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm100_max/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[1], smog_front_g[1], smog_front_b[1]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm100_max, RIGHT, 90, 4, ' ');    
  }  

}


void DisplayUpdate(){
  DisplayPM();
}

