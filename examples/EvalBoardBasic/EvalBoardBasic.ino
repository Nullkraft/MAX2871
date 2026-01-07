/*
  MAX2871 Evaluation Board - Basic Example
  
  This example demonstrates basic usage of the MAX2871 PLL synthesizer
  on the evaluation board using GPIO bit-banging communication.
  
  Hardware Setup:
  - Connect MAX2871 evaluation board to Arduino
  - CLK  -> A1
  - DATA -> A2  
  - LE   -> A3
  - CE   -> D5 (RF Enable)
  - Reference clock: 60 MHz
  
  The sketch will:
  1. Initialize the MAX2871
  2. Set output frequency to 100 MHz
  3. Enable both RF outputs at +5 dBm
*/

#include <max2871.h>
#include <bitbang_hal.h>

// Pin definitions for evaluation board
const uint8_t PIN_CLK = A1;     // Clock
const uint8_t PIN_DAT = A2;     // Data
const uint8_t PIN_LE  = A3;     // Latch Enable
const uint8_t PIN_CE  = 5;      // Chip Enable (RF output enable)

// Reference clock frequency in MHz
const double REF_FREQ_MHZ = 60.0;

// Create HAL and synthesizer objects
BitBangHAL hal(PIN_CLK, PIN_DAT, PIN_LE, PIN_CE);
MAX2871 synth(REF_FREQ_MHZ);

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }  // Wait for serial port (Leonardo/Micro)
  
  Serial.println("MAX2871 Evaluation Board - Basic Example");
  Serial.println("=========================================");
  
  // Initialize the hardware abstraction layer
  hal.begin();
  Serial.println("HAL initialized");
  
  // Attach HAL to synthesizer and initialize
  synth.attachHal(&hal);
  synth.begin();
  Serial.println("MAX2871 initialized");
  
  // Enable the RF output
  hal.setCEPin(true);
  Serial.println("RF output enabled");
  
  // Set frequency to 100 MHz
  Serial.println("\nSetting frequency to 100 MHz...");
  synth.setFrequency(100.0);
  
  // Enable both RF outputs (A and B)
  synth.outputSelect(RF_ALL);  // 3 = both outputs on
  Serial.println("Both RF outputs enabled");
  
  // Set output power to +5 dBm on both outputs
  synth.outputPower(5, RF_A);
  synth.outputPower(5, RF_B);
  Serial.println("Output power set to +5 dBm");
  
  Serial.println("\nSetup complete!");
  Serial.println("RF outputs should now be generating 100 MHz");
}

void loop() {
  // Example: Sweep through some frequencies
  static unsigned long lastChange = 0;
  static uint8_t freqIndex = 0;
  
  // Change frequency every 5 seconds
  if (millis() - lastChange > 5000) {
    lastChange = millis();
    
    // Frequency sequence: 100, 500, 1000, 2400, 5800 MHz
    const double frequencies[] = {100.0, 500.0, 1000.0, 2400.0, 5800.0};
    const uint8_t numFreqs = 5;
    
    freqIndex = (freqIndex + 1) % numFreqs;
    double freq = frequencies[freqIndex];
    
    Serial.print("Changing frequency to ");
    Serial.print(freq, 1);
    Serial.println(" MHz");
    
    synth.setFrequency(freq);
  }
}
