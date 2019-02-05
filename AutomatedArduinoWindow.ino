
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <Servo.h>
#include <Wire.h>
#include <dht11.h>

#define SLAVE_ADDRESS 0X40
//Commands from Raspberry Pi
#define OPEN_WINDOW_CMD 0
#define CLOSE_WINDOW_CMD 1
#define INSIDE_MEAS_CMD 2
#define OUTSIDE_MEAS_CMD 3
#define DHT11_DATA_PIN A0

dht11 DHT; //Note:DHT on behalf of the temperature and humidity sensor

//const int dht11_data = 12;

char insideTemp[5];
char outsideTemp[5];
char insideHum[5];
char outsideHum[5];
char sendCurData[5];
char prevTemp[5];
 char prevHum[5];
int in1Pin = 12;
int in2Pin = 11;
int in3Pin = 10;
int in4Pin = 9;
int sda = 20;
int scl = 21;
int pos = 0; //stores servo position
int refreshLCD = 0;
bool currentWinOpen = false;
int openWinStatus = 0;
bool insideReadingRequest = false;
bool outsideReadingRequest = false;
bool consistentReading = false;
bool firstTime = true;


LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
Stepper winMotor(512, in1Pin, in2Pin, in3Pin, in4Pin); //stepper motor operates the window
Servo tempMotor; //servo motor operates the temperature flap

void setup() {
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(in3Pin, OUTPUT);
  pinMode(in4Pin, OUTPUT);
  tempMotor.attach(8);
  
  lcd.begin(16,2);
  delay(2000);
  lcd.clear();
  
  while (!Serial);
  
  Serial.begin(9600);
  winMotor.setSpeed(20);
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  winMotor.step(-400);
  getInsideReading();
  getOutsideReading();
  firstTime = false;
}

void loop() {
  // put your main code here, to run repeatedly
  
  if (Serial.available())
  {
    int steps = Serial.parseInt();
    winMotor.step(steps);
  }
  
  openCloseWindow(openWinStatus); //opens or closes window based on current status

  if (openWinStatus == 2) {
    for (int i = 0; i < 5; i++) {
      sendCurData[i] = insideTemp[i];
    }
  } else if (openWinStatus == 3) {
    for (int i = 0; i < 5; i++) {
      sendCurData[i] = outsideTemp[i];
    }
  } else if (openWinStatus == 4) {
    for (int i = 0; i < 5; i++) {
      sendCurData[i] = insideHum[i];
    }
  } else if (openWinStatus == 5) {
    for (int i = 0; i < 5; i++) {
      sendCurData[i] = outsideHum[i];
    }
  }

  
  if (refreshLCD==19) {
    getInsideReading();
  } 
  else if(refreshLCD==0) {
    getOutsideReading();
  }
  if (refreshLCD<10) {
    displayInsideData();
    refreshLCD++;
  } else if (refreshLCD<19) {
    displayOutsideData();
    refreshLCD++;
  } else {
    refreshLCD = 0;
  }

  if(insideTemp > 26 && currentWinOpen == false) {
  winMotor.step(400);
  currentWinOpen = true;
  } else if (insideTemp < 27 && currentWinOpen == true) {
  winMotor.step(-400);
  currentWinOpen = false;
  }
    
  delay(500);
}

void openCloseWindow(int openWin) {
    if(openWin == 0 && currentWinOpen == false) {
      winMotor.step(400);
      currentWinOpen = true;
    } else if (openWin == 1 && currentWinOpen == true) {
      winMotor.step(-400);
      currentWinOpen = false;
    }
}

void getInsideReading() {
  bool consistentReading = false;
  
  while(!consistentReading) {
    if(firstTime) {
      gettingReadingScreen(4000);
    } else {
      delay(4000);
    }
    DHT.read(DHT11_DATA_PIN);
    itoa(DHT.temperature, insideTemp, 10);
    itoa(DHT.humidity, insideHum, 10);

    Serial.print("Temp: ");
    Serial.println(insideTemp);
    Serial.print("Prev: ");
    Serial.println(prevTemp);
    Serial.print("Hum: ");
    Serial.println(insideHum);
    Serial.print("Prev: ");
    Serial.println(prevHum);
    
    //if(int(insideTemp) == int(prevTemp) && int(insideHum-prevHum)<= 1 && int(insideHum-prevHum)>= -1) {
      //consistentReading = true;
    //}
    for(int i = 0; i < 5; i++) {
      prevTemp[i] = insideTemp[i];
      prevHum[i] = insideHum[i];
    }
    consistentReading = true;
  }
  insideReadingRequest = false;
}

void getOutsideReading() {
  tempMotor.write(180);
  
  bool consistentReading = false;
  
  while(!consistentReading) {
    if(firstTime) {
      gettingReadingScreen(4000);
    } else {
      delay(4000);
    }
    Serial.print("Outside Temp: ");
    Serial.println(outsideTemp);
    Serial.print("Outside Hum: ");
    Serial.println(outsideHum);
    
    DHT.read(DHT11_DATA_PIN);
    itoa(DHT.temperature, outsideTemp, 10);
    itoa(DHT.humidity, outsideHum, 10);

    //if(outsideTemp == prevTemp && (outsideHum-prevHum)<= 1) {
    //  consistentReading = true;
   // }
    for(int i = 0; i < 5; i++) {
      prevTemp[i] = outsideTemp[i];
      prevHum[i] = outsideHum[i];
    }
    consistentReading = true;
  }
  outsideReadingRequest = false;
  //tempMotor.write(0);
}

void sendData() {
  for(int i = 0; i<=5; i++){
    Wire.write(sendCurData[i]);
  }
}

// callback for received data
void receiveData(int byteCount){
  while(Wire.available()) {
    openWinStatus = Wire.read();
    Serial.print(openWinStatus);
    Serial.println("--------------------------------------");
    Serial.print("data received: ");
    Serial.println(openWinStatus);
  }
}


void gettingReadingScreen(int waitTime) {
  lcd.clear();
  lcd.print("Acquiring");
  lcd.setCursor(0,1);
  lcd.print("Measurements");

  int cycle = 0;
  for(int timeMS = 0; timeMS < waitTime; timeMS+=250) {
    switch(cycle){
      case 0:
        lcd.setCursor(14,0);
        lcd.print("* ");
        lcd.setCursor(14,1);
        lcd.print("  ");
        break;
     case 1:
        lcd.setCursor(14,0);
        lcd.print(" *");
        break;
     case 2:
        lcd.setCursor(14,0);
        lcd.print("  ");
        lcd.setCursor(14,1);
        lcd.print(" *");
        break;
      case 3:
        lcd.setCursor(14,1);
        lcd.print("* ");
        break;
      default:
        break;
    }
    cycle++;
    delay(250);
  }
}

void displayInsideData(){
  lcd.clear();
  lcd.print("IN    Hum: ");
  lcd.print(insideHum);
  lcd.print("%");
  lcd.setCursor(0,1);
  lcd.print("SIDE  Temp:");
  lcd.print(insideTemp);
  lcd.write(0xDF);
  lcd.print("C");
}

void displayOutsideData(){
  lcd.clear();
  lcd.print("OUT   Hum: ");
  lcd.print(outsideHum);
  lcd.print("%");
  lcd.setCursor(0,1);
  lcd.print("SIDE  Temp:");
  lcd.print(outsideTemp);
  lcd.write(0xDF);
  lcd.print("C");
}
