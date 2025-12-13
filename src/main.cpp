#include <Arduino.h>
#include <stdio.h>
extern "C" {
#include "ecat_slv.h"
#include "utypes.h"
#include "ecat_options.h"
#include "esc.h"
#include "spi.hpp"
#include <SPI.h>
// Define rxpdo buffer (required when MAX_MAPPINGS_SM2 is 0)
uint8_t rxpdo[MAX_RXPDO_SIZE] __attribute__((aligned(8)));
};
_Objects Obj;
extern SPIClass SPI_2;
#include "extend32to64.h"
extend32to64 longTime;

// --- TIMER OBJECT ---
HardwareTimer *MyTim2;

HardwareSerial Serial1(PA10, PA9);
#include <queue>

#define bitset(byte, nbit) ((byte) |= (1 << (nbit)))
#define bitclear(byte, nbit) ((byte) &= ~(1 << (nbit)))
#define bitflip(byte, nbit) ((byte) ^= (1 << (nbit)))
#define bitcheck(byte, nbit) ((byte) & (1 << (nbit)))

// --- LED PINS ---
const int PIN_ODD_LED  = PB13;
const int PIN_EVEN_LED = PB14;

extern "C" uint32_t ESC_SYNC0cycletime(void);
extern "C" void ESC_read(uint16_t address, void *buf, uint16_t len);

// --- SPI DIAGNOSTIC FUNCTION ---
void test_spi_communication(void) {
  Serial1.println("\n=== SPI Communication Test ===");
  
  uint32_t value;
  uint16_t status16;
  uint8_t status8;
  bool spi_ok = true;
  
  // Test 1: Read Chip ID/Revision Register (0x050)
  // Should return 0x9252 for LAN9252
  Serial1.print("Test 1: Chip ID Register (0x050)... ");
  ESC_read(0x050, &value, sizeof(value));
  Serial1.print("Read: 0x");
  Serial1.println(value, HEX);
  if ((value & 0xFFFF) == 0x9252) {
    Serial1.println("  [OK] LAN9252 detected!");
  } else {
    Serial1.print("  [FAIL] Expected 0x9252, got 0x");
    Serial1.println(value & 0xFFFF, HEX);
    spi_ok = false;
  }
  delay(10);
  
  // Test 2: Read Byte Test Register (0x064)
  // Should return 0x87654321
  Serial1.print("Test 2: Byte Test Register (0x064)... ");
  ESC_read(0x064, &value, sizeof(value));
  Serial1.print("Read: 0x");
  Serial1.println(value, HEX);
  if (value == 0x87654321) {
    Serial1.println("  [OK] Byte test passed!");
  } else {
    Serial1.print("  [FAIL] Expected 0x87654321, got 0x");
    Serial1.println(value, HEX);
    spi_ok = false;
  }
  delay(10);
  
  // Test 3: Read Hardware Config Register (0x074)
  // Check READY bit (bit 27)
  Serial1.print("Test 3: Hardware Config Register (0x074)... ");
  ESC_read(0x074, &value, sizeof(value));
  Serial1.print("Read: 0x");
  Serial1.println(value, HEX);
  if (value & (1 << 27)) {
    Serial1.println("  [OK] ESC is READY!");
  } else {
    Serial1.println("  [WARN] ESC not ready (bit 27 not set)");
  }
  delay(10);
  
  // Test 4: Read DL Status (0x0110) - Link Status
  Serial1.print("Test 4: DL Status Register (0x0110)... ");
  ESC_read(0x0110, &status16, sizeof(status16));
  Serial1.print("Read: 0x");
  Serial1.println(status16, HEX);
  Serial1.print("  Link Status: ");
  if (status16 & 0x0001) {
    Serial1.println("UP");
  } else {
    Serial1.println("DOWN");
  }
  delay(10);
  
  // Test 5: Read AL Status (0x0130)
  Serial1.print("Test 5: AL Status Register (0x0130)... ");
  ESC_read(0x0130, &status8, sizeof(status8));
  Serial1.print("Read: 0x");
  Serial1.println(status8, HEX);
  Serial1.print("  State: ");
  switch(status8 & 0x0F) {
    case 1: Serial1.println("INIT"); break;
    case 2: Serial1.println("PREOP"); break;
    case 4: Serial1.println("SAFEOP"); break;
    case 8: Serial1.println("OP"); break;
    default: Serial1.println("UNKNOWN"); break;
  }
  delay(10);
  
  // Test 6: Read AL Control (0x0120)
  Serial1.print("Test 6: AL Control Register (0x0120)... ");
  ESC_read(0x0120, &status8, sizeof(status8));
  Serial1.print("Read: 0x");
  Serial1.println(status8, HEX);
  delay(10);
  
  // Summary
  Serial1.println("\n=== Test Summary ===");
  if (spi_ok) {
    Serial1.println("[SUCCESS] SPI communication is working!");
  } else {
    Serial1.println("[FAILURE] SPI communication has issues!");
  }
  Serial1.println("===================\n");
}

void cb_set_outputs(void) // Get Master outputs, slave inputs, first operation
{
  // Update digital output pins
}



void cb_get_inputs(void) // Set Master inputs, slave outputs, last operation
{

   // 1. Read the Timer Hardware Register directly here
 // We cannot use 'currentCount' from the loop because it is out of scope.
 uint32_t raw_count = MyTim2->getHandle()->Instance->CNT;

 // 2. Assign to EtherCAT Object
 Obj.Counter = static_cast<int32_t>(raw_count);
}

uint16_t dc_checker(void);

static esc_cfg_t config = {
    .user_arg = NULL,
    .use_interrupt = 1,
    .watchdog_cnt = 150,
    .set_defaults_hook = NULL,
    .pre_state_change_hook = NULL,
    .post_state_change_hook = NULL,
    .application_hook = NULL,
    .safeoutput_override = NULL,
    .pre_object_download_hook = NULL,
    .post_object_download_hook = NULL,
    .rxpdo_override = NULL,
    .txpdo_override = NULL,
    .esc_hw_interrupt_enable = NULL,
    .esc_hw_interrupt_disable = NULL,
    .esc_hw_eep_handler = NULL,
    .esc_check_dc_handler = dc_checker,
};

void setup(void) {
  Serial1.begin(115200);
  delay(1000);  // Wait for serial to be ready
  Serial1.println("\n\nEtherCAT Encoder Test - Starting...");

  MyTim2 = new HardwareTimer(TIM2);

  // 3. CONFIGURE PINS (Crucial Step)
  // We use setMode to tell the Arduino core to configure PA0 and PA1 
  // as Alternate Function pins connected to TIM2. 
  // We use INPUT_CAPTURE temporarily just to set up the GPIOs.
  MyTim2->setMode(1, TIMER_INPUT_CAPTURE_RISING, PA0);
  MyTim2->setMode(2, TIMER_INPUT_CAPTURE_RISING, PA1);

  // 4. SWITCH TO ENCODER MODE (Using HAL)
  // Access the raw handle of the timer
  TIM_HandleTypeDef *htim = MyTim2->getHandle();
  //HAL_TIM_IC_DeInit(htim); 

  // Define the Encoder Configuration
  TIM_Encoder_InitTypeDef sConfig = {0};

  // TIM_ENCODERMODE_TI12 counts on both signals (4x resolution - highest precision)
  // Use TIM_ENCODERMODE_TI1 if you want 2x resolution.
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12; 
  
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0; // Increase to 0xF if inputs are noisy (mechanical switches)

  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;

  // Apply the configuration
  if (HAL_TIM_Encoder_Init(htim, &sConfig) != HAL_OK) {
    // Initialization Error
    while(1); 
  }

  // 5. FORCE 32-BIT & START
  // Set the maximum count value (Auto Reload Register)
  __HAL_TIM_SET_AUTORELOAD(htim, 0xFFFFFFFF);
  
  // Start the Encoder Interface
  HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
Serial1.println("\n\nTimer Started");
// 2. SETUP SPI
  // This initializes SPI_2 and sets pins
  spi_setup(); 
  Serial1.println("SPI Initialized.");
  // SPI_2.begin();
  // 3. START ETHERCAT STACK
  // The Manual Test passed, so this should now work immediately.
  Serial1.print("Initializing ESC... ");
  ESC_init(&config); 
  Serial1.println("DONE.");

  Serial1.print("Initializing Slave Stack... ");
  ecat_slv_init(&config);
  Serial1.println("DONE.");
  
  Serial1.println("--> STATE: SAFE-OP / OP. Ready for TwinCAT scan.");

}

void loop(void) {
#ifdef ECAT
  ecat_slv();
  
  // Periodic SPI test (every 10 seconds)
  static unsigned long lastTest = 0;
  if (millis() - lastTest > 10000) {
    lastTest = millis();
    test_spi_communication();
  }
#else
  // If ECAT not defined, just print encoder count
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    uint32_t count = MyTim2->getCount();
    Serial1.print("Encoder Count: ");
    Serial1.println(count);
  }
#endif
}

// Setup of DC
uint16_t dc_checker(void) {
  // Indicate we run DC
  ESCvar.dcsync = 1;
  return 0;
}

