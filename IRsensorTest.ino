#include <vehicle.h>
#include <IRremote.hpp>

#define IR_RECEIVE_PIN 4

vehicle myCar;

bool robotOn = false;

void setup() {

  Serial.begin(9600);

  myCar.Init();
  myCar.Move(Stop, 0);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
}

void loop() {

  if (IrReceiver.decode()) {

    uint32_t code = IrReceiver.decodedIRData.decodedRawData;
    Serial.println(code, HEX);

    // OK button toggle
    if (code == 0xBF40FF00) {
      robotOn = !robotOn;
    }

    IrReceiver.resume();
  }

  // movement logic
  if (robotOn) {
    myCar.Move(Backward, 80);
  } 
  else {
    myCar.Move(Stop, 0);
  }

}
