// basic_serial
//
// Opens the serial port at 115200 baud and prints an incrementing counter
// as "count X" every 2 seconds.

unsigned long lastSendMs = 0;
const unsigned long sendIntervalMs = 2000;
unsigned int count = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  unsigned long now = millis();
  if (now - lastSendMs >= sendIntervalMs) {
    lastSendMs = now;
    Serial.print("count ");
    Serial.println(count);
    count++;
  }
}
