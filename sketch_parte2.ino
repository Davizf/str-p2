#define SAMPLE_TIME 250 

//  pin 
int soundPin = 11;
int ledPin = 13;
int buttonPin = 7;


//  global variables
int muteOn = 0; // 0 ->off & 1 -> on
unsigned long timeOrig;

void play_bit() 
{ 
  static unsigned char data = 0;
  if (Serial.available()>1) {
    data = Serial.read();
  }  
  if(muteOn){
    OCR2A = 0;
  }else{
    OCR2A = data;
  }
}

void check_button(){
  if(digitalRead(buttonPin)){
    muteOn = 1 - muteOn;
  }
}


void check_led(){
  if(muteOn){
    digitalWrite(ledPin, HIGH);
  }else{
    digitalWrite(ledPin, LOW);  
  }
}

void setup ()
{
    pinMode(soundPin, OUTPUT);
    pinMode(ledPin, OUTPUT);
    pinMode(buttonPin, INPUT);
    Serial.begin(115200);
    timeOrig = micros();  
    TCCR2A = _BV(WGM20) | _BV(COM2A1) | _BV(COM2B1) ;  
    TCCR2B =  _BV(CS22);
}

void loop ()
{
    unsigned long timeDiff;
    play_bit();
    check_button();
    check_led();
    timeDiff = SAMPLE_TIME - (micros() - timeOrig);
    timeOrig = timeOrig + SAMPLE_TIME;
    delayMicroseconds(timeDiff);
}
