// Include standard library headers when libc is linked
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void setup() {
  Serial.println("Hello from SRAM");
  pinMode(LED_BUILTIN, OUTPUT);

  // Test malloc/free
  Serial.println("\n=== Testing malloc/free ===");
  void* ptr1 = malloc(100);
  void* ptr2 = malloc(50);
  
  if (ptr1 == nullptr || ptr2 == nullptr) {
    Serial.println("ERROR: malloc returned NULL!");
    return;
  }
  
  Serial.println("Memory allocated successfully");
  free(ptr1);
  free(ptr2);
  Serial.println("Memory freed");

  // Test string functions
  Serial.println("\n=== Testing string functions ===");
  char* str = reinterpret_cast<char*>(malloc(64));
  strcpy(str, "Hello");
  strcat(str, " World!");
  Serial.print("String: ");
  Serial.println(str);
  char* lenBuf = reinterpret_cast<char*>(malloc(16));
  sprintf(lenBuf, "Length: %u", static_cast<unsigned>(strlen(str)));
  Serial.println(lenBuf);
  free(lenBuf);
  free(str);

  // Test sprintf
  Serial.println("\n=== Testing sprintf ===");
  char* buf = reinterpret_cast<char*>(malloc(128));
  int val = 42;
  sprintf(buf, "Value: %d", val);
  Serial.println(buf);
  sprintf(buf, "Hex: 0x%x, Octal: %o", val, val);
  Serial.println(buf);
  free(buf);

  // Test math functions
  Serial.println("\n=== Testing math functions ===");
  double sinVal = sin(1.0);
  double sqrtVal = sqrt(16.0);
  double powVal = pow(2.0, 3.0);
  char* mathBuf = reinterpret_cast<char*>(malloc(64));
  sprintf(mathBuf, "sin(1.0)=%d, sqrt(16)=%d, pow(2,3)=%d", 
          static_cast<int>(sinVal * 1000),
          static_cast<int>(sqrtVal),
          static_cast<int>(powVal));
  Serial.println(mathBuf);
  free(mathBuf);

  // Test memset/memcpy
  Serial.println("\n=== Testing memory functions ===");
  char* mem1 = reinterpret_cast<char*>(malloc(32));
  char* mem2 = reinterpret_cast<char*>(malloc(32));
  memset(mem1, 0xAA, 32);
  memcpy(mem2, mem1, 32);
  int cmpResult = memcmp(mem1, mem2, 32);
  char* resultMsg = reinterpret_cast<char*>(malloc(16));
  sprintf(resultMsg, "memcmp result: %d", cmpResult);
  Serial.println(resultMsg);
  if (cmpResult == 0) {
    Serial.println("memcpy test: OK");
  } else {
    Serial.println("memcpy test: FAIL");
  }
  free(mem1);
  free(mem2);
  free(resultMsg);

  Serial.println("\n=== Libc tests complete ===");

  // Test new syscalls
  Serial.println("\n=== Testing new syscalls ===");
  
  // Test timing functions
  Serial.println("Testing timing functions...");
  uint32_t startMillis = millis();
  uint32_t startMicros = micros();
  delay(10);
  delayMicroseconds(100);
  uint32_t elapsedMillis = millis() - startMillis;
  uint32_t elapsedMicros = micros() - startMicros;
  char* timeBuf = reinterpret_cast<char*>(malloc(64));
  sprintf(timeBuf, "delay(10) elapsed: %lu ms, delayMicroseconds(100) elapsed: %lu us", 
          elapsedMillis, elapsedMicros);
  Serial.println(timeBuf);
  free(timeBuf);

  // Test analog I/O
  Serial.println("\nTesting analog I/O...");
  int analogVal = analogRead(A0);
  analogWrite(LED_BUILTIN, 128);  // PWM output
  delay(100);
  analogWrite(LED_BUILTIN, 0);
  char* analogBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(analogBuf, "analogRead(A0): %d", analogVal);
  Serial.println(analogBuf);
  free(analogBuf);

  // Test additional Serial methods
  Serial.println("\nTesting Serial methods...");
  Serial.flush();
  Serial.println("Testing println() with no args");
  int avail = Serial.available();
  char* serialBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(serialBuf, "Serial.available(): %d", avail);
  Serial.println(serialBuf);
  free(serialBuf);

  // Test SPI
  Serial.println("\nTesting SPI...");
  SPI.begin();
  SPI.beginTransaction();
  uint8_t txData = 0xAA;
  uint8_t rxData = SPI.transfer(txData);
  SPI.endTransaction();
  SPI.end();
  char* spiBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(spiBuf, "SPI.transfer(0x%02X) = 0x%02X", txData, rxData);
  Serial.println(spiBuf);
  free(spiBuf);

  // Test Wire/I2C
  Serial.println("\nTesting Wire/I2C...");
  Wire.begin();
  Wire.beginTransmission(0x50);
  Wire.write(0x00);
  uint8_t txStatus = Wire.endTransmission();
  Wire.end();
  char* wireBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(wireBuf, "Wire.endTransmission: %d", txStatus);
  Serial.println(wireBuf);
  free(wireBuf);

  Serial.println("\n=== All syscall tests complete ===");
}

void loop() {
  static uint32_t counter = 0;
  counter++;

  digitalWrite(LED_BUILTIN, HIGH);
  delay(150);

  // Periodically print counter using libc
  if (counter % 10 == 0) {
    char* msg = reinterpret_cast<char*>(malloc(32));
    sprintf(msg, "Loop #%lu", static_cast<unsigned long>(counter));
    Serial.println(msg);
    free(msg);
  }

  digitalWrite(LED_BUILTIN, LOW);
  delay(150);
}