#include "Joystick.h"

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, 20, 1,
  true, true, false, false, false, false,
  false, false, false, false, false);

//проверка работы 
bool Debug = false;
bool DebugButton = false;

// пины к которым подключены регистры
const int DataPin = 15;
const int ClockPin = 14;
const int LatchPin = 16;

//пины тумблера
const int ToggleSwitchPin1 = 8;
const int ToggleSwitchPin2 = 9;

// пины осей
 const int AxisY = 3;
 const int AxisX = 2;

// название кнопок в игре
#define S1  !(buttonInputs1 & 0x80) /* Nose Wheel Steering - Рулевое управление носовым колесом */  
#define S2  !(buttonInputs1 & 0x40) /* Pickle */   
#define TG2 !(buttonInputs1 & 0x20) /* Trigger 2 - тригер 2 */
#define TG1 !(buttonInputs1 & 0x10) /* Trigger 1 - тригер 1 */ 

#define H1U !(buttonInputs1 & 0x08) /* Trim */   
#define H1D !(buttonInputs1 & 0x04)    
#define H1R !(buttonInputs1 & 0x02)
#define H1L !(buttonInputs1 & 0x01)
 
#define H4L !(buttonInputs2 & 0x80) /* CMS */   
#define H4R !(buttonInputs2 & 0x40)
#define H4D !(buttonInputs2 & 0x20)
#define H4U !(buttonInputs2 & 0x10)

#define H3R !(buttonInputs2 & 0x08) /* DMS */   
#define H3L !(buttonInputs2 & 0x04)
#define H3D !(buttonInputs2 & 0x02)
#define H3U !(buttonInputs2 & 0x01)
 
//#define BUTTON1  !(buttonInputs3 & 0x80)    /*НЕТ КНОПКИ*/
//#define BUTTON2  !(buttonInputs3 & 0x40)    /*НЕТ КНОПКИ*/
#define S3 !(buttonInputs3 & 0x20) /* Pinky Switch - кнопка под мизинцем */
#define S4 !(buttonInputs3 & 0x10) /* Paddle Switch - подрулевой переключатель*/

#define H2R !(buttonInputs3 & 0x08)    /* TMS */
#define H2L !(buttonInputs3 & 0x04)
#define H2D !(buttonInputs3 & 0x02)
#define H2U !(buttonInputs3 & 0x01)

#define B19 !(digitalRead (ToggleSwitchPin1))
#define B20 !(digitalRead (ToggleSwitchPin2))

byte buttonInputs1 = 72;
byte buttonInputs2 = 159;
byte buttonInputs3 = 168;

// Hardware definitions
int minp = 0;                 // min adc value
int maxp = 1023;              // max adc value
int medp = (maxp+1)/2;        // med adc value
float mdz = 0.1*(maxp+1);     // max dead zone (10%)
float ndz = 0.05*(maxp+1);    // normal dead zone (5%)

// Axes definitions
float lx = 0;                 // last x value
float ly = 0;                 // last y value
int x = 0;
int y = 0;
int minx = minp + mdz;        // min x value
int miny = minp + mdz;        // min y value
int maxx = maxp - mdz;        // max x value
int maxy = maxp - mdz;        // max y value
int medx = medp;              // med x value
int medy = medp;              // med y value

// считывание регистров
byte shiftIn (int myDataPin, int myClockPin)
{
  int i;
  int temp = 0;
  int PinState;
  byte myDataIn = 0;
  
  pinMode (myClockPin, OUTPUT);
  pinMode (myDataPin, INPUT);

  for (i = 7; i >= 0; i--)
  {
    digitalWrite (myClockPin, 0);
    delayMicroseconds (2);
    temp = digitalRead (myDataPin);
    
    if (temp)
    {
      PinState = 1;
      myDataIn = myDataIn | (1 << i);
    }
    else PinState = 0;
    digitalWrite (myClockPin, 1);
  }
  return myDataIn;
}


// Фильтр осей
int filter(int a, float *la) {
  float alpha = 0.25;     // 1/4 filter
  float f = *la;
  f = (alpha * a) + ((1 - alpha) * f);
  *la = f;
  int i = constrain(f, minp, maxp);
  return i;
}


// Автоматическая калибровка
int calib(int a, int *minv, int *maxv, int mev) {
  int miv = *minv;
  int mav = *maxv;
  int i = 0;
  if (a < miv) miv = a;   
  if (a > mav) mav = a;   
  *minv = miv;
  *maxv = mav;
  if (mev > 0) {
    if (a < mev-ndz/2) {
      i = map(a, miv+ndz, mev-ndz/2, 0, maxp/2);   // normal min dead zone & normal center dead zone
    } else if (a > mev+ndz/2) {
      i = map(a, mev+ndz/2, mav-ndz, (maxp+1)/2, maxp);  // normal max dead zone & normal center dead zone
    } else {
      i = medp;
    }
  } else {
    i = map(a, miv+ndz, mav-ndz, minp, maxp);  // normal min dead zone & normal max dead zone
  }
  i = constrain(i, minp, maxp);
  return i;
}


// кривая экспоненты
int curve(float a) {
  a -= maxp/2;
  a = a*abs(a)/medp;
  a += medp;
  return constrain(a,minp,maxp);
}

// программа при запуске
void setup() 
{
  if (Debug | DebugButton) Serial.begin (9600);

  pinMode (LatchPin, OUTPUT);
  pinMode (ClockPin, OUTPUT);
  pinMode (DataPin, INPUT);

  pinMode (ToggleSwitchPin1, INPUT_PULLUP);
  pinMode (ToggleSwitchPin2, INPUT_PULLUP);

  // Initialize Joystick to update status manually
  Joystick.begin(false);
  
  // Set joystick ranges
  Joystick.setXAxisRange(minp,maxp);    
  Joystick.setYAxisRange(minp,maxp);
  
  // Initialize min toe brakes values and mid yaw value
  medx = 1023 - analogRead(AxisX);
  medy = analogRead(AxisY);
  
  // initialize last values
  lx = medx;
  ly = medy;
}


//основная программа
void loop()
{
  digitalWrite (LatchPin, 1);
  delayMicroseconds (20);
  digitalWrite (LatchPin, 0);

  buttonInputs1 = shiftIn (DataPin,ClockPin);
  buttonInputs2 = shiftIn (DataPin,ClockPin);
  buttonInputs3 = shiftIn (DataPin,ClockPin);

  // Write to joystick buttons
  Joystick.setButton(0,  TG1);
  Joystick.setButton(1,  S2);
  Joystick.setButton(2,  S3);
  Joystick.setButton(3,  S4);
  Joystick.setButton(4,  S1);
  Joystick.setButton(5,  TG2);
  Joystick.setButton(6,  H2U);
  Joystick.setButton(7,  H2R);
  Joystick.setButton(8,  H2D);
  Joystick.setButton(9,  H2L);
  Joystick.setButton(10, H3U);
  Joystick.setButton(11, H3R);
  Joystick.setButton(12, H3D);
  Joystick.setButton(13, H3L);
  Joystick.setButton(14, H4U);
  Joystick.setButton(15, H4R);
  Joystick.setButton(16, H4D);
  Joystick.setButton(17, H4L);
  Joystick.setButton(18, B19);
  Joystick.setButton(19, B20);

  // Determine Joystick Hat Position
  int angle = -1;
  if (H1U) 
  {
    if (H1R) 
    {
      angle = 45;
    } 
    else if (H1L) 
    {
      angle = 315;
    } 
    else 
    {
      angle = 0;
    }
  } 
  else if (H1D) 
  {
    if (H1R) 
    {
      angle = 135;
    } 
    else if (H1L) 
    {
      angle = 225;
    } 
    else 
    {
      angle = 180;
    }
  } 
  else if (H1R) 
  {
    angle = 90;
  } else if (H1L) 
  {
    angle = 270;
  }
  Joystick.setHatSwitch(0,angle);

  // Read the analog values into a variable
  x = 1023 - analogRead(AxisX);
  y = analogRead(AxisY);

  // Apply filter
  x = filter(x,&lx);
  y = filter(y,&ly);

  // Dinamic calibration
  x = calib(x,&minx,&maxx,medx);
  y = calib(y,&miny,&maxy,medy);

  // Apply exp curve to x & y
  x = curve(x);
  y = curve(y);

  // Write values to joystick
  Joystick.setXAxis(10*x);
  Joystick.setYAxis(10*y);
  
  // проверка кнопок
  if (DebugButton)
  {
    if (S3 == 1)Serial.println ("S3");   if (TG1 == 1)Serial.println ("TG1"); if (TG2 == 1)Serial.println ("TG2");
    if (S1 == 1)Serial.println ("S1");   if (S4 == 1)Serial.println ("S4");   if (S2 == 1)Serial.println ("S2");
/*if (BUTTON1 == 1)Serial.println ("BUTTON1");   if (BUTTON2 == 1)Serial.println ("BUTTON2");*/   if (H1D == 1)Serial.println ("H1D");
    if (H1U == 1)Serial.println ("H1U"); if (H1L == 1)Serial.println ("H1L"); if (H1R == 1)Serial.println ("H1R");
    if (H4D == 1)Serial.println ("H4D"); if (H4U == 1)Serial.println ("H4U"); if (H4L == 1)Serial.println ("H4L");
    if (H4R == 1)Serial.println ("H4R"); if (H3D == 1)Serial.println ("H3D"); if (H3U == 1)Serial.println ("H3U");
    if (H3L == 1)Serial.println ("H3L"); if (H3R == 1)Serial.println ("H3R"); if (H2D == 1)Serial.println ("H2D");
    if (H2U == 1)Serial.println ("H2U"); if (H2L == 1)Serial.println ("H2L"); if (H2R == 1)Serial.println ("H2R");
  }

  // проверка работы
  if (Debug) 
  {
    Serial.print(analogRead(AxisX)); Serial.print(" / "); Serial.print(analogRead(AxisY));
    Serial.print(" | ");
    Serial.print(x); Serial.print(" / "); Serial.print(y);
    Serial.print(" | ");
    Serial.print(minx); Serial.print(" / "); Serial.print(miny);
    Serial.print(" | ");
    Serial.print(maxx); Serial.print(" / "); Serial.print(maxy);
    Serial.print(" | ");
    Serial.print(medx); Serial.print(" / "); Serial.print(medy);
    Serial.print(" | ");
    Serial.print (buttonInputs1, BIN); Serial.print(" / "); Serial.print (buttonInputs2, BIN); Serial.print(" / "); Serial.print (buttonInputs3, BIN);
    Serial.print(" | ");
    Serial.print(B19); Serial.print(" / "); Serial.print (B20);
    Serial.println();
    Serial.println("-----------------------------------------------------------------------------------------------------------------");
    delay(500);
  }

  // Send values to the usb
  Joystick.sendState();
  // Delay 10ms (run 100 times per second)
  delay(10);
}
