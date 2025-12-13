/*
 * Licensed under the GNU General Public License version 2 with exceptions. See
 * LICENSE file in the project root for full license information
 */

 /** \file
 * \brief
 * ESC hardware layer functions for LAN9252.
 */

#include "esc.h"
#include <string.h>
#include "spi.hpp" 

// --- COMMAND DEFINITIONS ---
#define ESC_CMD_WRITE           0x02
#define ESC_CMD_READ            0x03

// --- REGISTER DEFINITIONS ---
#define ESC_CSR_DATA_REG        0x300
#define ESC_CSR_CMD_REG         0x304
#define ESC_CSR_CMD_BUSY        0x80000000 // Bit 31
#define ESC_CSR_CMD_READ        0xC0000000 // Read is Bit 31 & 30
#define ESC_CSR_CMD_WRITE       0x80000000 // Write is Bit 31
#define ESC_CSR_CMD_SIZE(x)     (x << 16)

#define ESC_RESET_CTRL_REG      0x1F8
#define ESC_RESET_CTRL_RST      0x00000040 // Bit 6
#define ESC_DIGITAL_RST         0x00000001

#define ESC_ID_REV_REG          0x050
#define LAN9252_ID_REV          0x9252

#define ESC_BYTE_TEST_REG       0x064
#define ESC_TEST_VALUE          0x87654321

#define ESC_HW_CFG_REG          0x074
#define ESC_READY               0x08000000 // Bit 27

static int lan9252 = 0; // Default board index

// ===========================================================================
// LOW LEVEL READ/WRITE (Matches your successful Manual Test)
// ===========================================================================

// Helper: Write 32-bit value (Used for CSR Registers)
static void lan9252_write_32(uint16_t address, uint32_t val)
{
    uint8_t data[7];
    
    // Command 0x02 (WRITE)
    data[0] = ESC_CMD_WRITE;
    data[1] = (address >> 8);
    data[2] = (address & 0xFF);
    
    // Data (Little Endian)
    data[3] = (val & 0xFF);
    data[4] = ((val >> 8) & 0xFF);
    data[5] = ((val >> 16) & 0xFF);
    data[6] = ((val >> 24) & 0xFF);

    spi_select(lan9252);
    write(lan9252, data, 7);
    spi_unselect(lan9252);
}

// Helper: Read 32-bit value (Used for CSR Registers)
static uint32_t lan9252_read_32(uint32_t address)
{
    uint8_t cmd[3];
    uint8_t result[4];

    // Command 0x03 (READ) - NO DUMMY BYTE
    cmd[0] = ESC_CMD_READ;
    cmd[1] = (address >> 8);
    cmd[2] = (address & 0xFF);

    spi_select(lan9252);
    write(lan9252, cmd, 3);
    read(lan9252, result, 4); // Read 4 bytes immediately
    spi_unselect(lan9252);

    return ((uint32_t)result[3] << 24) |
           ((uint32_t)result[2] << 16) |
           ((uint32_t)result[1] << 8)  |
           ((uint32_t)result[0]);
}

// ===========================================================================
// CSR ACCESS (Indirect Access for System Registers >= 0x1000)
// ===========================================================================

static void ESC_read_csr(uint16_t address, void *buf, uint16_t len)
{
    uint32_t val;
    
    // 1. Setup CSR Command (Read + Size + Address)
    val = (ESC_CSR_CMD_READ | ESC_CSR_CMD_SIZE(len) | address);
    lan9252_write_32(ESC_CSR_CMD_REG, val);

    // 2. Wait for Busy Flag to Clear
    do {
        val = lan9252_read_32(ESC_CSR_CMD_REG);
    } while (val & ESC_CSR_CMD_BUSY);

    // 3. Read Data
    val = lan9252_read_32(ESC_CSR_DATA_REG);
    memcpy(buf, &val, len);
}

static void ESC_write_csr(uint16_t address, void *buf, uint16_t len)
{
    uint32_t val = 0;
    memcpy(&val, buf, len);

    // 1. Write Data to Data Register
    lan9252_write_32(ESC_CSR_DATA_REG, val);

    // 2. Setup CSR Command (Write + Size + Address)
    val = (ESC_CSR_CMD_WRITE | ESC_CSR_CMD_SIZE(len) | address);
    lan9252_write_32(ESC_CSR_CMD_REG, val);

    // 3. Wait for Busy Flag to Clear
    do {
        val = lan9252_read_32(ESC_CSR_CMD_REG);
    } while (val & ESC_CSR_CMD_BUSY);
}

// ===========================================================================
// MAIN SOES INTERFACE FUNCTIONS
// ===========================================================================

void ESC_read(uint16_t address, void *buf, uint16_t len)
{
    // If High Address, use CSR Indirect Access
    if (address >= 0x1000) {
        ESC_read_csr(address, buf, len);
        return;
    }

    // Direct Read (Command 0x03) for RAM
    spi_select(lan9252);

    uint8_t cmd[3];
    cmd[0] = ESC_CMD_READ;
    cmd[1] = (address >> 8);
    cmd[2] = (address & 0xFF);
    
    write(lan9252, cmd, 3);
    read(lan9252, (uint8_t*)buf, len);

    spi_unselect(lan9252);
}

void ESC_write(uint16_t address, void *buf, uint16_t len)
{
    // If High Address, use CSR Indirect Access
    if (address >= 0x1000) {
        ESC_write_csr(address, buf, len);
        return;
    }

    // Direct Write (Command 0x02) for RAM
    spi_select(lan9252);

    uint8_t cmd[3];
    cmd[0] = ESC_CMD_WRITE;
    cmd[1] = (address >> 8);
    cmd[2] = (address & 0xFF);
    
    write(lan9252, cmd, 3);
    write(lan9252, (uint8_t*)buf, len);

    spi_unselect(lan9252);
    
    // Mimic AL Event behavior
    ESC_read_csr(ESCREG_ALEVENT, (void *)&ESCvar.ALevent, sizeof(ESCvar.ALevent));
    ESCvar.ALevent = etohs(ESCvar.ALevent);
}

void ESC_reset(void) {
    // Not implemented for this board
}

void ESC_init(const esc_cfg_t * config)
{
    uint32_t value;

    spi_setup();

    // 1. Soft Reset the LAN9252
    lan9252_write_32(ESC_RESET_CTRL_REG, ESC_DIGITAL_RST);
    
    // Wait for Reset Bit to clear
    do {
        value = lan9252_read_32(ESC_RESET_CTRL_REG);
    } while (value & ESC_RESET_CTRL_RST);

    // 2. Byte Order Test (Should read 0x87654321)
    // This is where you were getting stuck!
    do {
        value = lan9252_read_32(ESC_BYTE_TEST_REG);
    } while (value != ESC_TEST_VALUE);

    // 3. Wait for Ready Flag
    do {
        value = lan9252_read_32(ESC_HW_CFG_REG);
    } while ((value & ESC_READY) == 0);

    // 4. Verify Chip ID
    value = lan9252_read_32(ESC_ID_REV_REG);
    if ((value >> 16) != LAN9252_ID_REV) {
        while(1); // Error: Wrong Chip
    }
}