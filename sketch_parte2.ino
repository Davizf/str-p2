#define SAMPLE_TIME 250
long CLOCK_SYSTEM = 16000000;
long prescaled = 1;

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
    int compValue=CLOCK_SYSTEM/(prescaled * data);
    OCR2A = compValue;
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
    TCCR2A = _BV(COM2A1) | _BV(WGM20) | _BV(CS20);

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
