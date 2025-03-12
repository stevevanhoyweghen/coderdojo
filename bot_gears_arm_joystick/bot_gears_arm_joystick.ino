/*
Steve Van Hoyweghen

2025-03-05: Eerste versie
2025-03-12: Documentatie aangepast

Introductie
~~~~~~~~~~~
Een programma om met een Arduino Nano en een joystick een educatieve robotarm aan te sturen.
Je kan de robotarm links-rechts en voor-achter aansturen met behulp van een joystick.
Door op de joystick te drukken kan je afwisselend de klauw openen en sluiten.

Ik heb de robotarm gebouwd en de software geschreven als https://coderdojobelgium.be/nl/
vrijwilliger om te gebruiken tijdens de Arduino sessies.

Didactische opmerkingen steeds welkom.

3D Print
~~~~~~~~
De robot arm onderdelen zijn 3D geprint met PLA.

Referenties:
- https://projecthub.arduino.cc/bzqp/super-bot-gears-a-3d-printed-arduino-starter-kit-6d6613
- https://www.hackster.io/bzqp/super-bot-gears-a-3d-printed-arduino-starter-kit-91485e
- https://www.youtube.com/watch?v=yBgqDcDxOuM
- https://www.thingiverse.com/thing:4176199/files
- https://www.printables.com/model/125076-super-bot-gears-a-3d-printed-arduino-starter-kit/files

Opmerkingen:
- Enkele onderdelen geven problemen bij het genereren van de GCODE. Opgelost met deze online tool https://www.formware.co/onlinestlrepair.
- De geprinte onderdelen zouden in elkaar moeten klemmen, maar dat deden ze niet. Ik gebruikte een gekalibreerde Bambu Lab A1.
  Ik heb de geprinte PLA-onderdelen gelijmd met secondelijm en dat werkt prima.
- Je hoeft niet alle onderdelen te printen als je enkel de robot arm wil bouwen.

Schakeling
~~~~~~~~~~
JOYSTICK        ARDUINO NANO
GND             GND
V5+             +5V
VRx             A5
VRy             A6
SW              D2

SERVO X         ARDUINO NANO
Bruine draad    GND
Rode draad      +5V
Gele draad      D9

SERVO Y         ARDUINO NANO
Bruine draad    GND
Rode draad      +5V
Gele draad      D10

SERVO KLAUW     ARDUINO NANO
Bruine draad    GND
Rode draad      +5V
Gele draad      D11

Opmerkingen:
- Alles werkt op één USB 5V-voeding van 1 Ampère.
- Om de hoogfrequente spanningspieken veroorzaakt door de inductieve belasting van de servo motoren op te vangen
  is er tussen +5V en GND een 100nF keramische condensator geplaatst. Plaats deze zo dicht mogelijk
  bij de +5V en GND pinnen van de Arduino Nano om deze te beschermen.
- Tevens is er een elektrolytische condensator van 470 uF/35V in parallel geplaatst om de
  spanningsveranderingen veroorzaakt door de servomotoren te dempen. Let op de polariteit!
- De gebruikte waarden voor de condensatoren zijn niet zo kritisch.
- Je kan ook een afzonderlijk 5V-voeding voor de servo motoren voorzien.

Software
~~~~~~~~
- Het gebruik van de Arduino map functie kan wat toelichting vergen:
  https://reference.arduino.cc/reference/en/language/functions/math/map/

Software licentie
~~~~~~~~~~~~~~~~~
Deze software is rechtenvrij en je kan deze naar believen gebruiken en aanpassen. Voor alle duidelijkheid ...

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org/>
*/

#include <Servo.h>

// De gebruikte ingangen en uitgangen van de Arduino.
#define JOYSTICK_X_IN A5
#define JOYSTICK_Y_IN A6
#define JOYSTICK_DRUKKNOP_IN 2

#define SERVO_X_UIT 9
#define SERVO_Y_UIT 10
#define SERVO_KLAUW_UIT 11

// Minima en maxima voor de servo motoren.
#define SERVO_X_MIN 0
#define SERVO_X_MAX 180
#define SERVO_Y_MIN 100
#define SERVO_Y_MAX 160
#define SERVO_KLAUW_OPEN 180
#define SERVO_KLAUW_DICHT 120

// De waarden gebruikt, los van de servo motor details.
#define ARM_DRAAI_MIN 0
#define ARM_DRAAI_MIDDEN 90
#define ARM_DRAAI_MAX 180

#define ARM_VOOR 0
#define ARM_ACHTER 100

#define KLAUW_DICHT 0
#define KLAUW_OPEN 100

// De vertragingen nodig voor een vloeiende beweging van de servomotoren.
#define LOOP_VERTRAGING 100
#define KLAUW_VERTRAGING 500

// Globale variabelen om de toestand van de robot arm weer te geven.
int klauw, armDraai, armVoorAchter;

Servo servoX;
Servo servoY;
Servo servoKlauw;

// Draai de robot arm.
void beweegArmDraai(int waarde) {
  waarde = map(waarde, 0, 180, SERVO_X_MIN, SERVO_X_MAX);
  servoY.write(waarde);
}

// Beweeg de robot arm naar voor of naar achter.
void beweegArmVoorAchter(int waarde) {
  waarde = map(waarde, 0, 100, SERVO_Y_MIN, SERVO_Y_MAX);
  servoX.write(waarde);
}

// Open of sluit de klauw.
void beweegOpenSluitKlauw(int waarde) {
  waarde = map(waarde, 0, 100, SERVO_KLAUW_OPEN, SERVO_KLAUW_DICHT);
  servoKlauw.write(waarde);
}

// Lees de joystick x of y waarden. Deze gaan van 0 tot 1023, inclusief.
// Map deze waarden naar geschikte waarden om de servos aan te sturen.
int joystickStap(int ingang) {
  return map((analogRead(ingang) - 512), -512, 512, -3, 4);
}

// Begrens een integer waarde tussen minimum en maximum, inclusief deze waarden.
int begrens(int waarde, int minimum, int maximum) {
  if (waarde < minimum)
    waarde = minimum;
  else if (waarde > maximum)
    waarde = maximum;
  return waarde;
}

// Als de drukknop ingedrukt wordt, dan wordt de klauw geopend of gesloten.
// We maken hier gebruik van een pointer die toegang geeft tot de globale variabele.
void behandelDrukknop(int *klauw) {
  // Als de drukknop is ingedrukt lezen we een lage waarde
  if (digitalRead(JOYSTICK_DRUKKNOP_IN) == LOW) {
    // Telkens de drukknop wordt ingedrukt wisselen we van toestand.
    // Dus van open naar dicht, of van dicht naar open.
    if (*klauw == KLAUW_OPEN)
      *klauw = KLAUW_DICHT;
    else
      *klauw = KLAUW_OPEN;
    delay(KLAUW_VERTRAGING);
  }
}

// Toon de waarden die de toestand van de robot arm weergeven.
void printInfo(int armDraai, int armVoorAchter, int klauw) {
  Serial.print("armDraai=");
  Serial.print(armDraai);
  Serial.print(" armVoorAchter=");
  Serial.print(armVoorAchter);
  Serial.print(" klauw=");
  Serial.println(klauw);
}

// Maak alles klaar om de robot arm te kunnen gebruiken.
void setup() {
  // Stel de baud snelheid in voor de seriële monitor.
  Serial.begin(9600);

  // Stel de ingang in voor de joystick drukknop om de klauw te bedienen.
  pinMode(JOYSTICK_DRUKKNOP_IN, INPUT_PULLUP);

  // Maak de 3 servo motoren klaar voor gebruik.
  servoX.attach(SERVO_X_UIT);
  servoY.attach(SERVO_Y_UIT);
  servoKlauw.attach(SERVO_KLAUW_UIT);

  // Stel de startpositie van de robot arm in.
  armDraai = ARM_DRAAI_MIDDEN;
  armVoorAchter = ARM_ACHTER;
  klauw = KLAUW_DICHT;

  // Toon dat de robot arm kan gebruikt worden.
  Serial.println();
  Serial.println("Robot arm klaar voor gebruik.");
}

// Hier wordt het eigenlijk werk gedaan in een steeds herhaalde lus.
void loop() {
  // Lees de X en Y waarden in van de joystick en begrens deze binnen de gewenste maxima en minima.
  armDraai = begrens(armDraai + joystickStap(JOYSTICK_X_IN), ARM_DRAAI_MIN, ARM_DRAAI_MAX);
  armVoorAchter = begrens(armVoorAchter + joystickStap(JOYSTICK_Y_IN), ARM_VOOR, ARM_ACHTER);

  // Lees de joystick drukknop en vertaal dit in een waarde om de klauw aan te sturen.
  behandelDrukknop(&klauw);

  // Stuur de servomotoren aan met de berekende waarden.
  beweegArmDraai(armDraai);
  beweegArmVoorAchter(armVoorAchter);
  beweegOpenSluitKlauw(klauw);

  // Toon informatie op de seriële monitor.
  printInfo(armDraai, armVoorAchter, klauw);

  // Vertraag een beetje zodat de servomotoren kunnen volgen.
  delay(LOOP_VERTRAGING);
}
