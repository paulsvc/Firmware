# SPI Setup and Usage Explanation

## Overview

This project uses **SPI2** to communicate with the **LAN9252 EtherCAT Slave Controller (ESC)**. The SPI communication is abstracted through a hardware abstraction layer (HAL) that provides a clean interface for the EtherCAT stack.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    main.cpp (Your Application)               │
│                                                              │
│  setup() {                                                   │
│    ecat_slv_init(&config);   ───────────────┐                │
│  }                                          │                │
│                                             │                │
│  loop() {                                   │                │
│    ecat_slv();  ────────────────────────────┼──┐             │
│  }                                          │  │             │
└─────────────────────────────────────────────┼──┼─────────────┘
                                              │  │
┌─────────────────────────────────────────────┼──┼─────────────┐
│         EtherCAT Stack (SOES Library)       │  │             │
│                                             │  │             │
│  ecat_slv_init() {                          │  │             │
│    ESC_init(config);  ────────────────┐     │  │             │
│  }                                    │     │  │             │
│                                       │     │  │             │
│  ecat_slv() {                         │     │  │             │
│    ESC_read() / ESC_write()  ────── ──┼─────┼──┘             │
│  }                                    │     │                │
└───────────────────────────────────────┼─────┼────────────────┘
                                        │     │
┌───────────────────────────────────────┼─────┼─────────────────┐
│      Hardware Abstraction Layer       │     │                 │
│                                       │     │                 │
│  esc_hw.c (LAN9252 Driver)            │     │                 │
│                                       │     │                 │
│  ESC_init() {                         │     │                 │
│    spi_setup();  ──────────── ────┐   │     │                 │
│    // Reset LAN9252               │   │     │                 │
│    // Verify chip ID              │   │     │                 │
│  }                                │   │     │                 │
│                                   │   │     │                 │
│  ESC_read() {                     │   │     │                 │
│    lan9252_read_32() {            │   │     │                 │
│      spi_select()  ───────────────┼───┼─────┼──┐              │
│      write() / read()  ───────────┼───┼─────┼──┼──┐           │
│      spi_unselect()  ─────────────┼───┼─────┼──┼──┼──┐        │
│    }                              │   │     │  │  │  │        │
│  }                                │   │     │  │  │  │        │
│                                   │   │     │  │  │  │        │
│  ESC_write() {                    │   │     │  │  │  │        │
│    lan9252_write_32() {           │   │     │  │  │  │        │
│      spi_select()  ───────────────┼───┼─────┼──┼──┼──┼──┐     │
│      write()  ────────────────────┼───┼─────┼──┼──┼──┼──┼──┐  │
│      spi_unselect()  ─────────────┼───┼─────┼──┼──┼──┼──┼──┼─┐│
│    }                              │   │     │  │  │  │  │  │ ││
│  }                                │   │     │  │  │  │  │  │ ││
└───────────────────────────────────┼───┼─────┼──┼──┼──┼──┼──┼─┼┘
                                    │   │     │  │  │  │  │  │ │
┌───────────────────────────────────┼───┼─────┼──┼──┼──┼──┼──┼─┼┐
│      SPI Hardware Layer           │   │     │  │  │  │  │  │ ││
│                                   │   │     │  │  │  │  │  │ ││
│  spi.cpp / spi.hpp                │   │     │  │  │  │  │  │ ││
│                                   │   │     │  │  │  │  │  │ ││
│  spi_setup() {                    │   │     │  │  │  │  │  │ ││
│    SPI_2.begin(PB10, PC2, PC3);  ─┼───┘     │  │  │  │  │  │ ││
│    pinMode(PC4, OUTPUT);          │         │  │  │  │  │  │ ││
│    SPI_2.beginTransaction(...);   │         │  │  │  │  │  │ ││
│  }                                │         │  │  │  │  │  │ ││
│                                   │         │  │  │  │  │  │ ││
│  spi_select() {                   │         │  │  │  │  │  │ ││
│    digitalWrite(PC4, LOW);  ──────┼─────────┘  │  │  │  │  │ ││
│  }                                │            │  │  │  │  │ ││
│                                   │            │  │  │  │  │ ││
│  write() {                        │            │  │  │  │  │ ││
│    SPI_2.transfer(data[i]);  ─────┼────────────┘  │  │  │  │ ││
│  }                                │               │  │  │  │ ││
│                                   │               │  │  │  │ ││
│  read() {                         │               │  │  │  │ ││
│    SPI_2.transfer(0xFF);  ────────┼───────────────┘  │  │  │ ││
│  }                                │                  │  │  │ ││
│                                   │                  │  │  │ ││
│  spi_unselect() {                 │                  │  │  │ ││
│    digitalWrite(PC4, HIGH);  ─────┼──────────────────┘  │  │ ││
│  }                                │                     │  │ ││
└───────────────────────────────────┼─────────────────────┼──┼─┘│
                                    │                     │  │  │
┌───────────────────────────────────┼─────────────────────┼──┼──┘
│      STM32F407 Hardware           │                     │  │
│                                   │                     │  │
│  SPI2 Peripheral:                 │                     │  │
│    - SCK:  PB10                   │                     │  │
│    - MISO: PC2                    │                     │  │
│    - MOSI: PC3                    │                     │  │
│    - CS:   PC4 (software managed) │                     │  │
│                                   │                     │  │
│  LAN9252 EtherCAT Controller      │                     │  │
└───────────────────────────────────┴─────────────────────┴──┘
```

## File Structure

### 1. Configuration File: `lib/soes/hal/arduino-lan9252/spi.hpp`

This header file defines the SPI configuration constants:

```cpp
#define SPIX_ESC              SPI2        // Uses SPI2 peripheral
#define SPIX_ESC_SPEED        50000000   // 50 MHz SPI speed
#define ESC_GPIO_Pin_CS       PC4        // Chip Select pin (software managed)
#define SCS_ACTIVE_POLARITY   SCS_LOW    // CS active low
#define DUMMY_BYTE            0xFF       // Dummy byte for reads
```

**Key Points:**
- Uses **SPI2** (STM32 SPI2 peripheral)
- Speed: **50 MHz** (very fast for real-time EtherCAT)
- CS pin: **PC4** (software-controlled, not hardware CS)
- Active low chip select

### 2. Implementation File: `lib/soes/hal/arduino-lan9252/spi.cpp`

This file implements the SPI functions. For SPI2, it creates a `SPIClass` instance:

```cpp
SPIClass SPI_2(SPI2);  // Create SPI2 instance
```

#### `spi_setup()` - Initialization
```cpp
void spi_setup(void) {
  SPI_2.begin(PB10, PC2, PC3);                    // Initialize SPI2 with custom pins
  pinMode(PC4, OUTPUT);                          // Configure CS pin
  spi_unselect(0);                                // Set CS high (inactive)
  delay(100);                                     // Wait for stabilization
  SPI_2.beginTransaction(SPISettings(...));      // Configure SPI parameters
}
```

**What happens:**
1. `SPI_2.begin(PB10, PC2, PC3)` - Initializes SPI2 with custom pins (PB10=SCK, PC2=MISO, PC3=MOSI)
2. Configures PC4 as output for chip select
3. Sets CS high (inactive)
4. Configures SPI transaction settings (50 MHz, MSB first, Mode 0)

#### `spi_select()` / `spi_unselect()` - Chip Select Control
```cpp
void spi_select(int8_t board) {
  digitalWrite(PC4, LOW);    // Activate CS (active low)
}

void spi_unselect(int8_t board) {
  digitalWrite(PC4, HIGH);   // Deactivate CS
}
```

#### `write()` / `read()` - Data Transfer
```cpp
void write(int8_t board, uint8_t *data, uint8_t size) {
  for(int i = 0; i < size; ++i) {
    SPI_2.transfer(data[i]);    // Send each byte
  }
}

void read(int8_t board, uint8_t *result, uint8_t size) {
  for(int i = 0; i < size; ++i) {
    result[i] = SPI_2.transfer(0xFF);  // Send dummy byte, receive data
  }
}
```

### 3. Hardware Driver: `lib/soes/hal/arduino-lan9252/esc_hw.c`

This file implements the LAN9252-specific communication protocol:

#### `ESC_init()` - Called from EtherCAT Stack
```cpp
void ESC_init(const esc_cfg_t * config) {
  spi_setup();                    // ← SPI initialization happens here!
  
  // Reset LAN9252 via SPI
  lan9252_write_32(ESC_RESET_CTRL_REG, ESC_DIGITAL_RST);
  
  // Wait for reset to complete
  // Verify chip ID
  // Check ready flag
}
```

#### `lan9252_read_32()` / `lan9252_write_32()` - Low-level Commands
```cpp
static uint32_t lan9252_read_32(uint32_t address) {
  uint8_t data[4];
  uint8_t result[4];
  
  // Build read command
  data[0] = ESC_CMD_FAST_READ;      // Command byte
  data[1] = (address >> 8) & 0xFF; // Address high byte
  data[2] = address & 0xFF;         // Address low byte
  data[3] = ESC_CMD_FAST_READ_DUMMY; // Dummy byte
  
  spi_select(lan9252);              // ← Activate CS
  write(lan9252, data, sizeof(data)); // Send command
  read(lan9252, result, sizeof(result)); // Read data
  spi_unselect(lan9252);            // ← Deactivate CS
  
  return (result[3] << 24) | (result[2] << 16) | 
         (result[1] << 8) | result[0];
}
```

### 4. Application Code: `src/main.cpp`

#### Initialization Chain
```cpp
void setup(void) {
  // ... GPIO setup ...
  
  #ifdef ECAT
    ecat_slv_init(&config);  // ← Entry point
  #endif
}

void loop(void) {
  #ifdef ECAT
    ecat_slv();  // ← Continuously processes EtherCAT communication
  #endif
}
```

**Call Chain:**
1. `setup()` calls `ecat_slv_init(&config)`
2. `ecat_slv_init()` calls `ESC_init(config)`
3. `ESC_init()` calls `spi_setup()`
4. `spi_setup()` initializes SPI2 hardware

**Runtime:**
1. `loop()` calls `ecat_slv()` continuously
2. `ecat_slv()` processes EtherCAT events
3. When data needs to be read/written, it calls `ESC_read()` / `ESC_write()`
4. These functions use `lan9252_read_32()` / `lan9252_write_32()`
5. Which in turn call `spi_select()`, `write()`, `read()`, `spi_unselect()`

## SPI Pin Configuration

### Current Setup (SPI2 - Custom Pins)
- **SCK (Clock)**: PB10
- **MISO (Master In Slave Out)**: PC2
- **MOSI (Master Out Slave In)**: PC3
- **CS (Chip Select)**: PC4 (software managed)

### How Pins Are Configured

For STM32F407, SPI2 requires explicit pin configuration when using custom pins:
- SPI2 instance must be created: `SPIClass SPI_2(SPI2)`
- Pins are specified in `SPI_2.begin(PB10, PC2, PC3)` - SCK, MISO, MOSI
- CS is manually controlled via `digitalWrite(PC4, ...)`
- All SPI operations use `SPI_2` instead of default `SPI`

## SPI Communication Protocol

### Transaction Flow

```
1. spi_select()           → CS goes LOW
2. write(command_bytes)   → Send command to LAN9252
3. read(data_bytes)      → Receive data from LAN9252
4. spi_unselect()        → CS goes HIGH
```

### Example: Reading a Register

```
┌─────────┐                    ┌──────────┐
│  STM32  │                    │ LAN9252 │
└────┬────┘                    └────┬─────┘
     │                              │
     │ CS LOW                       │
     ├─────────────────────────────>│
     │                              │
     │ 0x0B (FAST_READ)            │
     ├─────────────────────────────>│
     │ 0x00 (Address High)         │
     ├─────────────────────────────>│
     │ 0x50 (Address Low)          │
     ├─────────────────────────────>│
     │ 0xFF (Dummy)                │
     ├─────────────────────────────>│
     │                              │
     │                   0x92 (Data)│
     │<─────────────────────────────┤
     │                   0x52 (Data)│
     │<─────────────────────────────┤
     │                   0x00 (Data)│
     │<─────────────────────────────┤
     │                   0x01 (Data)│
     │<─────────────────────────────┤
     │                              │
     │ CS HIGH                      │
     ├─────────────────────────────>│
```

## Key Functions Summary

| Function | Location | Purpose |
|----------|----------|---------|
| `spi_setup()` | `spi.cpp` | Initialize SPI2 hardware |
| `spi_select()` | `spi.cpp` | Activate CS (set PC4 LOW) |
| `spi_unselect()` | `spi.cpp` | Deactivate CS (set PC4 HIGH) |
| `write()` | `spi.cpp` | Send data bytes via SPI |
| `read()` | `spi.cpp` | Receive data bytes via SPI |
| `ESC_init()` | `esc_hw.c` | Initialize LAN9252 (calls `spi_setup()`) |
| `lan9252_read_32()` | `esc_hw.c` | Read 32-bit value from LAN9252 |
| `lan9252_write_32()` | `esc_hw.c` | Write 32-bit value to LAN9252 |
| `ESC_read()` | `esc_hw.c` | High-level read (used by EtherCAT stack) |
| `ESC_write()` | `esc_hw.c` | High-level write (used by EtherCAT stack) |

## Important Notes

1. **No Direct SPI Calls in main.cpp**: The application code (`main.cpp`) never directly calls SPI functions. Everything goes through the EtherCAT stack.

2. **Custom Pin Configuration**: SPI2 pins (PB10, PC2, PC3) must be explicitly configured using `SPI_2.begin(PB10, PC2, PC3)` - requires creating a `SPIClass SPI_2(SPI2)` instance.

3. **Software CS**: Chip Select is managed in software via PC4, not hardware CS. This gives more control over timing.

4. **High Speed**: 50 MHz is very fast - ensure your PCB traces are short and properly routed for signal integrity.

5. **SPI Mode 0**: CPOL=0, CPHA=0 (clock idle low, data sampled on rising edge)

6. **MSB First**: Data is transmitted most significant bit first.

## Debugging Tips

If SPI communication fails:

1. **Check CS pin**: Verify PC4 is toggling correctly
2. **Verify pins**: Confirm PB10, PC2, PC3 are connected correctly
3. **Check speed**: 50 MHz might be too fast - try reducing `SPIX_ESC_SPEED`
4. **Verify LAN9252**: Check if chip is powered and reset properly
5. **Use oscilloscope**: Check SPI signals for proper timing
6. **Check SPI2 instance**: Ensure `SPIClass SPI_2(SPI2)` is properly initialized

## Summary

The SPI setup is completely transparent to your application code. When you call `ecat_slv_init()`, it automatically:
1. Initializes SPI2 hardware
2. Configures CS pin (PC4)
3. Sets up LAN9252 communication
4. Verifies the chip is working

During runtime, `ecat_slv()` handles all SPI communication automatically in the background to exchange EtherCAT process data with the master.

