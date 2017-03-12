#include <UTFT.h>


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

unsigned int  smog_sensor_r[10];
unsigned int start_sign = 0xAA;
unsigned int stop_sign =0xAB;
unsigned int id_sign = 0xC0;
unsigned int temp_sign =0x0; 
unsigned int pm25_h = 0;
unsigned int pm25_l = 0;
unsigned int pm100_h = 0;
unsigned int pm100_l = 0;
unsigned int smog_state = 0; 
unsigned int smog_front_r = 255;
unsigned int smog_front_g = 255;
unsigned int smog_front_b = 255;
unsigned int smog_back_r = 0;
unsigned int smog_back_g = 0;
unsigned int smog_back_b = 255;

float pm25 = 2.5;
float pm100 =10.0;
void setup()
{
  randomSeed(analogRead(0));
  Serial.begin(9600);
  while(!Serial){;}
// Setup the LCD
  myGLCD.InitLCD();//初始化液晶
  myGLCD.InitLCD();//初始化两次有利于系统稳定
  myGLCD.setFont(SmallFont);//设置字体为SmallFont格式
}
void loop()
{ 
//  myGLCD.fillScr(255,0,0);//填充RED
//  delay (500);
//  myGLCD.fillScr(0,255,0);//填充GREEN
//  delay (500);
//  myGLCD.fillScr(0,0,255);//填充BLUE
//  delay (500);
  Serial.print("Read: "); 
  //En_8X12 Test
  myGLCD.setColor(255, 255, 255);//设置字体颜色
  myGLCD.setBackColor(255, 0, 0);//设置背景颜色
  myGLCD.clrScr(); //清屏
  
  myGLCD.setFont(BigFont); //设置大字体BigFont（16*16字符）
  myGLCD.print("Smog is", LEFT, 12, display_rot);    
  myGLCD.print("coming.", LEFT, 32, display_rot);  
  myGLCD.setBackColor(0,0,255);
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.print("Smog_Mirror V0.1 ", LEFT, 65, display_rot);    
  myGLCD.print("Developed by", LEFT, 90, display_rot);  
  myGLCD.print("Wang River Jiang", LEFT, 105, display_rot);   
//  myGLCD.print("QQ:37828265         ", LEFT, 110); 
  delay (2000);
 
  myGLCD.clrScr(); //清屏

 while(1) {
   SensorRead();
   
   DisplayUpdate();
   
   delay(100);
 }
}

void SensorRead(){
  
    while (Serial.available() > 0) {
      temp_sign=Serial.read();
      switch (smog_state){
        
      case 0:  
        if (temp_sign == start_sign) {
          smog_state++;
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
        break;
        
      case 3:
        pm25_h= temp_sign;
        smog_state++;        
        break;
        
      case 4:
        pm100_l= temp_sign;
        smog_state++;        
        break;
        
      case 5:
        pm100_h= temp_sign;
        smog_state++;        
        break;  

      case 6:
        smog_state++;        
        break;
        
      case 7:
        smog_state++;        
        break;  
        
      case 8:
        smog_state++;        
        break;     
      
      case 9:  
        if (temp_sign == stop_sign) {
          smog_state++;
        }
        break;  
     }
     delay (10);
    }
    
    if (smog_state == 10) {
      smog_state = 0;
      pm25 = (pm25_h*256.0 + pm25_l)/10;
      pm100  = (pm100_h*256.0 + pm100_l)/10;
    }

}

void DisplayUpdate(){
  
  if (pm25 <= 50.0) {
    smog_back_r = 34;
    smog_back_g = 139;
    smog_back_b = 34;
  } else if (pm25 <= 100.0) {
    smog_back_r = 56;
    smog_back_g = 94;
    smog_back_b = 15;  
  } else if (pm25 <= 150.0) {
    smog_back_r = 210;
    smog_back_g = 105;
    smog_back_b = 30;    
  } else if (pm25 <= 200.0){
    smog_back_r = 160;
    smog_back_g = 32;
    smog_back_b = 240;   
  } else if (pm25 <= 300.0) {
    smog_back_r = 255;
    smog_back_g = 0;
    smog_back_b = 0; 
  } else if (pm25 <= 400.0) {
    smog_back_r = 176;
    smog_back_g = 23;
    smog_back_b = 31;   
  } else {
    smog_back_r = 41;
    smog_back_g = 36;
    smog_back_b = 33;     
  }
  myGLCD.setColor(smog_front_r, smog_front_g, smog_front_b);//设置字体颜色
  myGLCD.setBackColor(smog_back_r,smog_back_g, smog_back_b);//设置背景颜色
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.print("PM2.5(ug/m3):", LEFT, 12, display_rot);    
  myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
  myGLCD.printNumF(pm25, 1, RIGHT, 30, '.', 10, ' ');
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.print("PM10(ug/m3):", LEFT, 62, display_rot);    
  myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
  myGLCD.printNumF(pm100, 1, RIGHT, 80, '.', 10, ' ');
}

