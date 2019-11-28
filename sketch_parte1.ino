#define SAMPLE_TIME 250

//  pin
int soundPin = 11;
int ledPin = 13;
int buttonPin = 7;


//  global variables
int button_old_value;
int muteOn = 0; // 0 ->off & 1 -> on
unsigned long timeOrig;

ISR(TIMER2_COMPA_vect)
{
  static int bitwise = 1;
  static unsigned char data = 0;


    if (bitwise > 128) {
        bitwise = 1;
        if (Serial.available()>1) {
           data = Serial.read();
        }
    }
    if(muteOn){
      digitalWrite(soundPin, 0);
    }else{
      digitalWrite(soundPin, (data & bitwise) );
    }

    bitwise = (bitwise * 2);
}

void check_button(){
  int new_val = digitalRead(buttonPin);
  if(button_old_value == 0 && new_val == 1 ){
     muteOn = 1 - muteOn;
  }
  button_old_value = new_val;
  
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
    
    
    TIMSK2 = _BV(OCIE2A);
    
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
