const int laserPin = 12;     // laser diode
const int greenLedPin = 11;  // unlock indicator led
const int redLedPin = 10;    // lock indicator led
const int buttonPin = 8;     // push button pin
const int receiverPin = 9;   // laser receiver

const int inputSequence[] = { 1, 0, 1, 1, 0, 0, 1, 1 };
const int expectedSequence[] = { 1, 0, 1, 1, 0, 0, 1, 1 };
const int dataLength = sizeof(inputSequence) / sizeof(inputSequence[0]);
int receivedBuffer[24];  // circular buffer
int bufferIndex = 0;

// timing constants
const unsigned long pulseWidth = 100;
const unsigned long bitInterval = 200;
const unsigned long readInterval = 20;
const unsigned long debounceTime = 50;

// state variables
unsigned long previousMillis = 0;
unsigned long lastReadTime = 0;
unsigned long lastButtonDebounce = 0;
int dataIndex = 0;
bool transmitting = false;
bool isUnlocked = false;
int lastButtonState = HIGH;
int buttonState = HIGH;

void setup() {
  Serial.begin(9600);
  pinMode(laserPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(receiverPin, INPUT);

  digitalWrite(laserPin, LOW);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, HIGH);
}

void startNewTransmission() {
  transmitting = true;
  dataIndex = 0;
  Serial.println("\n=== Starting transmission ===");
  previousMillis = millis();
}

void handleButton() {
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastButtonDebounce = millis();
  }

  if ((millis() - lastButtonDebounce) > debounceTime) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW && !transmitting) {
        startNewTransmission();
      }
    }
  }
  lastButtonState = reading;
}

void checkReceivedSequence() {
  receivedBuffer[bufferIndex] = digitalRead(receiverPin);
  bufferIndex = (bufferIndex + 1) % 24;

  for (int start = 0; start <= 24 - dataLength; start++) {
    bool matched = true;
    for (int i = 0; i < dataLength; i++) {
      if (receivedBuffer[(start + i) % 24] != expectedSequence[i]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      isUnlocked = !isUnlocked;
      digitalWrite(greenLedPin, isUnlocked);
      digitalWrite(redLedPin, !isUnlocked);
      Serial.println(isUnlocked ? "UNLOCKED" : "LOCKED");
      memset(receivedBuffer, 0, sizeof(receivedBuffer));
      bufferIndex = 0;
      break;
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();

  handleButton();

  // transmit
  if (transmitting) {
    if (dataIndex < dataLength) {
      unsigned long timeSinceBitStart = currentMillis - previousMillis;

      digitalWrite(
        laserPin,
        timeSinceBitStart < pulseWidth &&
        inputSequence[dataIndex] == 1 ? HIGH : LOW
      );

      if (timeSinceBitStart >= bitInterval) {
        previousMillis = currentMillis;
        dataIndex++;
      }
    } else {
      transmitting = false;
      digitalWrite(laserPin, LOW);
    }
  }

  // receive
  if (currentMillis - lastReadTime >= readInterval) {
    lastReadTime = currentMillis;
    checkReceivedSequence();
  }
}