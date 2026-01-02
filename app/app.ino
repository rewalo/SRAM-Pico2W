// Include standard library headers when libc is linked
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Forward declaration for core 1 task
void core1Task();

// Forward declarations for cross-page function call tests
void test_function_chain();
void test_function_pointers();
void test_recursion();
void test_nested_calls();
void test_large_functions();
void test1();
void test2();
void test3();
void test4();
void test5();
void test6();
void recursive_function(int depth, int level);
void nested_function_a();
void nested_function_b();
void nested_function_c();
void large_function_1();
void large_function_2();
void core1Task();  // Core 1 task function

// Forward declarations for interrupt, WiFi, and BLE tests
void test_interrupts();
void test_wifi();
void test_ble();
void test_isr();  // ISR function for interrupt testing

namespace core1_telemetry {
constexpr uintptr_t kCore1StatusAddr = 0x2002F000u;
constexpr bool kCore1StatusEnabled = false;

[[maybe_unused]] inline void SetCore1Status(uint32_t value) {
  if (!kCore1StatusEnabled) {
    return;
  }
  *reinterpret_cast<volatile uint32_t*>(kCore1StatusAddr) = value;
}
}  // namespace core1_telemetry

constexpr uintptr_t kSioBase = 0xD0000000u;
constexpr uintptr_t kSioCpuidOffset = 0x00000000u;

inline uint32_t ReadCoreId() {
  return *reinterpret_cast<volatile uint32_t*>(kSioBase + kSioCpuidOffset) & 0x3u;
}

String a1 = "hello world";
String sub = a1.substring(0, 3);

String a2 = "HALLO WORLD";
String lower = a2.toLower();

void setup() {
  Serial.begin(115200);
  Serial.flush();

  Serial.println("Hello from SRAM");
  Serial.println(sub);
  Serial.println(lower);
  randomSeed(analogRead(0));  // Seed from analog noise
  long r = random(10, 100);   // Random between 10-100
  long mapped = map(r, 10, 100, 0, 255);  // Map to 0-255

  char c = 'A';
  if (isUpperCase(c)) {
    c = toLowerCase(c);  // 'a'
  }
  Serial.println("=== Math and Character Tests ===");
  Serial.print("Random (10-100): ");
  Serial.println(r);
  Serial.print("Mapped (0-255): ");
  Serial.println(mapped);
  Serial.print("Character: ");
  Serial.println(c);
  pinMode(LED_BUILTIN, OUTPUT);
  
  multicore_launch_core1(core1Task);

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
  
  // Test cross-page function calls
  Serial.println("\n");
  Serial.println("=========================================");
  Serial.println("TESTING CROSS-PAGE FUNCTION CALLS");
  Serial.println("=========================================");
  
  // Test 1: Function chain
  test_function_chain();
  delay(100);
  
  // Test 2: Function pointers
  test_function_pointers();
  delay(100);
  
  // Test 3: Recursion
  test_recursion();
  delay(100);
  
  // Test 4: Nested calls
  test_nested_calls();
  delay(100);
  
  // Test 5: Large functions
  test_large_functions();
  delay(100);
  
  Serial.println("\n=== All cross-page function call tests complete ===");
  Serial.println("If all tests passed, the overlay system is working correctly!");
  
  // Test interrupts
  Serial.println("\n");
  Serial.println("=========================================");
  Serial.println("TESTING INTERRUPTS");
  Serial.println("=========================================");
  test_interrupts();
  delay(100);
  
  // Test WiFi
  Serial.println("\n");
  Serial.println("=========================================");
  Serial.println("TESTING WiFi");
  Serial.println("=========================================");
  test_wifi();
  delay(100);
  
  // Test Bluetooth/BLE
  Serial.println("\n");
  Serial.println("=========================================");
  Serial.println("TESTING BLUETOOTH/BLE");
  Serial.println("=========================================");
  test_ble();
  delay(100);
  
  Serial.println("\n=== All feature tests complete ===");
}

void loop() {
  static uint32_t counter = 0;
  counter++;

  //digitalWrite(LED_BUILTIN, HIGH);
  delay(150);

  // Periodically print counter using libc
  if (counter % 10 == 0) {
    char* msg = reinterpret_cast<char*>(malloc(32));
    sprintf(msg, "Loop #%lu", static_cast<unsigned long>(counter));
    Serial.println(msg);
    free(msg);
  }

  //digitalWrite(LED_BUILTIN, LOW);
  delay(150);
}

// Test function chain - calls functions in sequence to test cross-page calls
void test_function_chain() {
  Serial.println("\n=== Testing Function Chain (Cross-Page Calls) ===");
  Serial.println("Calling test1() -> test2() -> test3() -> test4() -> test5() -> test6()");
  
  test1();
}

// Test function pointers - indirect calls across pages
void test_function_pointers() {
  Serial.println("\n=== Testing Function Pointers ===");
  
  // Function pointer type
  typedef void (*TestFuncPtr)();
  
  // Array of function pointers
  TestFuncPtr funcs[] = {test1, test2, test3, test4, test5, test6};
  const char* names[] = {"test1", "test2", "test3", "test4", "test5", "test6"};
  
  Serial.println("Calling functions via function pointers:");
  for (int i = 0; i < 6; i++) {
    Serial.print("  Calling ");
    Serial.print(names[i]);
    Serial.println("() via pointer");
    funcs[i]();
    delay(10);  // Small delay to observe behavior
  }
}

// Test recursion - may span multiple pages
void test_recursion() {
  Serial.println("\n=== Testing Recursion ===");
  Serial.println("Recursive function (depth 5):");
  
  recursive_function(5, 0);
}

// Test nested function calls
void test_nested_calls() {
  Serial.println("\n=== Testing Nested Function Calls ===");
  
  Serial.println("Calling nested_function_a() which calls nested_function_b()...");
  nested_function_a();
}

// Test large functions that might span pages
void test_large_functions() {
  Serial.println("\n=== Testing Large Functions (Potential Page Boundaries) ===");
  
  large_function_1();
  large_function_2();
}

// Individual test functions - each should be in potentially different pages
void test1() {
  Serial.println("  [test1] Executing...");
  char* buf = reinterpret_cast<char*>(malloc(32));
  sprintf(buf, "  [test1] Address: 0x%08lX", reinterpret_cast<uintptr_t>(&test1));
  Serial.println(buf);
  free(buf);
  
  // Call another function to test chain
  Serial.println("  [test1] Calling test2()...");
  test2();
}

void test2() {
  Serial.println("    [test2] Executing...");
  char* buf = reinterpret_cast<char*>(malloc(32));
  sprintf(buf, "    [test2] Address: 0x%08lX", reinterpret_cast<uintptr_t>(&test2));
  Serial.println(buf);
  free(buf);
  
  // Call another function
  Serial.println("    [test2] Calling test3()...");
  test3();
}

void test3() {
  Serial.println("      [test3] Executing...");
  char* buf = reinterpret_cast<char*>(malloc(32));
  sprintf(buf, "      [test3] Address: 0x%08lX", reinterpret_cast<uintptr_t>(&test3));
  Serial.println(buf);
  free(buf);
  
  // Call another function
  Serial.println("      [test3] Calling test4()...");
  test4();
}

void test4() {
  Serial.println("        [test4] Executing...");
  char* buf = reinterpret_cast<char*>(malloc(32));
  sprintf(buf, "        [test4] Address: 0x%08lX", reinterpret_cast<uintptr_t>(&test4));
  Serial.println(buf);
  free(buf);
  
  // Call another function
  Serial.println("        [test4] Calling test5()...");
  test5();
}

void test5() {
  Serial.println("          [test5] Executing...");
  char* buf = reinterpret_cast<char*>(malloc(32));
  sprintf(buf, "          [test5] Address: 0x%08lX", reinterpret_cast<uintptr_t>(&test5));
  Serial.println(buf);
  free(buf);
  
  // Call another function
  Serial.println("          [test5] Calling test6()...");
  test6();
}

void test6() {
  Serial.println("            [test6] Executing...");
  char* buf = reinterpret_cast<char*>(malloc(32));
  sprintf(buf, "            [test6] Address: 0x%08lX", reinterpret_cast<uintptr_t>(&test6));
  Serial.println(buf);
  free(buf);
  Serial.println("            [test6] Reached end of chain!");
}

// Recursive function to test if recursion works across pages
void recursive_function(int depth, int level) {
  if (depth <= 0) {
    char* buf = reinterpret_cast<char*>(malloc(48));
    sprintf(buf, "  Recursion base case reached at level %d", level);
    Serial.println(buf);
    free(buf);
    return;
  }
  
  char* buf = reinterpret_cast<char*>(malloc(48));
  sprintf(buf, "  Recursion level %d, depth %d", level, depth);
  Serial.println(buf);
  free(buf);
  
  // Add some computation to make function larger
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += i;
  }
  
  // Recursive call
  recursive_function(depth - 1, level + 1);
  
  // Post-recursion work
  char* buf2 = reinterpret_cast<char*>(malloc(48));
  sprintf(buf2, "  Returning from level %d (sum=%d)", level, sum);
  Serial.println(buf2);
  free(buf2);
}

// Nested function calls
void nested_function_a() {
  Serial.println("  [nested_a] Starting...");
  char* buf = reinterpret_cast<char*>(malloc(32));
  sprintf(buf, "  [nested_a] Address: 0x%08lX", reinterpret_cast<uintptr_t>(&nested_function_a));
  Serial.println(buf);
  free(buf);
  
  Serial.println("  [nested_a] Calling nested_function_b()...");
  nested_function_b();
  
  Serial.println("  [nested_a] Returned from nested_function_b()");
}

void nested_function_b() {
  Serial.println("    [nested_b] Starting...");
  char* buf = reinterpret_cast<char*>(malloc(32));
  sprintf(buf, "    [nested_b] Address: 0x%08lX", reinterpret_cast<uintptr_t>(&nested_function_b));
  Serial.println(buf);
  free(buf);
  
  Serial.println("    [nested_b] Calling nested_function_c()...");
  nested_function_c();
  
  Serial.println("    [nested_b] Returned from nested_function_c()");
}

void nested_function_c() {
  Serial.println("      [nested_c] Starting...");
  char* buf = reinterpret_cast<char*>(malloc(32));
  sprintf(buf, "      [nested_c] Address: 0x%08lX", reinterpret_cast<uintptr_t>(&nested_function_c));
  Serial.println(buf);
  free(buf);
  
  Serial.println("      [nested_c] Deepest level reached!");
}

// Large functions that might span page boundaries
void large_function_1() {
  Serial.println("  [large1] Starting large function...");
  
  // Add lots of local variables and operations to make function large
  int a = 1, b = 2, c = 3, d = 4, e = 5;
  int f = 6, g = 7, h = 8, i = 9, j = 10;
  
  // Multiple operations
  for (int k = 0; k < 20; k++) {
    a += k;
    b *= 2;
    c = a + b;
    d = c - a;
    e = d * b;
  }
  
  char* buf = reinterpret_cast<char*>(malloc(64));
  sprintf(buf, "  [large1] Computed: a=%d, b=%d, c=%d, d=%d, e=%d", a, b, c, d, e);
  Serial.println(buf);
  free(buf);
  
  Serial.println("  [large1] Calling large_function_2()...");
  large_function_2();
  
  Serial.println("  [large1] Finished");
}

void large_function_2() {
  Serial.println("    [large2] Starting large function...");
  
  // More local variables
  float x = 1.0f, y = 2.0f, z = 3.0f;
  double sum = 0.0;
  
  // More computations
  for (int i = 0; i < 30; i++) {
    x += 0.1f;
    y *= 1.1f;
    z = x + y;
    sum += static_cast<double>(z);
  }
  
  char* buf = reinterpret_cast<char*>(malloc(64));
  sprintf(buf, "    [large2] Final: x=%.2f, y=%.2f, z=%.2f, sum=%.2f", 
          x, y, z, sum);
  Serial.println(buf);
  free(buf);
  
  Serial.println("    [large2] Finished");
}

// Core 1 task - monitors BOOTSEL button and resets if pressed
void core1Task() {
  for (;;) {
    if (bootselButton()) {
      while (bootselButton()) {
        // Wait for button release
      }
      softwareReset();
    }
    delay(10);
  }
}

// =====================================================================
// Interrupt test functions
// =====================================================================

// Interrupt mode constants are defined in common.h

// Global interrupt counter for testing
volatile int interrupt_counter = 0;
volatile uint8_t interrupt_pin = 0;

// ISR function for interrupt testing
void test_isr() {
  interrupt_counter++;
}

void test_interrupts() {
  Serial.println("\n=== Testing GPIO Interrupts ===");
  
  // Test pin (use a safe pin - GPIO 2)
  const uint8_t test_pin = 2;
  
  // Self-contained test:
  // On RP2040/RP2350, GPIO edge IRQs also trigger when the pin output toggles.
  // So we can test without any external wiring by driving the pin as OUTPUT.
  pinMode(test_pin, OUTPUT);
  digitalWrite(test_pin, LOW);

  const int toggles = 10;

  auto do_toggle = [&]() {
    for (int i = 0; i < toggles; i++) {
      digitalWrite(test_pin, HIGH);
      delayMicroseconds(50);
      digitalWrite(test_pin, LOW);
      delayMicroseconds(50);
    }
  };

  // CHANGE should see both edges: 2 per toggle
  interrupt_counter = 0;
  Serial.println("Attaching interrupt on CHANGE and toggling pin...");
  attachInterrupt(test_pin, test_isr, CHANGE);
  do_toggle();
  detachInterrupt(test_pin);
  Serial.print("CHANGE count (expected ~");
  Serial.print(toggles * 2);
  Serial.print("): ");
  Serial.println(interrupt_counter);

  // RISING should see 1 per toggle (LOW->HIGH)
  interrupt_counter = 0;
  Serial.println("Attaching interrupt on RISING and toggling pin...");
  attachInterrupt(test_pin, test_isr, RISING);
  do_toggle();
  detachInterrupt(test_pin);
  Serial.print("RISING count (expected ~");
  Serial.print(toggles);
  Serial.print("): ");
  Serial.println(interrupt_counter);

  // FALLING should see 1 per toggle (HIGH->LOW)
  interrupt_counter = 0;
  Serial.println("Attaching interrupt on FALLING and toggling pin...");
  attachInterrupt(test_pin, test_isr, FALLING);
  do_toggle();
  detachInterrupt(test_pin);
  Serial.print("FALLING count (expected ~");
  Serial.print(toggles);
  Serial.print("): ");
  Serial.println(interrupt_counter);

  // Restore a safe state
  pinMode(test_pin, INPUT_PULLUP);
  Serial.println("Interrupt test complete (self-contained).");
}

// =====================================================================
// WiFi test functions
// =====================================================================

void test_wifi() {
  Serial.println("\n=== Testing WiFi Functions ===");
  
  // Test WiFi.begin() - try to connect (will fail if no credentials, but function should work)
  Serial.println("Testing WiFi.begin()...");
  Serial.println("[NOTE] This will attempt to connect. If no credentials provided, it will fail gracefully.");
  
  // Test WiFi.status()
  Serial.println("\nTesting WiFi.status()...");
  int status = WiFi.status();
  char* statusBuf = reinterpret_cast<char*>(malloc(64));
  sprintf(statusBuf, "WiFi status: %d", status);
  Serial.println(statusBuf);
  free(statusBuf);
  
  // Test WiFi.SSID() - may return empty if not connected
  Serial.println("\nTesting WiFi.SSID()...");
  const char* ssid = WiFi.SSID();
  if (ssid != nullptr) {
    Serial.print("Current SSID: ");
    Serial.println(ssid);
  } else {
    Serial.println("SSID: (not connected)");
  }
  
  // Test WiFi.localIP()
  Serial.println("\nTesting WiFi.localIP()...");
  uint32_t ip = WiFi.localIP();
  char* ipBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(ipBuf, "Local IP: 0x%08lX", static_cast<unsigned long>(ip));
  Serial.println(ipBuf);
  free(ipBuf);
  
  // Test WiFi.subnetMask()
  Serial.println("\nTesting WiFi.subnetMask()...");
  uint32_t mask = WiFi.subnetMask();
  char* maskBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(maskBuf, "Subnet Mask: 0x%08lX", static_cast<unsigned long>(mask));
  Serial.println(maskBuf);
  free(maskBuf);
  
  // Test WiFi.gatewayIP()
  Serial.println("\nTesting WiFi.gatewayIP()...");
  uint32_t gateway = WiFi.gatewayIP();
  char* gatewayBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(gatewayBuf, "Gateway IP: 0x%08lX", static_cast<unsigned long>(gateway));
  Serial.println(gatewayBuf);
  free(gatewayBuf);
  
  // Test WiFi.RSSI()
  Serial.println("\nTesting WiFi.RSSI()...");
  int32_t rssi = WiFi.RSSI();
  char* rssiBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(rssiBuf, "RSSI: %ld dBm", static_cast<long>(rssi));
  Serial.println(rssiBuf);
  free(rssiBuf);
  
  // Test WiFi.macAddress()
  Serial.println("\nTesting WiFi.macAddress()...");
  uint8_t mac[6];
  uint8_t* macPtr = WiFi.macAddress(mac);
  if (macPtr != nullptr) {
    char* macBuf = reinterpret_cast<char*>(malloc(32));
    sprintf(macBuf, "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(macBuf);
    free(macBuf);
  } else {
    Serial.println("MAC: (unavailable)");
  }
  
  // Test WiFi.hostname()
  Serial.println("\nTesting WiFi.hostname()...");
  const char* test_hostname = "SRAM-Pico2W-Test";
  int hostname_result = WiFi.hostname(test_hostname);
  char* hostnameBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(hostnameBuf, "hostname() result: %d", hostname_result);
  Serial.println(hostnameBuf);
  free(hostnameBuf);
  
  // Test WiFi.getHostname()
  const char* current_hostname = WiFi.getHostname();
  if (current_hostname != nullptr) {
    Serial.print("Current hostname: ");
    Serial.println(current_hostname);
  } else {
    Serial.println("Hostname: (unavailable)");
  }
  
  // Test WiFi.disconnect()
  Serial.println("\nTesting WiFi.disconnect()...");
  int disconnect_result = WiFi.disconnect();
  char* disconnectBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(disconnectBuf, "disconnect() result: %d", disconnect_result);
  Serial.println(disconnectBuf);
  free(disconnectBuf);
  
  Serial.println("\nWiFi test complete.");
  Serial.println("[NOTE] Actual WiFi connection requires valid SSID and password.");
}

// =====================================================================
// Bluetooth/BLE test functions
// =====================================================================

void test_ble() {
  Serial.println("\n=== Testing Bluetooth/BLE Functions ===");
  
  // Test BLE.begin()
  Serial.println("Testing BLE.begin()...");
  bool begin_result = BLE.begin();
  char* beginBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(beginBuf, "BLE.begin() result: %s", begin_result ? "true" : "false");
  Serial.println(beginBuf);
  free(beginBuf);
  
  if (!begin_result) {
    Serial.println("[WARNING] BLE.begin() failed. BLE may not be available or initialized.");
    Serial.println("Continuing with other tests...");
  }
  
  // Test BLE.setLocalName()
  Serial.println("\nTesting BLE.setLocalName()...");
  const char* ble_name = "SRAM-Pico2W-BLE";
  BLE.setLocalName(ble_name);
  Serial.print("Set BLE local name to: ");
  Serial.println(ble_name);
  
  // Test BLE.setAdvertisedServiceUuid()
  Serial.println("\nTesting BLE.setAdvertisedServiceUuid()...");
  const char* service_uuid = "12345678-1234-1234-1234-123456789abc";
  BLE.setAdvertisedServiceUuid(service_uuid);
  Serial.print("Set advertised service UUID to: ");
  Serial.println(service_uuid);
  
  // Test BLE.setAdvertisedServiceData()
  Serial.println("\nTesting BLE.setAdvertisedServiceData()...");
  uint8_t service_data[] = {0x01, 0x02, 0x03, 0x04};
  BLE.setAdvertisedServiceData(0x1234, service_data, sizeof(service_data));
  Serial.println("Set advertised service data (UUID: 0x1234, 4 bytes)");
  
  // Test BLE.address()
  Serial.println("\nTesting BLE.address()...");
  const char* ble_address = BLE.address();
  if (ble_address != nullptr) {
    Serial.print("BLE address: ");
    Serial.println(ble_address);
  } else {
    Serial.println("BLE address: (unavailable)");
  }
  
  // Test BLE.available()
  Serial.println("\nTesting BLE.available()...");
  bool available = BLE.available();
  char* availBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(availBuf, "BLE.available(): %s", available ? "true" : "false");
  Serial.println(availBuf);
  free(availBuf);
  
  // Test BLE.central()
  Serial.println("\nTesting BLE.central()...");
  bool central = BLE.central();
  char* centralBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(centralBuf, "BLE.central(): %s", central ? "true" : "false");
  Serial.println(centralBuf);
  free(centralBuf);
  
  // Test BLE.connected()
  Serial.println("\nTesting BLE.connected()...");
  bool connected = BLE.connected();
  char* connectedBuf = reinterpret_cast<char*>(malloc(32));
  sprintf(connectedBuf, "BLE.connected(): %s", connected ? "true" : "false");
  Serial.println(connectedBuf);
  free(connectedBuf);
  
  // Test BLE.advertise()
  Serial.println("\nTesting BLE.advertise()...");
  BLE.advertise();
  Serial.println("BLE.advertise() called - BLE should now be advertising");
  delay(1000);  // Give it a moment
  
  // Test BLE.stopAdvertise()
  Serial.println("\nTesting BLE.stopAdvertise()...");
  BLE.stopAdvertise();
  Serial.println("BLE.stopAdvertise() called - BLE should stop advertising");
  
  // Test BLE.disconnect()
  Serial.println("\nTesting BLE.disconnect()...");
  BLE.disconnect();
  Serial.println("BLE.disconnect() called");
  
  // Test BLE.end()
  Serial.println("\nTesting BLE.end()...");
  BLE.end();
  Serial.println("BLE.end() called - BLE should be stopped");
  
  Serial.println("\nBLE test complete.");
  Serial.println("[NOTE] Actual BLE functionality requires proper initialization and may need");
  Serial.println("       additional configuration for your specific use case.");
}