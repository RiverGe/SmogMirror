
#include <SoftwareSerial.h>
#include <UTFT.h>

SoftwareSerial mySerial(2, 3); // RX, TX

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
unsigned int stop_sign =0xBB;
unsigned int id_sign = 0x02;
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
unsigned int smog_back_r[10] = {0,   0x8e, 255, 255, 244, 175, 90, 51,  32, 13};
unsigned int smog_back_g[10] = {102, 0x8e, 128, 0,   13,  18,  13, 0,   32, 12};
unsigned int smog_back_b[10] = {0,   0x38,   0,   0,   100, 88,  67, 102, 32, 12};


word check_sum = 0;
word received_pkg = 0;
word check_failed = 0;

#define SETTLE_TIME 15


// Control bit of if a Display Update should be Applyied
boolean smog_updisp = true;
boolean smog_max_updisp = true;
boolean smog_min_updisp = true;

uint8_t smog_set_poweron[9] ={0xAA, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x66, 0xBB};
uint8_t smog_set_read[9]    ={0xAA, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x67, 0xBB};
uint8_t smog_set_poweroff[9]={0xAA, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x68, 0xBB};

float pm25  = 0.0;
float pm100 = 0.0;
float pm25_max  = 0.0;
float pm100_max = 0.0;
float pm25_min  = 9999;
float pm100_min = 9999;
float pm25_avg  = 0.0;
float pm100_avg = 0.0;
double pm25_sum = 0.0;
double pm100_sum = 0.0;

void setup()
{
  randomSeed(analogRead(0));
  
  // Initial UART1 For Smog Sensor
//  Serial.begin(9600);
//  while(!Serial){;}
  mySerial.begin(9600);
  // initialize digital pin 13 as an output.
  pinMode(13, OUTPUT);
  // Setup the LCD
  myGLCD.InitLCD();//初始化液晶
  myGLCD.InitLCD();//初始化两次有利于系统稳定
  myGLCD.setFont(SmallFont);//设置字体为SmallFont格式
}

void loop()
{ 
  SmogSetPowron();
  
  // Display the Starting Page
  myGLCD.setColor(255, 255, 255);//设置字体颜色
  myGLCD.setBackColor(255, 0, 0);//设置背景颜色
  myGLCD.clrScr(); //清屏
  
  myGLCD.setFont(BigFont); //设置大字体BigFont（16*16字符）
  myGLCD.print("Smog is", LEFT, 12, display_rot);    
  myGLCD.print("coming.", LEFT, 32, display_rot);  
  myGLCD.setBackColor(0,0,255);
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.print("Smog Mirror V1.2 ", LEFT, 65, display_rot);    
  myGLCD.print("Developed by", LEFT, 90, display_rot);  
  myGLCD.print("Wang River Jiang", LEFT, 105, display_rot);   
  delay (1000);
  myGLCD.clrScr(); //清屏
  
  Displaylabel();
  
  // Main Loop
  while(1) {
    // Smog Sensor Polling for measurement result.
    SmogSensorRead();
    
    // Update the display
    DisplayUpdate();
  }
}

void SmogSetPowron () {
   mySerial.write(smog_set_poweron,9);
}
void SmogSetPowroff () {
   mySerial.write(smog_set_poweroff,9);
}

void SmogSetRead () {
   mySerial.write(smog_set_read,9);
}
void SmogSensorRead(){
  delay(100);
  SmogSetRead();
  delay(100);

  while (mySerial.available() > 0) {
      temp_sign=mySerial.read();
      switch (smog_state){
        
      case 0:  
        if (temp_sign == start_sign) {
          smog_state++;
          check_sum = 0+temp_sign;
        }
        break;
        
      case 1:   
        if (temp_sign == id_sign) {
          smog_state++;
          check_sum = check_sum + temp_sign;        
        }
        break;
        
      case 2:
        pm100_h=temp_sign;
        smog_state++;
        check_sum = check_sum + temp_sign;        
        break;
        
      case 3:
        pm100_l= temp_sign;
        smog_state++;  
        check_sum = check_sum + temp_sign;        
        break;
        
      case 4:
        pm25_h= temp_sign;
        smog_state++;        
        check_sum = check_sum + temp_sign;        
        break;
        
      case 5:
        pm25_l= temp_sign;
        smog_state++;        
        check_sum = check_sum + temp_sign+0xBB;        
        break;  

      case 6:
        smog_state++;        
        break;
        
        
      case 7:
        if (lowByte(check_sum) != temp_sign){
          smog_state=0;  
          check_failed++;          
        } else {
          smog_state++;
        }
        break;     
      
      case 8:  
        if (temp_sign == stop_sign) {
          smog_state++;
        }
        break;  
        
      default:
        smog_state = 0;  
     }
     delay (10);
    }
    
    if (smog_state == 9) {
      smog_state = 0;
      pm25 = pm25_h*256.0 + pm25_l;
      pm100  = pm100_h*256.0 + pm100_l;
      received_pkg++;
      digitalWrite(13, lowByte(received_pkg)& 0x01);  
      
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
        smog_max_updisp=true;
      }
      if (pm100 > pm100_max){
        pm100_max=pm100;
        smog_max_updisp=true;
      }
      if (pm25<pm25_min){
        pm25_min=pm25;
        smog_min_updisp=true;
      }
      if (pm100 < pm100_min){
        pm100_min=pm100;
        smog_min_updisp=true;
      }      
    }

}

void Displaylabel(){
    myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
    myGLCD.setBackColor(0,0,0);
    myGLCD.print("SmogMirror:ug/m3", LEFT, 0, display_rot);    
    myGLCD.setBackColor(0,0,205);
    myGLCD.print("  PM2.5 ", LEFT, 16, display_rot);   
    myGLCD.setBackColor(139,69,19);
    myGLCD.print("  PM10  ", RIGHT, 16, display_rot);  
    
    myGLCD.setBackColor(0,102,102);
    myGLCD.print(" 1h AVG ", LEFT, 44, display_rot);    
    myGLCD.setBackColor(130,57,53);
    myGLCD.print(" 1h AVG ", RIGHT, 44, display_rot); 
        
    myGLCD.setBackColor(20,68,106);
    myGLCD.print("   MAX  ", LEFT, 72, display_rot);   
    myGLCD.setBackColor(117,36,35);
    myGLCD.print("   MAX  ", RIGHT, 72, display_rot);   
    
    myGLCD.setBackColor(3,38,58);
    myGLCD.print("   MIN  ", LEFT, 100, display_rot);   
    myGLCD.setBackColor(71,31,31);
    myGLCD.print("   MIN  ", RIGHT, 100, display_rot);   
}

void DisplayPM (){
  unsigned int index;
  if(smog_updisp){

    myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
    
    // PM2.5 Display
    if (lowByte(received_pkg) >= SETTLE_TIME){
      index=(unsigned int) (pm25/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm25, LEFT, 28, 4, ' ');  
    
      // PM10 Display
    if (lowByte(received_pkg) >= SETTLE_TIME){
      index=(unsigned int) (pm100/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm100, RIGHT, 28, 4, ' ');
    
    // PM2.5 Average Display   
    if (lowByte(received_pkg) >= SETTLE_TIME){
      index=(unsigned int) (pm25_avg/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm25_avg, LEFT, 56,  4, ' ');
    
    // PM10 Average Display
    if (lowByte(received_pkg) >= SETTLE_TIME){
      index=(unsigned int) (pm100_avg/50);
      index = index >9? 9 : index;  
      smog_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm100_avg, RIGHT, 56, 4, ' ');

  }
  if (smog_max_updisp || (lowByte(received_pkg) <= SETTLE_TIME)){
    // PM2.5 MAX Display
    if (lowByte(received_pkg) >= SETTLE_TIME){
      index=(unsigned int) (pm25_max/50);
      index = index >9? 9 : index;  
      smog_max_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm25_max, LEFT, 84, 4, ' ');
    
    // PM10 MAX Display
    if (lowByte(received_pkg) >= SETTLE_TIME){
      index=(unsigned int) (pm100_max/50);
      index = index >9? 9 : index;  
      smog_max_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm100_max, RIGHT, 84, 4, ' ');    
  }
  
  if (smog_min_updisp ||(lowByte(received_pkg) <= SETTLE_TIME)){  
    // PM2.5 MIN Display
    if (lowByte(received_pkg) >= SETTLE_TIME){
      index=(unsigned int) (pm25_min/50);
      index = index >9? 9 : index;  
      smog_min_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm25_min, LEFT, 112, 4, ' ');
    
    // PM10 MAX Display
    if (lowByte(received_pkg) >= SETTLE_TIME){
      index=(unsigned int) (pm100_min/50);
      index = index >9? 9 : index;  
      smog_min_updisp=false;
    } else {
      index = random(10)%10;
    }
    myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm100_min, RIGHT, 112, 4, ' ');    
  }  
}


void DisplayUpdate(){
  DisplayPM();
}

