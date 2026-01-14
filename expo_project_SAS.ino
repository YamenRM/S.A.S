#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID   "TMPL6K4tVS6rR"
#define BLYNK_TEMPLATE_NAME "structure analysis system"
#define BLYNK_AUTH_TOKEN    "83HQhEaGWanqx6urPVkpqeYdvmjCT4v9"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "KAREEM";
char pass[] = "12345678";



const int TRIG1 = 18; const int ECHO1 = 19;
const int TRIG2 = 21; const int ECHO2 = 22;
const int TRIG3 = 23; const int ECHO3 = 25;
const int TRIG4 = 26; const int ECHO4 = 27;

const int PIR_PIN = 33;
const int BUZZER = 5;


const int LED_GREEN = 13;
const int LED_YELLOW = 12;
const int LED_RED = 14;
const int LED_PIR = 17;


const int RESET_BUTTON = 4;


float THRESHOLD_WARNING = 1.5;  
float THRESHOLD_DANGER = 4.0;

float noiseMax = 0;
bool calibrated = false;
unsigned long calibStartTime = 0;
bool printedCalibMsg = false;



unsigned long lastLoopTime = 0;



float baselinePosX = 0;
float baselineNegX = 0;


long readUltrasonicMedian(int trigPin, int echoPin) {
  long readings[5];
  int count = 0;
  for (int i = 0; i < 5; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000);
    long distance = ((duration * 0.034) / 2) * 10; 
    readings[i] = distance;
    delay(10);
  }


  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (readings[j] < readings[i]) {
        long t = readings[i];
        readings[i] = readings[j];
        readings[j] = t;
      }
    }
  }
  return readings[2]; 
}


BLYNK_WRITE(V3) { 
  if (param.asInt() == 1) {
    long d1 = readUltrasonicMedian(TRIG1, ECHO1);
    long d2 = readUltrasonicMedian(TRIG2, ECHO2);
    long d3 = readUltrasonicMedian(TRIG3, ECHO3);
    long d4 = readUltrasonicMedian(TRIG4, ECHO4);
    baselinePosX = (d1 + d2) / 2.0;
    baselineNegX = (d3 + d4) / 2.0;
    Serial.println("Baseline reset from Blynk");
  }
}


BLYNK_WRITE(V4) {   
  if (param.asInt() == 1) {

    noiseMax = 0;
    calibrated = false;
    printedCalibMsg = false;
    calibStartTime = millis();

    Serial.println("Threshold calibration restarted from Blynk");
  }
}


void setup() {
  Serial.begin(115200);

  calibStartTime = millis();

  int trigPins[] = {TRIG1, TRIG2, TRIG3, TRIG4};
  int echoPins[] = {ECHO1, ECHO2, ECHO3, ECHO4};
  for (int i = 0; i < 4; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }

  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_PIR, OUTPUT);
  pinMode(RESET_BUTTON, INPUT_PULLUP);


  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);


  Serial.println("Calibrating baseline...");
  long d1 = readUltrasonicMedian(TRIG1, ECHO1);
  long d2 = readUltrasonicMedian(TRIG2, ECHO2);
  long d3 = readUltrasonicMedian(TRIG3, ECHO3);
  long d4 = readUltrasonicMedian(TRIG4, ECHO4);

  baselinePosX = (d1 + d2) / 2.0;
  baselineNegX = (d3 + d4) / 2.0;
  Serial.println("Baseline calibrated.");
}


void loop() {
  Blynk.run();


  if (millis() - lastLoopTime < 200) return;
  lastLoopTime = millis();




  if (digitalRead(RESET_BUTTON) == LOW) {
    Serial.println("Resetting baseline...");
    long d1 = readUltrasonicMedian(TRIG1, ECHO1);
    long d2 = readUltrasonicMedian(TRIG2, ECHO2);
    long d3 = readUltrasonicMedian(TRIG3, ECHO3);
    long d4 = readUltrasonicMedian(TRIG4, ECHO4);
    baselinePosX = (d1 + d2) / 2.0;
    baselineNegX = (d3 + d4) / 2.0;
    Serial.println("Baseline reset complete!");
  }


  long d1 = readUltrasonicMedian(TRIG1, ECHO1);
  long d2 = readUltrasonicMedian(TRIG2, ECHO2);
  long d3 = readUltrasonicMedian(TRIG3, ECHO3);
  long d4 = readUltrasonicMedian(TRIG4, ECHO4);

  float avgPosX = (d1 + d2) / 2.0;
  float avgNegX = (d3 + d4) / 2.0;
  float deltaPos = avgPosX - baselinePosX;
  float deltaNeg = avgNegX - baselineNegX;
  float deltaX = abs(deltaPos - deltaNeg);


 if (!calibrated && millis() - calibStartTime < 10000) {
  if (deltaX > noiseMax && deltaX < 15) {
    noiseMax = deltaX;
  }
  
 }

 if (!calibrated && millis() - calibStartTime >= 10000) {
  THRESHOLD_WARNING = noiseMax + 1.0;
  THRESHOLD_DANGER = noiseMax + 3.0;
  calibrated = true;


  Blynk.virtualWrite(V5, THRESHOLD_WARNING);
  Blynk.virtualWrite(V6, THRESHOLD_DANGER);


  if (!printedCalibMsg) {
    Serial.println("Calibration completed successfully.");
    Serial.print("WARNING threshold set to: ");
    Serial.print(THRESHOLD_WARNING);
    Serial.println(" mm");

    Serial.print("DANGER threshold set to: ");
    Serial.print(THRESHOLD_DANGER);
    Serial.println(" mm");

    printedCalibMsg = true;
  }

 }



  int pirState = digitalRead(PIR_PIN);
  digitalWrite(LED_PIR, pirState ? HIGH : LOW);


  if (!calibrated) {
   digitalWrite(LED_GREEN, millis() % 500 < 250);
   digitalWrite(LED_YELLOW, LOW);
   digitalWrite(LED_RED, LOW);
  }
  else{
    if (deltaX <= THRESHOLD_WARNING) {
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_RED, LOW);
      digitalWrite(BUZZER, LOW);
      Blynk.virtualWrite(V1, "SAFE");
    }
    else if (deltaX <= THRESHOLD_DANGER) {
     digitalWrite(LED_GREEN, LOW);
     digitalWrite(LED_YELLOW, HIGH);
     digitalWrite(LED_RED, LOW);
     digitalWrite(BUZZER, LOW);
     Blynk.virtualWrite(V1, "WARNING");
    } 
    else { 
     digitalWrite(LED_GREEN, LOW);
     digitalWrite(LED_YELLOW, LOW);
     digitalWrite(LED_RED, HIGH);
     digitalWrite(BUZZER, HIGH);
     Blynk.virtualWrite(V1, "DANGER");
    }

    
  }
  


  Blynk.virtualWrite(V0, deltaX); 
  Blynk.virtualWrite(V2, pirState); 


  Serial.print("d1: "); Serial.print(d1);
  Serial.print(" | d2: "); Serial.print(d2);
  Serial.print(" | d3: "); Serial.print(d3);
  Serial.print(" | d4: "); Serial.print(d4);
  Serial.print(" | ΔX: "); Serial.print(deltaX);
  Serial.print(" | PIR: "); Serial.println(pirState);

}
