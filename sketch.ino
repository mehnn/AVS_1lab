/*
  Ultrasonic Parking Assistant - four warning levels
  Board: Arduino Uno (ATmega328P)
  Sensor: HC-SR04, buzzer: piezo

  Based on the idea: https://github.com/lazycustard/Distance-measurement-using-ultrasonic-sensor
  Improvement: distance-dependent sound pattern instead of one <5 cm alarm.

  IMPORTANT: This version does NOT use NewPing. On Arduino Uno,
  NewPing and tone() can conflict over Timer2 interrupts.
*/

const byte TRIGGER_PIN = 11;
const byte ECHO_PIN = 12;
const byte BUZZER_PIN = 3;
const unsigned int MAX_DISTANCE_CM = 400;
const unsigned long MEASURE_INTERVAL_MS = 100;
const unsigned long ECHO_TIMEOUT_US = 25000UL;

enum AlarmLevel { SAFE, SLOW, FAST, STOP };
AlarmLevel currentLevel = SAFE;
unsigned long lastMeasurementMs = 0;
unsigned long beepCycleStartMs = 0;
bool firstMeasurement = true;
bool buzzerIsOn = false;

// Built-in Arduino functions replace NewPing to avoid Timer2 conflict.
// Returns 0 when no valid echo arrives.
unsigned int readDistanceCm() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  unsigned long echoTimeUs = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (echoTimeUs == 0) return 0;

  unsigned int distanceCm = echoTimeUs / 58UL;
  if (distanceCm == 0 || distanceCm > MAX_DISTANCE_CM) return 0;
  return distanceCm;
}

AlarmLevel chooseLevel(unsigned int distanceCm) {
  if (distanceCm == 0 || distanceCm >= 30) return SAFE;
  if (distanceCm >= 10) return SLOW;
  if (distanceCm >= 5) return FAST;
  return STOP;
}

const char* levelName(AlarmLevel level) {
  switch (level) {
    case SLOW: return "SLOW";
    case FAST: return "FAST";
    case STOP: return "STOP";
    default:   return "SAFE";
  }
}

void setBuzzer(bool enabled) {
  if (enabled == buzzerIsOn) return;
  buzzerIsOn = enabled;
  if (enabled) {
    tone(BUZZER_PIN, 1800);
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    noTone(BUZZER_PIN);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void updateSound(unsigned long nowMs) {
  if (currentLevel == SAFE) {
    setBuzzer(false);
  } else if (currentLevel == STOP) {
    setBuzzer(true);
  } else {
    const unsigned long periodMs = (currentLevel == SLOW) ? 900UL : 300UL;
    const unsigned long onTimeMs = (currentLevel == SLOW) ? 170UL : 120UL;
    unsigned long elapsedMs = (nowMs - beepCycleStartMs) % periodMs;
    setBuzzer(elapsedMs < onTimeMs);
  }
}

void setup() {
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);
  Serial.begin(9600);
  Serial.println("PARKING SENSOR READY");
}

void loop() {
  unsigned long nowMs = millis();

  if (firstMeasurement || nowMs - lastMeasurementMs >= MEASURE_INTERVAL_MS) {
    firstMeasurement = false;
    lastMeasurementMs = nowMs;
    unsigned int distanceCm = readDistanceCm();
    AlarmLevel newLevel = chooseLevel(distanceCm);

    if (newLevel != currentLevel) {
      currentLevel = newLevel;
      beepCycleStartMs = millis();
      setBuzzer(false);
    }

    Serial.print("Distance: ");
    if (distanceCm == 0) {
      Serial.print("no echo");
    } else {
      Serial.print(distanceCm);
      Serial.print(" cm");
    }
    Serial.print(" | Status: ");
    Serial.println(levelName(currentLevel));
  }

  updateSound(millis());
}
