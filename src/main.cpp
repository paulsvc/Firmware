#include <Arduino.h>
#include <stdio.h>
extern "C" {
#include "ecat_slv.h"
#include "utypes.h"
#include "ecat_options.h"
// Define rxpdo buffer (required when MAX_MAPPINGS_SM2 is 0)
uint8_t rxpdo[MAX_RXPDO_SIZE] __attribute__((aligned(8)));
};
_Objects Obj;

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

extern "C" uint32_t ESC_SYNC0cycletime(void);

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


#ifdef ECAT
  ecat_slv_init(&config);
#endif

}

void loop(void) {
#ifdef ECAT
  ecat_slv();
#endif
}

// Setup of DC
uint16_t dc_checker(void) {
  // Indicate we run DC
  ESCvar.dcsync = 1;
  return 0;
}

