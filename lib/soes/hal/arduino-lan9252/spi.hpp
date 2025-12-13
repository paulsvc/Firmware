#ifndef SRC_APP_SPI_H_
#define SRC_APP_SPI_H_

#include <stdint.h>

// --- Defines ---
#define SCS_LOW       0
#define SCS_HIGH      1
#define SCS_ACTIVE_POLARITY SCS_LOW

#define SPIX_ESC      SPI2
#define SPIX_ESC_SPEED 12000000
#define ESC_GPIO_Pin_CS PB8
#define DUMMY_BYTE    0xFF

// --- C/C++ COMPATIBILITY GUARD ---
// This tells the compiler: "Only use extern "C" if we are in a C++ file"
#ifdef __cplusplus
extern "C" {
#endif

// Function Declarations
void spi_setup(void);
void spi_select(int8_t board);
void spi_unselect(int8_t board);
void write(int8_t board, uint8_t *data, uint8_t size);
void read(int8_t board, uint8_t *result, uint8_t size);
void spi_bidirectionally_transfer(int8_t board, uint8_t *result, uint8_t *data, uint8_t size);

#ifdef __cplusplus
}
#endif
// ---------------------------------

#endif /* SRC_APP_SPI_H_ */