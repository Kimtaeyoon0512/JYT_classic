#include <AccelStepper.h>
#include <DHT.h>

const int drv8825_type = 1;
const int interval = 550;
const int goto_gear = 2;
const int dht_pin = 40;
int joystick_gear = 1;
int stop_mode = 0;  //1 is nonstop, 0 is stop
String command_line = "";
const int tracking_interval = 1683;
int tracking_available = 1; //1 is tracking, 0 is non-tracking.
/*
  now setting, x1000 reduced by two gears, and using stepping motors with 200step
  Right accession axis have to turn 1 cycle during a full day.
  it means,
  1000(reduced gear) * 128(microstepping for tracking) * 200(steps)
  = 25,600,000 steps    in    24h = 24 * 60m = 24 * 60 * 60 sec
  -> 8,000 steps    in    27 sec
  -> 1steps    in    0.003375 sec = 3375us

  so delay variable have to be 1687.5
  but I cannot know delaymicroseconds()'s variable type is int or double or else.
  so I think,
  variable be 1688 -> It goes 25,592,417.xxx in full day.
  but stars goes almost 361 degrees a day.
  so I think again,
  variable be 1687 -> It goes 25,607,587.xxx in full day.

  maybe, that's enough.
  but not satisfied. caculate again.

  variable be 1686 -> It goes 25,622,775.xxx in full day.

  it goes almost, 360.3 degrees a day.

  variable be 1685 -> 360.5 degrees a day.
  variable be 1683 -> 360.962532 degrees a day
  fxxk!!!!!!!!
  variable be 1682 -> 361.17684 degrees a day

  I'm satisfied with 1683.
*/



//-------------------pin num initializing
const int RA_step = 2;
const int RA_dir = 3;
const int RA_M0 = 4;
const int RA_M1 = 5;
const int RA_M2 = 6;

const int DEC_step = 8;
const int DEC_dir = 9;
const int DEC_M0 = 10;
const int DEC_M1 = 11;
const int DEC_M2 = 12;
//--------------------

void microstepping_convert(int object_micro);
AccelStepper stepper_RA(drv8825_type, RA_step, RA_dir);
AccelStepper stepper_DEC(drv8825_type, DEC_step, DEC_dir);
DHT dht(dht_pin, DHT22);

//start program, initializing
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  dht.begin();

  //--------------------stepper initializing
  stepper_RA.setMaxSpeed(3000);
  stepper_RA.setAcceleration(1500);

  stepper_DEC.setMaxSpeed(3000);
  stepper_DEC.setAcceleration(1500);

  stepper_RA.setCurrentPosition(0);
  stepper_DEC.setCurrentPosition(0);
  //--------------------


  //RA initializing
  pinMode(RA_step, OUTPUT);   //step
  pinMode(RA_dir, OUTPUT);   //dir
  pinMode(RA_M0, OUTPUT);   //M0
  pinMode(RA_M1, OUTPUT);   //M1
  pinMode(RA_M2, OUTPUT);   //M2

  //DEC initializing
  pinMode(DEC_step, OUTPUT);   //step
  pinMode(DEC_dir, OUTPUT);   //dir
  pinMode(DEC_M0, OUTPUT);  //M0
  pinMode(DEC_M1, OUTPUT);  //M1
  pinMode(DEC_M2, OUTPUT);  //M2

}



//start program's main loop
void loop() {



  if (tracking_available == 1) {
    //-------------------Tracking
    microstepping_convert(128);//RA micro stepping
    digitalWrite(RA_dir, HIGH);//tracking direction (RA)
    digitalWrite(RA_step, HIGH);
    delayMicroseconds(1683);
    digitalWrite(RA_step, LOW);
    delayMicroseconds(1683);
  } else{
    delay(1);
  }
  


  //--------------------Signal processing
  while (Serial1.available()) {
    char command_temp = (char)Serial1.read();
    command_line += command_temp;
    delay(1);
  }
  //--------------------



  //--------------------
  if ( command_line == "tracking" ) {
    if (tracking_available == 1) {
      tracking_available = 0;
      Serial1.println("!!! stop tracking !!!");
    }
    else {
      tracking_available = 1;
      Serial1.println("tracking...");
    }
  }
  //--------------------


  //--------------------classic goto
  if ( (command_line[0] == 'R') && (command_line[10] == 'D') ) {

    //--------------------steps caculating
    double single_cycle_steps = goto_gear * 200 * 1000;//now, goto_gear is 2^^

    double RA_hours = 10 * ((double)command_line[2] - 48) + (command_line[3] - 48);
    double RA_minutes = 10 * ((double)command_line[5] - 48) + (command_line[6] - 48);
    double RA_seconds = 10 * ((double)command_line[8] - 48) + (command_line[9] - 48);

    double DEC_angles = 10 * ((double)command_line[12] - 48) + (command_line[13] - 48);
    double DEC_arcminutes = 10 * ((double)command_line[15] - 48) + (command_line[16] - 48);
    double DEC_arcseconds = 10 * ((double)command_line[18] - 48) + (command_line[19] - 48);

    Serial.println((RA_hours * 16666.667) + (RA_minutes * 277.778) + (RA_seconds * 4.630));
    Serial.println((DEC_angles * 1111.111) + (DEC_arcminutes * 18.519) + (DEC_arcseconds * 0.309));

    double RA_steps = (RA_hours * 16666.667) + (RA_minutes * 277.778) + (RA_seconds * 4.630);
    double DEC_steps = (DEC_angles * 1111.111) + (DEC_arcminutes * 18.519) + (DEC_arcseconds * 0.309);

    Serial.println(RA_steps);
    Serial.println(DEC_steps);
    //--------------------

    microstepping_convert(2);

    if (command_line[1] == '-') {
      RA_steps = -RA_steps;
    }
    if (command_line[11] == '-') {
      DEC_steps = -DEC_steps;
    }

    stepper_RA.moveTo(RA_steps);
    stepper_DEC.moveTo(DEC_steps);
    while ((stepper_RA.distanceToGo() + stepper_DEC.distanceToGo() != 0) ) {
      stepper_RA.run();
      stepper_DEC.run();
      if (Serial1.read() == 'E') {
        break;
      }
    }
    stepper_RA.stop();
    stepper_DEC.stop();
    stepper_RA.setCurrentPosition(0);
    stepper_DEC.setCurrentPosition(0);
  }
  //--------------------



  //--------------------classic joystick
  if (command_line == "RA_BACK") {
    digitalWrite(RA_dir, LOW);//RA_BACK
    microstepping_convert(joystick_gear);
    while (true) {
      digitalWrite(RA_step, HIGH); delayMicroseconds(interval);
      digitalWrite(RA_step, LOW); delayMicroseconds(interval);
      if (Serial1.available() != 0) {
        char command_temp = Serial1.read();
        if (command_temp == 'e') {
          if (stop_mode == 0) {
            if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
            break;
          }
        }
        if (command_temp == 'E') {
          if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
          break;
        }
      }
    }
  }

  else if (command_line == "RA_GO") {
    digitalWrite(RA_dir, HIGH);//RA_GO
    microstepping_convert(joystick_gear);
    while (true) {
      digitalWrite(RA_step, HIGH); delayMicroseconds(interval);
      digitalWrite(RA_step, LOW); delayMicroseconds(interval);
      if (Serial1.available() != 0) {
        char command_temp = Serial1.read();
        if (command_temp == 'e') {
          if (stop_mode == 0) {
            if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
            break;
          }
        }
        if (command_temp == 'E') {
          if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
          break;
        }
      }
    }
  }

  else if (command_line == "DEC_CW") {
    digitalWrite(DEC_dir, HIGH);//DEC_CW
    microstepping_convert(joystick_gear);
    while (true) {
      digitalWrite(DEC_step, HIGH); delayMicroseconds(interval);
      digitalWrite(DEC_step, LOW); delayMicroseconds(interval);
      if (Serial1.available() != 0) {
        char command_temp = Serial1.read();
        if (command_temp == 'e') {
          if (stop_mode == 0) {
            if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
            break;
          }
        }
        if (command_temp == 'E') {
          if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
          break;
        }
      }
    }
  }

  else if (command_line == "DEC_CCW") {
    digitalWrite(DEC_dir, LOW);//DEC_CCW
    microstepping_convert(joystick_gear);
    while (true) {
      digitalWrite(DEC_step, HIGH); delayMicroseconds(interval);
      digitalWrite(DEC_step, LOW); delayMicroseconds(interval);
      if (Serial1.available() != 0) {
        char command_temp = Serial1.read();
        if (command_temp == 'e') {
          if (stop_mode == 0) {
            if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
            break;
          }
        }
        if (command_temp == 'E') {
          if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
          break;
        }
      }
    }
  }




  else if (command_line == "BACK_CW") {
    digitalWrite(RA_dir, LOW);//RA_BACK
    digitalWrite(DEC_dir, HIGH);//DEC_CW
    microstepping_convert(joystick_gear);
    while (true) {
      digitalWrite(RA_step, HIGH); digitalWrite(DEC_step, HIGH);
      delayMicroseconds(interval);
      digitalWrite(RA_step, LOW); digitalWrite(DEC_step, LOW);
      delayMicroseconds(interval);
      if (Serial1.available() != 0) {
        char command_temp = Serial1.read();
        if (command_temp == 'e') {
          if (stop_mode == 0) {
            if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
            break;
          }
        }
        if (command_temp == 'E') {
          if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
          break;
        }
      }
    }
  }

  else if (command_line == "GO_CW") {
    digitalWrite(RA_dir, HIGH);//RA_GO
    digitalWrite(DEC_dir, HIGH);//DEC_CW
    microstepping_convert(joystick_gear);
    while (true) {
      digitalWrite(RA_step, HIGH); digitalWrite(DEC_step, HIGH);
      delayMicroseconds(interval);
      digitalWrite(RA_step, LOW); digitalWrite(DEC_step, LOW);
      delayMicroseconds(interval);
      if (Serial1.available() != 0) {
        char command_temp = Serial1.read();
        if (command_temp == 'e') {
          if (stop_mode == 0) {
            if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
            break;
          }
        }
        if (command_temp == 'E') {
          if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
          break;
        }
      }
    }
  }

  else if (command_line == "BACK_CCW") {
    digitalWrite(RA_dir, LOW);//RA_BACK
    digitalWrite(DEC_dir, LOW);//DEC_CCW
    microstepping_convert(joystick_gear);
    while (true) {
      digitalWrite(RA_step, HIGH); digitalWrite(DEC_step, HIGH);
      delayMicroseconds(interval);
      digitalWrite(RA_step, LOW); digitalWrite(DEC_step, LOW);
      delayMicroseconds(interval);
      if (Serial1.available() != 0) {
        char command_temp = Serial1.read();
        if (command_temp == 'e') {
          if (stop_mode == 0) {
            if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
            break;
          }
        }
        if (command_temp == 'E') {
          if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
          break;
        }
      }
    }
  }

  else if (command_line == "GO_CCW") {
    digitalWrite(RA_dir, HIGH);//RA_GO
    digitalWrite(DEC_dir, LOW);//DEC_CCW
    microstepping_convert(joystick_gear);
    while (true) {
      digitalWrite(RA_step, HIGH); digitalWrite(DEC_step, HIGH);
      delayMicroseconds(interval);
      digitalWrite(RA_step, LOW); digitalWrite(DEC_step, LOW);
      delayMicroseconds(interval);
      if (Serial1.available() != 0) {
        char command_temp = Serial1.read();
        if (command_temp == 'e') {
          if (stop_mode == 0) {
            if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
            break;
          }
        }
        if (command_temp == 'E') {
          if(tracking_available == 0) {Serial1.println("!!! JYT is NOT tracking !!!");}
          break;
        }
      }
    }
  }
  //--------------------



  //--------------------Microstepping convert
  if (command_line == "g0") {
    if (stop_mode == 1) {
      stop_mode = 0;
      Serial1.println("now free-stop mode");
    }
    else if (stop_mode == 0) {
      stop_mode = 1;
      Serial1.println("now non-stop mode");
    }
  }
  else if (command_line == "g1") {
    joystick_gear = 1;
    Serial1.println("1 microstepping");
  }
  else if (command_line == "g2") {
    joystick_gear = 2;
    Serial1.println("1/2 microstepping");
  }
  else if (command_line == "g4") {
    joystick_gear = 4;
    Serial1.println("1/4 microstepping");
  }
  else if (command_line == "g8") {
    joystick_gear = 8;
    Serial1.println("1/8 microstepping");
  }
  else if (command_line == "g16") {
    joystick_gear = 16;
    Serial1.println("1/16 microstepping");
  }
  else if (command_line == "g32") {
    joystick_gear = 32;
    Serial1.println("1/32 microstepping");
  }
  else if (command_line == "g64") {
    joystick_gear = 64;
    Serial1.println("1/64 microstepping");
  }
  else if (command_line == "g128") {
    joystick_gear = 128;
    Serial1.println("1/128 microstepping");
  }
  //--------------------


  //--------------------DHT recall
  if (command_line == "DHT?") {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float hic = dht.computeHeatIndex(t, h, false);
    Serial.print("Temperature : ");
    Serial.println(t);
    Serial.print("Humidity : ");
    Serial.println(h);
    Serial.print("Heat Index");
    Serial.println(hic);

    Serial1.print("Temperature : ");
    Serial1.println(t);
    Serial1.print("Humidity : ");
    Serial1.println(h);
    Serial1.print("Heat Index");
    Serial1.println(hic);
  }
  //--------------------







  command_line = "";

}
//1dan = 000 = full
//2dan = 110 = 1/8
//3dan = 010 = 1/4
void microstepping_convert(int object_micro) {
  if (object_micro == 1) {
    digitalWrite(RA_M0, LOW);
    digitalWrite(RA_M1, LOW);
    digitalWrite(RA_M2, LOW);
    digitalWrite(DEC_M0, LOW);
    digitalWrite(DEC_M1, LOW);
    digitalWrite(DEC_M2, LOW);
  }
  else if (object_micro == 2) {
    digitalWrite(RA_M0, HIGH);
    digitalWrite(RA_M1, LOW);
    digitalWrite(RA_M2, LOW);
    digitalWrite(DEC_M0, HIGH);
    digitalWrite(DEC_M1, LOW);
    digitalWrite(DEC_M2, LOW);
  }
  else if (object_micro == 4) {
    digitalWrite(RA_M0, LOW);
    digitalWrite(RA_M1, HIGH);
    digitalWrite(RA_M2, LOW);
    digitalWrite(DEC_M0, LOW);
    digitalWrite(DEC_M1, HIGH);
    digitalWrite(DEC_M2, LOW);
  }
  else if (object_micro == 8) {
    digitalWrite(RA_M0, HIGH);
    digitalWrite(RA_M1, HIGH);
    digitalWrite(RA_M2, LOW);
    digitalWrite(DEC_M0, HIGH);
    digitalWrite(DEC_M1, HIGH);
    digitalWrite(DEC_M2, LOW);
  }
  else if (object_micro == 16) {
    digitalWrite(RA_M0, LOW);
    digitalWrite(RA_M1, LOW);
    digitalWrite(RA_M2, HIGH);
    digitalWrite(DEC_M0, LOW);
    digitalWrite(DEC_M1, LOW);
    digitalWrite(DEC_M2, HIGH);
  }
  else if (object_micro == 32) {
    digitalWrite(RA_M0, HIGH);
    digitalWrite(RA_M1, LOW);
    digitalWrite(RA_M2, HIGH);
    digitalWrite(DEC_M0, HIGH);
    digitalWrite(DEC_M1, LOW);
    digitalWrite(DEC_M2, HIGH);
  }
  else if (object_micro == 64) {
    digitalWrite(RA_M0, LOW);
    digitalWrite(RA_M1, HIGH);
    digitalWrite(RA_M2, HIGH);
    digitalWrite(DEC_M0, LOW);
    digitalWrite(DEC_M1, HIGH);
    digitalWrite(DEC_M2, HIGH);
  }
  else if (object_micro == 128) {
    digitalWrite(RA_M0, HIGH);
    digitalWrite(RA_M1, HIGH);
    digitalWrite(RA_M2, HIGH);
    digitalWrite(DEC_M0, HIGH);
    digitalWrite(DEC_M1, HIGH);
    digitalWrite(DEC_M2, HIGH);
  }
  else {
    digitalWrite(RA_M0, LOW);
    digitalWrite(RA_M1, LOW);
    digitalWrite(RA_M2, LOW);
    digitalWrite(DEC_M0, LOW);
    digitalWrite(DEC_M1, LOW);
    digitalWrite(DEC_M2, LOW);
  }
}
