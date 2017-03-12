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

unsigned int start_sign = 0xAA;
unsigned int stop_sign =0xAB;
unsigned int id_sign = 0xC0;
unsigned int temp_sign =0x0; 
unsigned int pm25_h = 0;
unsigned int pm25_l = 0;
unsigned int pm100_h = 0;
unsigned int pm100_l = 0;
unsigned int smog_state = 0; 
unsigned int smog_front_r[10] = {255, 255, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int smog_front_g[10] = {255, 250, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int smog_front_b[10] = {255, 250, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int smog_back_r[10] = {0,   204, 255, 255, 244, 175, 90, 51,  32, 13};
unsigned int smog_back_g[10] = {102, 204, 128, 0,   13,  18,  13, 0,   32, 12};
unsigned int smog_back_b[10] = {0,   0,   0,   0,   100, 88,  67, 102, 32, 12};
word check_sum = 0;

unsigned int somg_cmd_set_period_working [19]= {0xAA, 0xB4, 0x08, 0x01, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xFF, 0xFF, 0x8, 0xAB};
unsigned int somg_cmd_set_sleep_mode [19]= {0xAA, 0xB4, 0x06, 0x01, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xFF, 0xFF, 0x8, 0xAB};

float pm25  = 0.0;
float pm100 = 0.0;
void setup()
{
  randomSeed(analogRead(0));
  // Initial UART1 For Smog Sensor
  Serial1.begin(9600);
  while(!Serial){;}
// Setup the LCD
  myGLCD.InitLCD();//初始化液晶
  myGLCD.InitLCD();//初始化两次有利于系统稳定
  myGLCD.setFont(SmallFont);//设置字体为SmallFont格式
}
void loop()
{ 

  // Display the Starting Page
  myGLCD.setColor(255, 255, 255);//设置字体颜色
  myGLCD.setBackColor(255, 0, 0);//设置背景颜色
  myGLCD.clrScr(); //清屏
  
  myGLCD.setFont(BigFont); //设置大字体BigFont（16*16字符）
  myGLCD.print("Smog is", LEFT, 12, display_rot);    
  myGLCD.print("coming.", LEFT, 32, display_rot);  
  myGLCD.setBackColor(0,0,255);
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.print("Smog Mirror V0.2 ", LEFT, 65, display_rot);    
  myGLCD.print("Developed by", LEFT, 90, display_rot);  
  myGLCD.print("Wang River Jiang", LEFT, 105, display_rot);   
  delay (2000);
  myGLCD.clrScr(); //清屏
  
  // Config the Smog Working Every 1 s
  SmogSensorSetWorkingPeriod (0);
  
  // Main Loop
  while(1) {
    // Smog Sensor Polling for measurement result.
    SmogSensorRead();
    
    // Update the display
    DisplayUpdate();
    /*
    SmogSensorSetSleepMode (0);
    // Wait for a while
    delay(30000);
    SmogSensorSetSleepMode (1);
    delay(30000);
    */
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

void SmogSensorRead(){
    while (Serial1.available() > 0) {
      temp_sign=Serial1.read();
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
    }

}
 
/*
void DisplayUpdate(){
  unsigned int index=0;
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.setBackColor(0,0,255);
  myGLCD.print(" PM2.5(ug/m3):    ", LEFT, 12, display_rot);    
  myGLCD.print(" PM10(ug/m3):     ", LEFT, 45, display_rot);   
  myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
  index = random(300)%9;
  index = index >9? 0 : index;  
  myGLCD.setColor(smog_front_r[1], smog_front_g[1], smog_front_b[1]);//设置字体颜色
  myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
  myGLCD.printNumF(pm25, 1, RIGHT, 24, '.', 10, ' ');
  
  index = random(300)%9;
  index = index >9? 0 : index; 
  myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
  myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
  myGLCD.printNumF(pm100, 1, RIGHT, 57, '.', 10, ' ');
  index++;
}*/

 
void DisplayUpdate(){
  unsigned int index=0;
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.setBackColor(0,0,255);
  myGLCD.print(" PM2.5(ug/m3):    ", LEFT, 2, display_rot);    
  myGLCD.print(" PM10(ug/m3):     ", LEFT, 32, display_rot);   
  myGLCD.print(" Temperature(C):     ", LEFT, 62, display_rot);   
  myGLCD.print(" Humidity(%):     ", LEFT, 92, display_rot);   
  myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
  
  index=(unsigned int) (pm25/50);
  index = index >9? 9 : index;  
  myGLCD.setColor(smog_front_r[1], smog_front_g[1], smog_front_b[1]);//设置字体颜色
  myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
  myGLCD.printNumF(pm25, 1, RIGHT, 14, '.', 10, ' ');

  index=(unsigned int) (pm100/50);
  index = index >9? 9 : index;
  myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
  myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
  myGLCD.printNumF(pm100, 1, RIGHT, 44, '.', 10, ' ');
  
  index = random(300)%9;
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
  myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色 
  myGLCD.print("                        ", LEFT, 80, display_rot);
  
  index = random(300)%9;
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.setColor(smog_front_r[0], smog_front_g[0], smog_front_b[0]);//设置字体颜色
  myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色 
  myGLCD.print("                        ", LEFT, 104, display_rot);}


