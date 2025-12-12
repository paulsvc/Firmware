#include <Arduino.h>
#include <stdio.h>
extern "C" {
  #include "ecat_slv.h"
  #include "utypes.h"
  };
  _Objects Obj;

// --- LED PINS ---
const int PIN_ODD_LED  = PB13;
const int PIN_EVEN_LED = PB14;

#if !defined(HAVE_HWSERIAL1)
  HardwareSerial Serial1(PA10, PA9); // RX, TX
#endif

// --- TIMER OBJECT ---
HardwareTimer *MyTim2;

#define bitset(byte, nbit) ((byte) |= (1 << (nbit)))
#define bitclear(byte, nbit) ((byte) &= ~(1 << (nbit)))
#define bitflip(byte, nbit) ((byte) ^= (1 << (nbit)))
#define bitcheck(byte, nbit) ((byte) & (1 << (nbit)))

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
};

void setup(void) {
  // 1. CONFIGURE LED OUTPUTS
  pinMode(PIN_ODD_LED, OUTPUT);
  pinMode(PIN_EVEN_LED, OUTPUT);
  Serial1.begin(115200);

  #ifdef ECAT
  ecat_slv_init(&config);
  #endif
  // 2. INSTANTIATE TIMER
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
}

void loop(void) {
  // --- READ COUNTER ---
  // You can still use the Arduino method to read the count
  uint32_t currentCount = MyTim2->getCount();
  // Push encoder count into EtherCAT TX PDO (maps to 0x6000:01 Input12)
  Obj.Input12 = static_cast<int32_t>(currentCount);
  //Serial1.println(currentCount);
  //delay(300);
  #ifdef ECAT
  ecat_slv();
  #endif
  // --- ODD / EVEN LOGIC ---
  if (currentCount & 1) {
    // ODD
    digitalWrite(PIN_ODD_LED, HIGH);
    digitalWrite(PIN_EVEN_LED, LOW);
  } 
  else {
    // EVEN
    digitalWrite(PIN_ODD_LED, LOW);
    digitalWrite(PIN_EVEN_LED, HIGH);
  }
}