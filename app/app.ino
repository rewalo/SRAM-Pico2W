// Include standard library headers when libc is linked
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

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