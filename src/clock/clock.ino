//EEPROM map
//地址       说明
//0         按键是否开启

#include "I2C_BM8563.h"
#include "LowPower.h"
#include <EEPROM.h>

I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire);
byte pin[11] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, A1};

//地址 寄存器名称 Bit7 Bit6 Bit5 Bit4 Bit3 Bit2 Bit1 Bit0
//02H 秒 VL 00～59BCD 码格式数
//03H 分钟 — 00～59BCD 码格式数
//04H 小时 — — 00～23BCD 码格式数
//05H 日 — — 01～31BCD 码格式数
//06H 星期 — — — — — 0～6
//07H 月/世纪 C — — 01～12BCD 码格式数
//08H 年 00～99BCD 码格式数
//09H 分钟报警 AE 00～59BCD 码格式数
//0AH 小时报警 AE — 00～23BCD 码格式数
//0BH 日报警 AE — 01～31BCD 码格式数
//0CH 星期报警 AE — — — — 0～6
//byte  reg_addr = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C}; //bm8563 寄存器地址，用于读写寄存器

I2C_BM8563_DateTypeDef set_dateStruct;
unsigned long time;
unsigned long key1_press_time, key2_press_time;
bool key1_pressed = false;
bool key2_pressed = false;
byte mode = 0; //0:查看模式；1：设置模式
byte main_menu = 0;
byte set_menu = 0;
byte alarm_min = 0x80;
byte alarm_hour = 0x80;
byte alarm_date = 0x80;
byte alarm_weekDay = 0x80;
byte set_alarm_min = 0x80;
byte set_alarm_hour = 0x80;
byte set_alarm_date = 0x80;
byte set_alarm_weekDay = 0x80;
bool beep_key = false;
byte num1, num2;

void printNum1(byte num1) {
  for (byte i = 0; i <= 5; i++) {
    if (((num1 >> i) & 0x01) == 0x01) {
      digitalWrite(pin[i], HIGH);
    } else {
      digitalWrite(pin[i], LOW);
    }
  }
}

void printNum2(byte num2) {
  for (byte i = 0; i <= 4; i++) {
    if (((num2 >> i) & 0x01) == 0x01) {
      digitalWrite(pin[6 + i], HIGH);
    } else {
      digitalWrite(pin[6 + i], LOW);
    }
  }
}


void pinledInit() {
  for (byte i = 0; i <= 10; i++ ) {
    pinMode(pin[i], OUTPUT);
    digitalWrite(pin[i], LOW);
  }
}

void pinbeepInit() {
  pinMode(A2, OUTPUT);
  digitalWrite(A2, LOW);
}

uint8_t byteToBcd2(uint8_t value) {
  uint8_t bcdhigh = 0;

  while (value >= 10) {
    bcdhigh++;
    value -= 10;
  }

  return ((uint8_t)(bcdhigh << 4) | value);
}

uint8_t bcd2ToByte(uint8_t value) {
  uint8_t tmp = 0;
  tmp = ((uint8_t)(value & (uint8_t)0xF0) >> (uint8_t)0x4) * 10;
  return (tmp + (value & (uint8_t)0x0F));
}

void rtc_interrupt_enable() {
  Wire.beginTransmission(I2C_BM8563_DEFAULT_ADDRESS);
  Wire.write(0x01);
  Wire.write(0x02);
  Wire.endTransmission();
}

void rtc_warn_set(byte hour, byte min) {
  Wire.beginTransmission(I2C_BM8563_DEFAULT_ADDRESS);
  Wire.write(0x09);
  Wire.write(byteToBcd2(min));
  Wire.write(byteToBcd2(hour));
  Wire.write(0x80);
  Wire.write(0x80);
  Wire.endTransmission();
}

void pinkeyInit() {
  //  pinMode(2, OUTPUT);
  //  pinMode(A0, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(A0, HIGH);
  pinMode(2, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
}


void set_reg(byte addr, byte value) {
  Wire.beginTransmission(I2C_BM8563_DEFAULT_ADDRESS);
  Wire.write(addr);
  Wire.write(value);
  Wire.endTransmission();
}

byte get_reg(byte addr) {
  byte result;
  Wire.beginTransmission(I2C_BM8563_DEFAULT_ADDRESS);
  Wire.write(addr);
  Wire.endTransmission();
  Wire.requestFrom(I2C_BM8563_DEFAULT_ADDRESS, 1);


  while (Wire.available()) {
    result = Wire.read();
  }

  return result;
}

void setup() {
  // put your setup code here, to run once:
  // Init I2C
  Wire.begin();

  // Init RTC
  rtc.begin();
  pinledInit();
  pinkeyInit();
  pinbeepInit();
  time = millis();


  if (EEPROM.read(0) == 1) {
    beep_key = true;
  } else {
    beep_key = false;
  }

}



void loop() {
  I2C_BM8563_DateTypeDef dateStruct;
  I2C_BM8563_TimeTypeDef timeStruct;

  rtc.getDate(&dateStruct);
  rtc.getTime(&timeStruct);


  if (mode == 0) {
    switch (main_menu) {
      case 0:  //日历-时，分
        num1 = timeStruct.minutes;
        num2 = timeStruct.hours;
        break;
      case 1: //日历-年
        num1 = dateStruct.year;
        num2 = 1;
        break;
      case 2: //日历-月
        num1 = dateStruct.month;
        num2 = 2;
        break;
      case 3: //日历-天
        num1 = dateStruct.date;
        num2 = 3;
        break;
      case 4:  //日历-星期
        num1 = dateStruct.weekDay;
        num2 = 4;
        break;
      case 5: //闹钟-分
        num1 =  bcd2ToByte(get_reg(0x09));
        num2 = 5;
        break;
      case 6:  //闹钟-时
        num1 =  bcd2ToByte(get_reg(0x0A));
        num2 = 6;
        break;
      case 7:  //闹钟-日
        num1 =  bcd2ToByte(get_reg(0x0C));
        num2 = 7;
        break;
      case 8:   //闹钟-星期
        num1 =  bcd2ToByte(get_reg(0x0D));
        num2 = 8;
        break;
      case 9:    //按键声音开关
        if (beep_key) {
          num1 = 1;
        } else {
          num1 = 0;
        }
        num2 = 9;
        break;
      default: break;
    }
  }


  //key1按键处理
  if (digitalRead(2) == LOW) {
    key1_pressed = true;

    if ((millis() - key1_press_time) > 3000) {  //key1 长按  模式切换
      key1_pressed = false;
      if (beep_key) {
        tone(A2, 1000, 100);
      }
      if (mode == 0) {
        mode = 1;
        set_menu = 0;
        num1 = timeStruct.hours;
        num2 = 0;
      } else {
        mode = 0;
      }
      key1_press_time = millis();
    }
  } else if (key1_pressed && (digitalRead(2) == HIGH)) { //key1 短按
    key1_pressed = false;

    if (beep_key) {
      tone(A2, 1000, 100);
    }
    if (mode == 0) {  //查看模式，切换信息
      main_menu++;
      if (main_menu >= 10) {
        main_menu = 0;
      }
    } else {     //设置模式，切换设置信息
      set_menu++;
      if (set_menu >= 12) {
        set_menu = 0;
      }

      switch (set_menu) {
        case 0:  //日历设置-时
          num1 = timeStruct.hours;
          num2 = 0;
          break;
        case 1:  //日历设置-分
          num1 = timeStruct.minutes;
          num2 = 1;
          break;
        case 2:  //日历设置-秒
          num1 = timeStruct.seconds;
          num2 = 2;
          break;
        case 3:  //日历设置-年低位
          num1 = dateStruct.year;
          num2 = 3;
          break;
        case 4:  //日历设置-月
          num1 = dateStruct.month;
          num2 = 4;
          break;
        case 5:  //日历设置-日
          num1 = dateStruct.date;
          num2 = 5;
          break;
        case 6:  //日历设置-星期
          num1 = dateStruct.weekDay;
          num2 = 6;
          break;
        case 7:  //闹钟设置-分
          
          if ((get_reg(0x09) & 0x80 ) == 0x80) {
            num1 = 0xFF;
          }else{
            num1 = bcd2ToByte(get_reg(0x09));
            }
          num2 = 7;
          break;
        case 8:  //闹钟设置-时
           if ((get_reg(0x0A) & 0x80 ) == 0x80) {
            num1 = 0xFF;
          }else{
            num1 = bcd2ToByte(get_reg(0x0A));
            }
          num2 = 8;
          break;
        case 9:  //闹钟设置-日
          if ((get_reg(0x0B) & 0x80 ) == 0x80) {
            num1 = 0xFF;
          }else{
            num1 = bcd2ToByte(get_reg(0x0B));
            }
          num2 = 9;
          break;
        case 10:  //闹钟设置-星期
           if ((get_reg(0x0C) & 0x80 ) == 0x80) {
            num1 = 0xFF;
          }else{
            num1 = bcd2ToByte(get_reg(0x0C));
            }
          num2 = 10;
          break;
        case 11:  //按键音设置开关
          if (beep_key) {
            num1 = 1;
          } else {
            num1 = 0;
          }
          num2 = 11;
          break;
      }




    }
    key1_press_time = millis();
  } else {
    key1_press_time = millis();
  }




  //key2长按3s进入掉电模式，短按在查看模式是调整日历，在设置模式是设置的调整

  //key2按键处理
  if (digitalRead(A0) == LOW) {
    key2_pressed = true;
    if ((millis() - key2_press_time) > 3000) {
      key2_pressed = false;
      if (beep_key) {
        tone(A2, 1000, 100);
      }
      pinledInit();
      attachInterrupt(0, wakeUp, LOW);
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
      detachInterrupt(0);
    }
  } else if (key2_pressed && (digitalRead(A0) == HIGH)) { //key2 短按
    key2_pressed = false;
    if (beep_key) {
      tone(A2, 1000, 100);
    }

    if (mode == 0) {   //查看模式该键暂时无效

    } else {         //设置模式，该键是增加
      switch (set_menu) {
        case 0:  //日历设置-时
          num1++;
          if (num1 >= 24) {
            num1 = 0;
          }
          set_reg(0x04, byteToBcd2(num1));
          //rtc hour set
          break;
        case 1:  //日历设置-分
          num1++;
          if (num1 >= 60) {
            num1 = 0;
          }
          set_reg(0x03, byteToBcd2(num1));
          break;
        case 2:  //日历设置-秒
          num1++;
          if (num1 >= 60) {
            num1 = 0;
          }
          set_reg(0x02, byteToBcd2(num1));
          break;
        case 3:  //日历设置-年
          num1++;
          if (num1 >= 61) {
            num1 = 0;
          }

          set_reg(0x08, byteToBcd2(  num1));
          break;
        case 4:  //日历设置-月
          num1++;
          if (num1 >= 13) {
            num1 = 1;
          }
          set_reg(0x07, byteToBcd2(num1));
          break;
        case 5:  //日历设置-日
          num1++;
          if (num1 >= 31) {
            num1 = 0;
          }
          set_reg(0x05, byteToBcd2(num1));
          break;
        case 6:  //日历设置-星期
          num1++;
          if (num1 >= 7) {
            num1 = 0;
          }
          set_reg(0x06, byteToBcd2(num1));
          break;
        case 7:  //闹钟设置-分
          num1++;
          if (num1 >= 60) {
            num1 = 0xFF;    //全是1代表取消报警
            set_reg(0x09, 0x80);
          }  else {
            set_reg(0x09, byteToBcd2(num1));
          }


          break;
        case 8:  //闹钟设置-时
          num1++;
          if (num1 >= 24) {
            num1 = 0xFF;
            set_reg(0x0A, 0x80);
          } else {
            set_reg(0x0A, byteToBcd2(num1));
          }

          break;
        case 9:  //闹钟设置-日
          num1++;
          if (num1 >= 32) {
            num1 = 0xFF;
            set_reg(0x0B, 0x80);
          }  else {
            set_reg(0x0B, byteToBcd2(num1));
          }
          break;
        case 10:  //闹钟设置-星期
          num1++;
          if (num1 >= 7) {
            num1 = 0xFF;
            set_reg(0x0C, 0x80);
          }  else {
            set_reg(0x0C, num1);
          }

          break;
        case 11:  //按键音设置开关
          if (num1 == 0) {
            num1 = 1;
            beep_key = true;
          } else {
            num1 = 0;
            beep_key = false;
          }
          EEPROM.update(0, num1);
          break;
      }

    }
    key2_press_time = millis();
  } else {
    key2_press_time = millis();
  }


  printNum1(num1);
  printNum2(num2);

}




void wakeUp() {

}
