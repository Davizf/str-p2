#define SAMPLE_TIME 250

//  pin
int soundPin = 11;
int ledPin = 13;
int buttonPin = 7;


//  global variables
int button_old_value;
int muteOn = 0; // 0 ->off & 1 -> on
unsigned long timeOrig;

void play_bit()
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
