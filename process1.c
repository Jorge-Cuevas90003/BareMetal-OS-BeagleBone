void put_char(char c);
extern volatile int timer_enabled;
extern volatile int current_process;

void process1_main(void) {
    static char c = 'a';
    static int initialized = 0;

    if (!initialized) {
        initialized = 1;
    }

    if (timer_enabled) {
        // Modo preemptivo con temporizador, imprimo un carácter
        if (current_process == 0) {
            put_char(c);
            put_char(' ');
            c++;
            if (c > 'z') {
                c = 'a';
            }
            for (volatile int i = 0; i < 50000; i++) {} // Hago una pausa breve para no sobrecargar
        }
    } else {
        // Modo cooperativo, imprimo 3 caracteres para simular 1 segundo
        for (int i = 0; i < 3; i++) {
            put_char(c);
            put_char(' ');
            c++;
            if (c > 'z') {
                c = 'a';
            }
            // Ajusto el retardo para que cada carácter tome ~1/3 de segundo
            for (volatile int j = 0; j < 800000; j++) {}
        }
    }
}
