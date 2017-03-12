#include <SoftwareSerial.h>
#include <UTFT.h>

#define PM_AT_CF1 1;

// Software UART Configure
SoftwareSerial mySerial(2, 3); // RX, TX
 
UTFT myGLCD(YYROBOT_TFT144,A2,A1,A5,A4,A3);  // Remember to change the model parameter to suit your display module!
//YYROBOT_TFT144  屏幕型号，不用修改
//LED----A0  UTFT库里面设定的，如果需要修改需要修改库文件
//SCL----A1
//SDA----A2
//RS-----A3
//RST----A4
//CS-----A5
extern uint8_t SmallFont[];//原始文件在库文件的DefaultFonts.c中
extern uint8_t BigFont[];//原始文件在库文件的DefaultFonts.c中

// PM Sensor Protocol
unsigned int start_sign1 = 0x42;
unsigned int start_sign2 = 0x4d;
unsigned int temp_sign =0x0; 

// State bit of FSM for Smog
unsigned int smog_state = 0; 

// Color Graded Values
unsigned int smog_back_r[10] = {0,   0x8e, 255, 255, 244, 175, 90, 51,  32, 13};
unsigned int smog_back_g[10] = {102, 0x8e, 128, 0,   13,  18,  13, 0,   32, 12};
unsigned int smog_back_b[10] = {0,   0x38,   0,   0,   100, 88,  67, 102, 32, 12};

unsigned long received_pkg = 0;

#define SETTLE_TIME 5


// Control bit of if a Display Update should be Applyied
boolean smog_updisp = true;
boolean smog_max_updisp = true;
boolean smog_min_updisp = true;

word pm10= 0;
word pm25  = 0.0;
word pm100 = 0.0;
word pm10_max  = 0.0;
word pm25_max  = 0.0;
word pm100_max = 0.0;
word pm10_min  = 9999;
word pm25_min  = 9999;
word pm100_min = 9999;
float pm10_avg  = 0.0;
float pm25_avg  = 0.0;
float pm100_avg = 0.0;
unsigned long pm10_sum = 0.0;
unsigned long pm25_sum = 0.0;
unsigned long pm100_sum = 0.0;

word pm3_count = 0;
word pm5_count = 0;
word pm10_count = 0;
word pm25_count = 0;
word pm50_count = 0;
word pm100_count = 0;

void setup()
{
  randomSeed(analogRead(0));
  
  // Initial Software UART For Smog Sensor
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
  
  // Display the Starting Page
  myGLCD.setColor(255, 255, 255);//设置字体颜色
  myGLCD.setBackColor(255, 0, 0);//设置背景颜色
  myGLCD.clrScr(); //清屏
  
  myGLCD.setFont(BigFont); //设置大字体BigFont（16*16字符）
  myGLCD.print("Smog is", LEFT, 12, 0);    
  myGLCD.print("coming.", LEFT, 32, 0);  
  myGLCD.setBackColor(0,0,255);
  myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
  myGLCD.print("Smog Mirror V1.2 ", LEFT, 65, 0);    
  myGLCD.print("Developed by", LEFT, 90, 0);  
  myGLCD.print("Wang River Jiang", LEFT, 105, 0);   
  delay (2000);
  myGLCD.clrScr(); //清屏
  
  
  // Main Loop
  while(1) {
    // Update the display
    DisplayUpdate();

    // Smog Sensor Polling for measurement result.
    SmogSensorRead();  
  }
}

void SmogSensorRead(){
  uint8_t read_pkg[40];

  while (mySerial.available() > 0) {
      temp_sign=mySerial.read();
      switch (smog_state){
        
      case 0:  
        if (temp_sign == start_sign1) {
          smog_state++;
        } else {
          smog_state=0;
        }
        break;
        
      case 1:   
        if (temp_sign == start_sign2) {
          smog_state++;
        } else {
          smog_state=0;
        }
        break;
        
      case 2:
        smog_state++;
        break;
        
      case 3:
        smog_state++;  
        break;
        
      case 4:
        read_pkg[0]=temp_sign;
        smog_state++;  
        for(int i=1;i<36;i++){
          if (mySerial.available() > 0){
            read_pkg[i]=mySerial.read();
          }
        }        
        break;
        
      default:
      ;
     }
     delay (10);
  }
    
    if (smog_state == 5) {
      smog_state = 0;
     #if defined (PM_AT_CF1)
        pm10 = read_pkg[0]*256.0 + read_pkg[1];
        pm25 = read_pkg[2]*256.0 + read_pkg[3];
        pm100  = read_pkg[4]*256.0 + read_pkg[5];
     #else
        pm10 = read_pkg[6]*256.0 + read_pkg[7];
        pm25 = read_pkg[8]*256.0 + read_pkg[9];
        pm100  = read_pkg[10]*256.0 + read_pkg[11];
     #endif 
      pm3_count = read_pkg[12]*256.0 + read_pkg[13];
      pm5_count = read_pkg[14]*256.0 + read_pkg[15];
      pm10_count = read_pkg[16]*256.0 + read_pkg[17];
      pm25_count = read_pkg[18]*256.0 + read_pkg[19];
      pm50_count = read_pkg[20]*256.0 + read_pkg[21];
      pm100_count = read_pkg[22]*256.0 + read_pkg[23];
      
      received_pkg++;
      // LED Blink for indicate the package receiving
      digitalWrite(13, lowByte(received_pkg)& 0x01);   
      
      if (received_pkg > 3600){
        pm10_avg=pm10_avg*0.9997+pm10*0.0003;
        pm25_avg=pm25_avg*0.9997+pm25*0.0003;
        pm100_avg=pm100_avg*0.9997+pm100*0.0003;
      } else {
        pm10_sum=pm10_sum+pm10;
        pm25_sum=pm25_sum+pm25;
        pm100_sum=pm100_sum+pm100;
        pm10_avg=pm10_sum/received_pkg;
        pm25_avg=pm25_sum/received_pkg;
        pm100_avg=pm100_sum/received_pkg;
      }
      smog_updisp=true;
      if (pm10>pm10_max){
        pm10_max=pm10;
        smog_max_updisp=true;
      }
      if (pm25>pm25_max){
        pm25_max=pm25;
        smog_max_updisp=true;
      }
      if (pm100 > pm100_max){
        pm100_max=pm100;
        smog_max_updisp=true;
      }
      if (pm10<pm10_min){
        pm10_min=pm10;
        smog_min_updisp=true;
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
    myGLCD.print("SmogMirror:ug/m3", LEFT, 0, 0);    
    myGLCD.setBackColor(0,0,205);
    myGLCD.print("  PM2.5 ", LEFT, 16, 0);   
    myGLCD.setBackColor(139,69,19);
    myGLCD.print("  PM10  ", RIGHT, 16, 0);  
    
    myGLCD.setBackColor(0,102,102);
    myGLCD.print(" 1h AVG ", LEFT, 44, 0);    
    myGLCD.setBackColor(130,57,53);
    myGLCD.print(" 1h AVG ", RIGHT, 44, 0); 
        
    myGLCD.setBackColor(20,68,106);
    myGLCD.print("   MAX  ", LEFT, 72, 0);   
    myGLCD.setBackColor(117,36,35);
    myGLCD.print("   MAX  ", RIGHT, 72, 0);   
    
    myGLCD.setBackColor(3,38,58);
    myGLCD.print("   MIN  ", LEFT, 100, 0);   
    myGLCD.setBackColor(71,31,31);
    myGLCD.print("   MIN  ", RIGHT, 100, 0);   
}
 
void Displaylabel_1(){
    myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
    myGLCD.setBackColor(0,0,0);
    myGLCD.print("SmogMirror:ug/m3", LEFT, 0, 0);    
    myGLCD.setBackColor(0,0,205);
    myGLCD.print("PM1.0", LEFT, 16, 0);   
    myGLCD.setBackColor(139,69,19);
    myGLCD.print("PM2.5", CENTER, 16, 0);   
    myGLCD.setBackColor(130,57,53);
    myGLCD.print(" PM10", RIGHT, 16, 0);  
    myGLCD.setBackColor(0,102,102);
    myGLCD.print("   1 hour AVG   ", CENTER, 44, 0);    
    myGLCD.setBackColor(20,68,106);
    myGLCD.print("     Maximum    ", CENTER, 72, 0);   
    myGLCD.setBackColor(3,38,58);
    myGLCD.print("     Minimum    ", CENTER, 100, 0);   
} 
void Displaylabel_2(){
    myGLCD.setFont(SmallFont);          //设置字体为SmallFont格式(8*12字符)
    myGLCD.setBackColor(0,0,0);
    myGLCD.print("SmogMirror:count", LEFT, 0, 0);    
    myGLCD.print(">0.3um", RIGHT, 16, 0);         
    myGLCD.print(">0.5um", RIGHT, 36, 0);         
    myGLCD.print(">1.0um", RIGHT, 56, 0);         
    myGLCD.print(">2.5um", RIGHT, 76, 0);         
    myGLCD.print(">  5um", RIGHT, 96, 0);         
    myGLCD.print("> 10um", RIGHT, 116, 0);         
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
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm100_min, RIGHT, 112, 4, ' ');    
  }  
}


void DisplayPM_1 (){
  unsigned int index;
  if(smog_updisp){
    smog_updisp=false;
    
    // PM1.0 Display
    index=(unsigned int) (pm10/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm10, LEFT, 28, 5, ' '); 
    
    // PM2.5 Display
    index=(unsigned int) (pm25/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm25, CENTER, 28, 5, ' ');      
      // PM10 Display
    index=(unsigned int) (pm100/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm100, RIGHT, 28, 5, ' ');
    
    // PM1.0 Average Display   
    index=(unsigned int) (pm10_avg/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm10_avg, LEFT, 56,  5, ' ');
    
    // PM2.5 Average Display   
    index=(unsigned int) (pm25_avg/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm25_avg, CENTER, 56,  5, ' ');
    
    // PM10 Average Display
    index=(unsigned int) (pm100_avg/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm100_avg, RIGHT, 56, 5, ' ');

  }
  if (smog_max_updisp){
    smog_max_updisp=false;
    // PM1.0 MAX Display
    index=(unsigned int) (pm10_max/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm10_max, LEFT, 84, 5, ' ');
    
    index=(unsigned int) (pm25_max/50);
    index = index >9? 9 : index;  

    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm25_max, CENTER, 84, 5, ' ');
    
    index=(unsigned int) (pm100_max/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm100_max, RIGHT, 84, 5, ' ');    
  }
  
  if (smog_min_updisp ){  
    smog_min_updisp=false;
    // PM1.0 MIN Display
    index=(unsigned int) (pm10_min/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm10_min, LEFT, 112, 5, ' ');
    
    // PM2.5 MIN Display
    index=(unsigned int) (pm25_min/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色  
    myGLCD.printNumI((word)pm25_min, CENTER, 112, 5, ' ');

    // PM10 MAX Display
    index=(unsigned int) (pm100_min/50);
    index = index >9? 9 : index;  
    myGLCD.setBackColor(smog_back_r[index],smog_back_g[index], smog_back_b[index]);//设置背景颜色
    myGLCD.printNumI((word)pm100_min, RIGHT, 112, 5, ' ');    
  }  
}

void DisplayPM_2 (){
  unsigned int index;
  if(smog_updisp){

    myGLCD.setFont(BigFont);          //设置字体为SmallFont格式(16*16字符)
    myGLCD.setBackColor(0,0,0);

    myGLCD.printNumI((word)pm3_count, LEFT, 12, 5, ' '); 
    myGLCD.printNumI((word)pm5_count, LEFT, 32, 5, ' '); 
    myGLCD.printNumI((word)pm10_count, LEFT, 52, 5, ' ');  
    myGLCD.printNumI((word)pm25_count, LEFT, 72, 5, ' ');  
    myGLCD.printNumI((word)pm50_count, LEFT, 92, 5, ' ');  
    myGLCD.printNumI((word)pm100_count, LEFT, 112, 5, ' ');    
  }  
}
/*
void DisplayUpdate(){
  if((lowByte(received_pkg) & 0x3F) == 0){
    myGLCD.clrScr(); //清屏
    smog_max_updisp=true;
    smog_min_updisp=true;
    Displaylabel();
  } else if ((lowByte(received_pkg) & 0x3F) == 30){
    myGLCD.clrScr(); //清屏
    smog_max_updisp=true;
    smog_min_updisp=true;
    Displaylabel_1();  
  } else if ((lowByte(received_pkg) & 0x3F) == 50){
    myGLCD.clrScr(); //清屏
    smog_max_updisp=true;
    smog_min_updisp=true;
    Displaylabel_2();  
  }
  
  if((lowByte(received_pkg) & 0x3F) < 30){
    DisplayPM();
  } else if((lowByte(received_pkg) & 0x3F) < 50){
    DisplayPM_1();
  } else {
    DisplayPM_2();  
  }
  
}
*/
void DisplayUpdate(){
  if((lowByte(received_pkg) & 0x3F) == 0){
    myGLCD.clrScr(); //清屏
    smog_max_updisp=true;
    smog_min_updisp=true;
    Displaylabel();
  } else if ((lowByte(received_pkg) & 0x3F) == 50){
    myGLCD.clrScr(); //清屏
    smog_max_updisp=true;
    smog_min_updisp=true;
    Displaylabel_2();  
  }
  
  if((lowByte(received_pkg) & 0x3F) < 50){
    DisplayPM();
  } else {
    DisplayPM_2();
  }
  
}

