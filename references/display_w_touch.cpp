// Corrected Logic Arduino Code for ESP32-S3
// This version correctly handles sensors where the touch value INCREASES.
// It calibrates a low baseline and uses a percentage increase for the threshold.
// All settings are saved to persist after a reboot.

// --- Include Necessary Libraries ---
#include <Adafruit_DotStar.h> // For the DotStar LED strip
#include <SPI.h>              // Required for DotStar communication
#include <Preferences.h>      // For saving settings to permanent memory

// --- Create a Preferences object ---
Preferences preferences;

// --- DotStar Strip Definitions ---
#define NUMPIXELS 9             // The number of LEDs in your DotStar strip
#define DATAPIN   11            // Data pin for the DotStar strip
#define CLOCKPIN  12            // Clock pin for the DotStar strip
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

// --- Touch Sensor Definitions ---
#define TOUCH_PIN 4      // The GPIO pin for the touch sensor
int touchThreshold = 0;  // The precise threshold. Loaded from memory or calibrated.
// Sensitivity is the percentage INCREASE above the baseline to set the threshold.
uint8_t sensitivity = 10; // Default: trigger at 10% above the baseline.

// --- Debouncing Variables ---
int touchState = 0;                 // The current debounced state (0 = OFF, 1 = ON)
int lastReading = 0;                // The previous raw reading
unsigned long lastDebounceTime = 0; // The last time the state changed
unsigned long debounceDelay = 50;   // The debounce time in milliseconds

// --- Function to run the simplified calibration sequence ---
void runCalibrationSequence() {
  Serial.println("\n--- Starting Touch Calibration ---");
  Serial.println("Calibrating UNTOUCHED state (low baseline)...");
  Serial.println(">>> DO NOT TOUCH the sensor for 5 seconds. <<<");
  strip.fill(strip.Color(0, 0, 255)); // Blue for calibration
  strip.show();
  delay(2000); // Give user time to read and remove hand

  long untouchedSum = 0;
  int calibrationSamples = 200;
  for (int i = 0; i < calibrationSamples; i++) {
    untouchedSum += touchRead(TOUCH_PIN);
    delay(5);
  }
  int untouchedAverage = untouchedSum / calibrationSamples;
  Serial.print("Untouched Average (Baseline): ");
  Serial.println(untouchedAverage);

  // Calculate the threshold as a percentage INCREASE above the baseline
  touchThreshold = (int)(untouchedAverage * (1.0 + (sensitivity / 100.0)));

  // Save the new threshold to permanent memory
  preferences.begin("touch-settings", false);
  preferences.putUInt("touchThresh", touchThreshold);
  preferences.end();

  Serial.println("\n-------------------------------------------------");
  Serial.println("Calibration Complete and Saved!");
  Serial.print("New Touch Threshold set to: ");
  Serial.println(touchThreshold);
  Serial.println("-------------------------------------------------");
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  strip.begin();
  strip.show(); // Initialize strip

  Serial.println("--- ESP32-S3 Touch Control (Corrected Logic) ---");

  // --- Load all saved settings from permanent memory ---
  preferences.begin("touch-settings", true); // Start in read-only mode
  touchThreshold = preferences.getUInt("touchThresh", 0);
  uint8_t savedBrightness = preferences.getUChar("brightness", 80); // Default to 80
  sensitivity = preferences.getUChar("sensitivity", 10); // Default to 10%
  preferences.end();

  strip.setBrightness(savedBrightness);

  if (touchThreshold != 0) {
    Serial.print("Loaded saved threshold: ");
    Serial.println(touchThreshold);
  } else {
    Serial.println("No threshold saved. Please run calibration.");
  }
  Serial.print("Loaded saved brightness: ");
  Serial.println(savedBrightness);
  Serial.print("Loaded saved sensitivity: ");
  Serial.print(sensitivity);
  Serial.println("% increase");
  
  Serial.println("\n--- Commands ---");
  Serial.println("Send 'c' to start a new calibration sequence.");
  Serial.println("Send 'b' + number (e.g., b150) to set brightness.");
  Serial.println("Send 's' + number (e.g., s5) to set sensitivity percentage (1-25).");
}

void loop() {
  // === Part 1: Handle Serial Input ===
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'b') {
      int newBrightness = Serial.parseInt();
      newBrightness = constrain(newBrightness, 0, 255);
      strip.setBrightness(newBrightness);
      
      preferences.begin("touch-settings", false);
      preferences.putUChar("brightness", newBrightness);
      preferences.end();

      Serial.print("New brightness set and saved: ");
      Serial.println(newBrightness);

    } else if (command == 'c') {
      runCalibrationSequence();
    } else if (command == 's') {
      int newSensitivity = Serial.parseInt();
      // A smaller number is MORE sensitive.
      newSensitivity = constrain(newSensitivity, 1, 25);
      sensitivity = newSensitivity;

      preferences.begin("touch-settings", false);
      preferences.putUChar("sensitivity", sensitivity);
      preferences.end();

      Serial.print("New sensitivity set and saved: ");
      Serial.print(sensitivity);
      Serial.println("%. You MUST recalibrate ('c') for this to take effect.");
    }
    while(Serial.available() > 0) Serial.read(); // Clear buffer
  }

  // Only run touch detection if a threshold has been set
  if (touchThreshold == 0) {
    strip.fill(strip.Color(50, 0, 0)); // Dim red
    strip.show();
    delay(250);
    strip.clear();
    strip.show();
    delay(250);
    return; // Skip the rest of the loop
  }

  // === Part 2: Debouncing Logic ===
  int touchValue = touchRead(TOUCH_PIN);
  // CORRECTED LOGIC: Check if the value is GREATER than the threshold
  int currentReading = (touchValue > touchThreshold) ? 1 : 0;

  if (currentReading != lastReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentReading != touchState) {
      touchState = currentReading;
    }
  }
  lastReading = currentReading;

  // --- Print diagnostic data ---
  Serial.print("Touch:");
  Serial.print(touchValue);
  Serial.print(", Thresh:");
  Serial.print(touchThreshold);
  Serial.print(", State:");
  Serial.println(touchState);

  // === Part 3: Control the DotStar LED Strip Color ===
  if (touchState == 1) {
    strip.fill(strip.Color(0, 255, 0)); // Green
  } else {
    strip.fill(strip.Color(255, 0, 0)); // Red
  }
  strip.show();

  // === Part 4: Global Delay ===
  delay(10);
}
