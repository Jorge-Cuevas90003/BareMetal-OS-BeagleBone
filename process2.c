void put_char(char c);
extern volatile int timer_enabled;
extern volatile int current_process;

void process2_main(void) {
    static char c = '0';
    static int initialized = 0;

    if (!initialized) {
        initialized = 1;
    }

    if (timer_enabled) {
        // Modo preemptivo, imprimo un dígito
        if (current_process == 1) {
            put_char(c);
            put_char(' ');
            c++;
            if (c > '9') {
                c = '0';
            }
            for (volatile int i = 0; i < 50000; i++) {} // Pausa corta para evitar saturación
        }
    } else {
        // Modo cooperativo, imprimo 3 dígitos para simular 1 segundo
        for (int i = 0; i < 3; i++) {
            put_char(c);
            put_char(' ');
            c++;
            if (c > '9') {
                c = '0';
            }
            // Retardo para que cada dígito sea ~1/3 de segundo
            for (volatile int j = 0; j < 800000; j++) {}
        }
    }
}
