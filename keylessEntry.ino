//config
const int sensorPin = A0; 
const int ledPin = 13;
const int relayPin = 2;
const unsigned long readTimeout = 2000;
const unsigned long buzzTime = 5000;
// time spent up until we're certain (damn warble tone)
const unsigned long debounceUpThreshold = 0;
// time spent down until we're certain (account for warble tone!)
const unsigned long debounceDownThreshold = 200;

//state
boolean answerKey[] = {
  false, false, false, true,
  false, false, false, true
};

const int answerLength = 8;
unsigned long inputSequence[answerLength];
int sequencePos = -1; // -1 (not started reading)

unsigned long lastOnTime = 0;
unsigned long lastOffTime = 0;
boolean lastValue = false;


void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin, OUTPUT);  
  pinMode(relayPin, OUTPUT);  
  pinMode(sensorPin, INPUT);
  Serial.begin(9600);
  resetState();
}

void loop() {
  readSequence();
  unsigned long currentTime = millis();
  unsigned long timeSinceLastChange = currentTime - max(lastOffTime, lastOnTime);

  if (timeSinceLastChange > readTimeout) {
    if (sequencePos >= 0 && isEntryCorrect()) {
      Serial.println("Buzzing door");
      buzzDoor();
    }

    resetState();
  }
}

void readSequence() {
  boolean currentValue = readSignal();
  unsigned long currentTime = millis();

  //  if (currentValue && (currentTime - lastOffTime) < debounceThreshold || 
  //    !currentValue && (currentTime - lastOnTime) < debounceThreshold) {
  //      return;
  //    }

  if (currentValue != lastValue) {
    Serial.println(currentValue ? "ON" : "OFF");
    digitalWrite(ledPin, currentValue ? HIGH : LOW);
    if (currentValue) {
      // increment pos even if it goes beyond the array, this tells us if extraneous entries were made
      sequencePos+=1;
      lastOnTime = currentTime;
    } 
    else {
      if (sequencePos < answerLength) {
        inputSequence[sequencePos] = currentTime - lastOnTime;
      }
      lastOffTime = currentTime;
    }
  }

  lastValue = currentValue;
}

boolean isEntryCorrect() {
  if (sequencePos != answerLength - 1) {
    // too short or too long
    Serial.print("Answer wrong length. Was ");
    Serial.println(sequencePos + 1);

    return false;
  } 

  // we find the longest value that should be short,
  // and the shortest value that should be long
  // thereby biesting the values.

  // as long as all the shorts are shorter than all the longs, it's right.
  unsigned long longestShort = 0;
  unsigned long shortestLong = -1;//max

  int i;
  for (i = 0; i < answerLength; i++) {
    unsigned long entryLengthMillis = inputSequence[i];
    if (answerKey[i]) {
      //long
      if (entryLengthMillis < shortestLong) {
        shortestLong = entryLengthMillis;
      }
    } 
    else {
      //short
      if (entryLengthMillis > longestShort) {
        longestShort = entryLengthMillis;
      }
    }
  }
  return longestShort < shortestLong;
}

void buzzDoor() {
  digitalWrite(relayPin, HIGH); 
  delay(buzzTime);
  digitalWrite(relayPin, LOW);
}

// debouncer state
unsigned long _lastChangeTime = 0;
boolean _lastCertainValue = false;
boolean _lastReadValue = false;

// Read signal with debouncing
boolean readSignal() {
  boolean currentValue = digitalRead(sensorPin) == HIGH;


  if (currentValue != _lastReadValue) {
    _lastChangeTime = millis();
    _lastReadValue = currentValue;
  }

  unsigned long timeSinceLastChange = millis() - _lastChangeTime;

  if (currentValue && timeSinceLastChange > debounceUpThreshold ||
    !currentValue && timeSinceLastChange > debounceDownThreshold) {

    _lastCertainValue = currentValue;

  }

  return _lastCertainValue;
}

void resetState() {
  if (sequencePos == -1) {
    return;
  }
  Serial.println("resetting");

  int i;
  for (i = 0; i < answerLength; i++) {
    inputSequence[i] = 0;
  }
  sequencePos = -1;
}




