#include <UTFT.h>
#include <dht11.h>
#include <DS3231.h>
#include <Wire.h>


dht11 DHT11;
DS3231 Clock;
bool Century=false;
bool h12=false;
bool PM=false;
unsigned int hour =21;
unsigned int minute =36;
unsigned int second = 00;
unsigned int year =17;
unsigned int month =01;
unsigned int date = 13;
unsigned int DoW =5;
unsigned int Ahour =12;
unsigned int Aminute =30;
unsigned int Asecond = 59;
unsigned int Ayear =17;
unsigned int Amonth =01;
unsigned int Adate = 29;
unsigned int ADoW =5;

boolean Fhour =true;
boolean Fminute =true;
boolean Fsecond = true;
boolean Fyear =true;
boolean Fmonth =true;
boolean Fdate = true;
boolean FDoW =true;

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
unsigned int smog_start_sign = 0xAA;
unsigned int smog_stop_sign =0xAB;
unsigned int smog_id_sign = 0xC0;
unsigned int smog_temp_sign =0x0; 

unsigned int tvoc_start_sign = 0xFF;
unsigned int tvoc_id_sign = 0x01;
unsigned int tvoc_temp_sign =0x0; 

// Original Read Out of PM data
unsigned int pm25_h = 0;
unsigned int pm25_l = 0;
unsigned int pm100_h = 0;
unsigned int pm100_l = 0;
unsigned int tvoc_h = 0;
unsigned int tvoc_l = 0;

// State bit of FSM for Smog
unsigned int smog_state = 0; 
unsigned int tvoc_state = 0; 

// Color Graded Values
unsigned int smog_front_r[10] = {255, 255, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int smog_front_g[10] = {255, 250, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int smog_front_b[10] = {255, 250, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int smog_back_r[10] = {0,   204, 255, 255, 244, 175, 90, 51,  32, 13};
unsigned int smog_back_g[10] = {102, 204, 128, 0,   13,  18,  13, 0,   32, 12};
unsigned int smog_back_b[10] = {0,   0,   0,   0,   100, 88,  67, 102, 32, 12};


word smog_check_sum = 0;
word smog_received_pkg = 0;
word smog_check_failed = 0;
word tvoc_check_sum = 0;
word tvoc_received_pkg = 0;
word tvoc_check_failed = 0;

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
float pm25_1h_avg  = 0.0;
float pm100_1h_avg = 0.0;
double pm25_1h_sum = 0.0;
double pm100_1h_sum = 0.0;

void setup()
{
  randomSeed(analogRead(0));
  // Setup the IIC IF
  Wire.begin();
  // Initially Congfigure the Time and Date
  /*
  Clock.setSecond(second);//Set the second 
  Clock.setMinute(minute);//Set the minute 
  Clock.setHour(hour);  //Set the hour 
  Clock.setDoW(DoW);    //Set the day of the week
  Clock.setDate(date);  //Set the date of the month
  Clock.setMonth(month);  //Set the month of the year
  Clock.setYear(year);  //Set the year (Last two digits of the year)  
  */
  
  // Initial UART1 For Smog Sensor
  Serial1.begin(9600);
  Serial2.begin(9600);

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
  myGLCD.print("Smog Mirror V0.8 ", LEFT, 65, display_rot);    
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
    
    ReadDS3231();
    
    TvocSensorRead();
    /*
    if (smog_received_pkg == SETTLE_TIME){
      SmogSensorSetWorkingPeriod (1);
      smog_received_pkg = 31;
    }
    */  
/*   
    if ((lowByte(smog_received_pkg) > SETTLE_TIME) && (pm25 < 150) && !smog_in_sleep){
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
    if ((lowByte(smog_received_pkg) > SETTLE_TIME) && (pm25 < 150) && Fminute){
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

void ReadDS3231()
{
  int Asecond,Aminute,Ahour,Adate,Amonth,Ayear,Atemperature,ADoW; 
  Asecond=Clock.getSecond();
  Aminute=Clock.getMinute();
  Ahour=Clock.getHour(h12, PM);
  Adate=Clock.getDate();
  ADoW=Clock.getDoW();
  Amonth=Clock.getMonth(Century);
  Ayear=Clock.getYear();
  
  if (second!=Asecond){
    second=Asecond;
    Fsecond = true;
  }
  if (minute!=Aminute){
    minute=Aminute;
    Fminute=true;
  }
  if(hour!=Ahour){
    hour=Ahour;
    Fhour=true;
  }
  if (DoW!=ADoW){
    DoW=ADoW;
    FDoW = true;
  }
  if(date!=Adate){
    date=Adate;
    Fdate=true;
  }
  if(month!=Amonth){
    month=Amonth;
    Fmonth=true;
  }
  if(year!=Ayear){
    year=Ayear;
    Fyear=true;
  }
  
//  temperature=Clock.getTemperature();

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
     Serial1.write (somg_cmd_set_period_working [i]);
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
     Serial1.write (somg_cmd_set_sleep_mode [i]);
     delay(10);
   }
}
void TempHumidSensorRead(){
  int chk =0;
  if ((smog_received_pkg%5)==0){
    chk =DHT11.read(DHT11PIN);
    if (chk == DHTLIB_OK){
      humidity = DHT11.humidity;
      temperature = DHT11.temperature;
      temp_humid_updisp=true;
    }
  }
}

void SmogSensorRead(){
    while (Serial1.available() > 0) {
      smog_temp_sign=Serial1.read();
      switch (smog_state){
        
      case 0:  
        if (smog_temp_sign == smog_start_sign) {
          smog_state++;
          smog_check_sum = 0;
        }
        break;
        
      case 1:   
        if (smog_temp_sign == smog_id_sign) {
          smog_state++;
        }
        break;
        
      case 2:
        pm25_l= smog_temp_sign;
        smog_state++;
        smog_check_sum = smog_check_sum + smog_temp_sign;        
        break;
        
      case 3:
        pm25_h= smog_temp_sign;
        smog_state++;  
        smog_check_sum = smog_check_sum + smog_temp_sign;        
        break;
        
      case 4:
        pm100_l= smog_temp_sign;
        smog_state++;        
        smog_check_sum = smog_check_sum + smog_temp_sign;        
        break;
        
      case 5:
        pm100_h= smog_temp_sign;
        smog_state++;        
        smog_check_sum = smog_check_sum + smog_temp_sign;        
        break;  

      case 6:
        smog_state++;        
        smog_check_sum = smog_check_sum + smog_temp_sign;        
        break;
        
      case 7:
        smog_state++;        
        smog_check_sum = smog_check_sum + smog_temp_sign;        
        break;  
        
      case 8:
        if (lowByte(smog_check_sum) != smog_temp_sign){
          smog_state=0;  
          smog_check_failed++;          
        } else {
          smog_state++;
        }
        break;     
      
      case 9:  
        if (smog_temp_sign == smog_stop_sign) {
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
      smog_received_pkg++;
      
      if (smog_received_pkg > 3600){
        pm25_1h_avg=pm25_1h_avg*0.9997+pm25*0.0003;
        pm100_1h_avg=pm100_1h_avg*0.9997+pm100*0.0003;
      } else {
        pm25_1h_sum=pm25_1h_sum+pm25;
        pm100_1h_sum=pm100_1h_sum+pm100;
        pm25_1h_avg=pm25_1h_sum/smog_received_pkg;
        pm100_1h_avg=pm100_1h_sum/smog_received_pkg;
      }
      smog_updisp=true;
      if (pm25>pm25_max){
        pm25_max=pm25;
        tvoc_updisp=true;  // Temperary put it here
      }
      if (pm100 > pm100_max){
        pm100_max=pm100;
        tvoc_updisp=true;  // Temperary put it here
      }
    }

}

void TvocSensorRead(){
    while (Serial2.available() > 0) {
      tvoc_temp_sign=Serial2.read();
      switch (tvoc_state){
        
      case 0:  
        if (tvoc_temp_sign == tvoc_start_sign) {
          tvoc_state++;
          tvoc_check_sum = 0 + tvoc_temp_sign;        
        }
        break;
 
      case 1:  
        if (tvoc_temp_sign == tvoc_start_sign) {
          tvoc_state++;
          tvoc_check_sum = tvoc_check_sum + tvoc_temp_sign;        
        }
        break;       
        
      case 2:   
          tvoc_state++;
          tvoc_check_sum = tvoc_check_sum + tvoc_temp_sign;        
        break;
        
      case 3:   
          tvoc_state++;
          tvoc_check_sum = tvoc_check_sum + tvoc_temp_sign;        
        break;        
        
      case 4:
        tvoc_h= tvoc_temp_sign;
        tvoc_state++;
        tvoc_check_sum = tvoc_check_sum + tvoc_temp_sign;        
        break;
        
      case 5:
        tvoc_l= tvoc_temp_sign;
        tvoc_state++;
        tvoc_check_sum = tvoc_check_sum + tvoc_temp_sign;           
        break;
        

      case 6:
        tvoc_state++;        
        tvoc_check_sum = tvoc_check_sum + tvoc_temp_sign;        
        break;
        
      case 7:
        tvoc_state++;        
        tvoc_check_sum = tvoc_check_sum + tvoc_temp_sign;        
        break;  
        
      case 8:
 //       if (lowByte(tvoc_check_sum) != tvoc_temp_sign){
 //         tvoc_state=0;  
 //         tvoc_check_failed++;          
 //       } else {
          tvoc_state++;
 //       }
        break;     
      
      default:
        tvoc_state = 0;  
     }
     delay (10);
    }
    
    if (tvoc_state == 9) {
      tvoc_state = 0;
      tvoc = (tvoc_h*256.0 + tvoc_l)/100;
      tvoc_received_pkg++;
      
      tvoc_updisp=true;
    }

}

void DisplayTimeDate(){
  unsigned int index;
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.setColor(255, 255, 255);//设置字体颜色
  myGLCD.setBackColor(0, 0, 0);//设置背景颜色  

  if(FDoW){
    switch (DoW) {
    case 1: 
      myGLCD.print("Monday", LEFT, 0, display_rot);   
      break;
    
    case 2: 
      myGLCD.print("Tuesday", LEFT, 0, display_rot);   
      break;
    
    case 3: 
      myGLCD.print("Wensday", LEFT, 0, display_rot);   
      break;
      
    case 4: 
      myGLCD.print("Thursday", LEFT, 0, display_rot);   
      break;
      
    case 5: 
      myGLCD.print("Friday", LEFT, 0, display_rot);   
      break;
 
    case 6: 
      myGLCD.print("Saturday", LEFT, 0, display_rot);   
      break;
 
    default: 
      myGLCD.print("Sunday", LEFT, 0, display_rot);   
    }
    FDoW = false;
  }  
  // Time Display
  if (Fhour){
    myGLCD.printNumI(hour, 88, 0,  2, ' ');
    Fhour=false;
  }
  if (Fminute){
    if (minute <10){
      myGLCD.print(":0", 104, 0, display_rot);
      myGLCD.printNumI(minute, 120, 0,  1, ' ');
    }else {
      myGLCD.print(":", 104, 0, display_rot);
      myGLCD.printNumI(minute, 112, 0,  2, ' ');
    }
    Fminute=false;
  }
  
  // Date Display
  if (Fdate) {    
    myGLCD.printNumI(date, 40, 116,  2, ' ');
    Fdate=false;
  }
  if (Fmonth){
    myGLCD.print("-", 56, 116, display_rot);
    index = 64;
    switch (month) {
    case 1: 
      myGLCD.print("Jan", index, 116, display_rot);   
      break;
    
    case 2: 
      myGLCD.print("Feb", index, 116, display_rot);   
      break;
    
    case 3: 
      myGLCD.print("Mar", index, 116, display_rot);   
      break;
      
    case 4: 
      myGLCD.print("Apr", index, 116, display_rot);   
      break;
      
    case 5: 
      myGLCD.print("May", index, 116, display_rot);   
      break;
 
    case 6: 
      myGLCD.print("Jue", index, 116, display_rot);   
      break;
 
    case 7: 
      myGLCD.print("Jul", index, 116, display_rot);   
      break;
    
    case 8: 
      myGLCD.print("Aug", index, 116, display_rot);   
      break;
    
    case 9: 
      myGLCD.print("Sep", index, 116, display_rot);   
      break;
      
    case 10: 
      myGLCD.print("Oct", index, 116, display_rot);   
      break;
      
    case 11: 
      myGLCD.print("Nov", index, 116, display_rot);   
      break;
 
    case 12: 
      myGLCD.print("Dec", index, 116, display_rot);   
      break;  
      
    default:
     ;
    }
    Fmonth = false;
  }
  if (Fyear){
    myGLCD.print("-20", 88, 116, display_rot);
    myGLCD.printNumI(year, 112, 116,  2, ' ');
    Fyear = false;
  }

  
}

void Displaylabel(){
    myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
    myGLCD.setBackColor(0,0,205);
    myGLCD.print("Temp(oC)", LEFT, 22, display_rot);   
    myGLCD.setBackColor(139,69,19);
    myGLCD.print("Humid(%)", RIGHT, 22, display_rot);  
    
    myGLCD.setBackColor(130,57,53);
    myGLCD.print("  PM2.5 ", LEFT, 50, display_rot);    
    myGLCD.setBackColor(0,102,102);
    myGLCD.print("  PM10  ", RIGHT, 50, display_rot); 
        
    myGLCD.setBackColor(0,0,205);
    myGLCD.print("PM2.5MAX", LEFT, 78, display_rot);   
    myGLCD.setBackColor(139,69,19);
    myGLCD.print(" PM10MAX", RIGHT, 78, display_rot);   
    myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
}

  

void DisplayTempHumid(){
  unsigned int index;

  if (temp_humid_updisp && !smog_in_sleep){
    myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
    // Temperature Display
    if (lowByte(smog_received_pkg) > SETTLE_TIME){
      index=(unsigned int) ((temperature+50)/10);
      index = index >9? 9 : index;  
      temp_humid_updisp=false;
    } else {
      index = random(10)%10;
    } 
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((int)temperature, LEFT, 34,  4, ' ');
    
    // Humidity Display
    if (lowByte(smog_received_pkg) > SETTLE_TIME){
      index=(unsigned int) (humidity/10);
      index = index >9? 9 : index;  
      temp_humid_updisp=false;
    } else {
      index = random(10)%10;
    } 
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((int)humidity, RIGHT, 34, 4, ' ');
  }

}
void DisplayPM (){
  unsigned int index;
  if(smog_updisp){

    myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
    
    // PM2.5 Display
    if (lowByte(smog_received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm25/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[1], smog_front_g[1], smog_front_b[1]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm25, LEFT, 62, 4, ' ');  
    
      // PM10 Display
    if (lowByte(smog_received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm100/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm100, RIGHT, 62, 4, ' ');
  }
}

void DisplayVoc(){
  unsigned int index;
  if(tvoc_updisp){
    myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)  
    // CH2O Display
    if (lowByte(smog_received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm100/50);
      index = index >9? 9 : index;  
      tvoc_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm100_1h_avg, RIGHT, 90, 4, ' ');
    
    // TVOC Display
    if (lowByte(smog_received_pkg) > SETTLE_TIME){
      index=(unsigned int) (pm25/50);
      index = index >9? 9 : index;  
      tvoc_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[1], smog_front_g[1], smog_front_b[1]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumF(tvoc, 2, LEFT, 90, '.', 4,' ');
  }
}
void DisplayUpdate(){
  DisplayTimeDate();
  DisplayTempHumid();
  DisplayPM();
  DisplayVoc();
}

