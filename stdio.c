#include <stdint.h>

#define UART0_BASE 0x44E09000
#define UART_LSR 0x14
#define UART_THR 0x00

void put_char(char c) {
    volatile uint32_t *uart = (volatile uint32_t *)UART0_BASE;
    while (!(uart[UART_LSR / 4] & 0x20)) {}  // Espero a que el transmisor esté disponible
    uart[UART_THR / 4] = c;
    for (volatile int i = 0; i < 10000; i++) {}  // Agrego un pequeño retardo
}

void put_string(const char *s) {
    while (*s) {
        if (*s == '\n') put_char('\r'); // Inserto retorno de carro antes de salto de línea
        put_char(*s++);
    }
}

void put_hex(uint32_t val) {
    char hex[9];
    hex[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        int nibble = val & 0xF;
        hex[i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10; // Convierto el valor a hexadecimal
        val >>= 4;
    }
    put_string(hex);
}
