#define SAMPLE_TIME 250
long CLOCK_SYSTEM = 16000000;
long prescaled = 1;

//  pin
int soundPin = 11;
int ledPin = 13;
int buttonPin = 7;


//  global variables
int button_old_value;
int muteOn = 0; // 0 ->off & 1 -> on
unsigned long timeOrig;

ISR(TIMER1_COMPA_vect)
{
  static int bitwise = 1;
  static unsigned char data = 0;


    if (bitwise > 128) {
        bitwise = 1;
        if (Serial.available()>1) {
           data = Serial.read();
        }
    }
    
    noInterrupts();
    int localMuteOn = muteOn;
    interrupts();
    
    if(localMuteOn){
      digitalWrite(soundPin, 0);
    }else{
      digitalWrite(soundPin, (data & bitwise) );
    }

    bitwise = (bitwise * 2);

}

void check_button(){
  int new_val = digitalRead(buttonPin);
  if(button_old_value == 0 && new_val == 1 ){
    
    noInterrupts();
    muteOn = 1 - muteOn;
    interrupts();
    
  }
  button_old_value = new_val;

   noInterrupts();
   int localMuteOn = muteOn;
   interrupts();
    
  if(localMuteOn){
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

    TCCR1A =  0; 
    TCCR1B = _BV(WGM12) | _BV(CS10);  // CTC Mode & prescaled 1
    TIMSK1= _BV(OCIE1A); 
    // CLOCK_SYSTEM / ( prescaled * (1 second / 250 us ))
    OCR1A = CLOCK_SYSTEM / (prescaled * ( 1000000 / 250) );
    
    Serial.begin(115200);
    timeOrig = micros();
}

void loop ()
{
    unsigned long timeDiff;
    check_button();
    timeDiff = SAMPLE_TIME - (micros() - timeOrig);
    timeOrig = timeOrig + SAMPLE_TIME;
    delayMicroseconds(timeDiff);
}
