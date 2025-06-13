#include <stdint.h>

#define UART0_BASE 0x44E09000
#define WDT1_BASE 0x44E35000
#define CM_PER_BASE 0x44E00000
#define CM_WKUP_BASE 0x44E00400
#define DMTIMER2_BASE 0x48040000
#define INTC_BASE 0x48200000

// Registros de control de reloj
#define CM_WKUP_WDT1_CLKCTRL 0xB8
#define CM_PER_UART0_CLKCTRL 0x4C
#define CM_PER_TIMER2_CLKCTRL 0x80
#define CM_PER_CLK_32KHZ 0x148

// Número de interrupción del DMTimer2
#define TIMER2_IRQ 68

struct pcb {
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
    uint32_t sp, lr, pc, cpsr;
};

struct pcb pcbs[2];
volatile int current_process = 0;
volatile int timer_enabled = 0;
volatile int scheduler_tick = 0;

void put_char(char c);
void put_string(const char *s);
void put_hex(uint32_t val);
void process1_main(void);
void process2_main(void);

extern void load_context(struct pcb *pcb);

void schedule(void) {
    current_process = (current_process + 1) % 2;
    scheduler_tick++; // Actualizo el proceso actual y el contador de ticks
}

void timer_interrupt_handler(void) {
    volatile uint32_t *timer = (volatile uint32_t *)DMTIMER2_BASE;
    volatile uint32_t *intc = (volatile uint32_t *)INTC_BASE;

    // Guardo el contexto del proceso en ejecución
    asm volatile (
        "stmfd sp!, {r0-r12, lr}\n"
        "mrs r0, spsr\n"
        "stmfd sp!, {r0}\n"
        "str sp, %0\n"
        : "=m" (pcbs[current_process].sp)
    );

    // Limpio la interrupción para continuar
    timer[0x40 / 4] = 0x02;  // TISR: borro la interrupción por desbordamiento
    intc[0x10 / 4] = 0x01;   // INTC_CONTROL: confirmo el manejo de IRQ

    // Selecciono el siguiente proceso
    schedule();

    // Restauro el contexto del nuevo proceso
    load_context(&pcbs[current_process]);
}

int setup_timer_interrupt(void) {
    volatile uint32_t *cm_per = (volatile uint32_t *)CM_PER_BASE;
    volatile uint32_t *timer = (volatile uint32_t *)DMTIMER2_BASE;
    volatile uint32_t *intc = (volatile uint32_t *)INTC_BASE;
    int timeout;

    put_string("Configurando la interrupción del temporizador...\n");

    // Activo el reloj del temporizador
    put_string("Habilitando el reloj del timer2...\n");
    cm_per[CM_PER_TIMER2_CLKCTRL / 4] = 0x02;
    for (volatile int i = 0; i < 100000; i++) {}

    // Verifico que el reloj esté activo
    timeout = 50000;
    while ((cm_per[CM_PER_TIMER2_CLKCTRL / 4] & 0x03) != 0x02 && timeout-- > 0) {
        for (volatile int i = 0; i < 1000; i++) {}
    }
    if (timeout <= 0) {
        put_string("No se activó el reloj del timer2\n");
        return 0;
    }
    put_string("Reloj del timer2 activado\n");

    // Configuro el temporizador para 1 segundo (24M ciclos a 24 MHz)
    put_string("Ajustando el temporizador...\n");
    timer[0x2C / 4] = 0x00;         // TCLR: detengo el temporizador
    for (volatile int i = 0; i < 10000; i++) {}
    timer[0x40 / 4] = 0x07;         // TISR: limpio todas las interrupciones
    timer[0x38 / 4] = 0xFFE17B00;   // TLDR: configuro para 1 segundo
    timer[0x3C / 4] = 0xFFE17B00;   // TCRR: establezco el contador inicial
    timer[0x28 / 4] = 0x02;         // TIER: permito interrupción por desbordamiento
    put_string("Temporizador configurado para intervalos de 1 segundo\n");

    // Preparo el controlador de interrupciones
    put_string("Configurando el controlador de interrupciones...\n");
    intc[0x10 / 4] = 0x01;          // INTC_CONTROL: habilito interrupciones
    intc[0xB0 / 4] = 0x00;          // INTC_THRESHOLD: prioridad 0
    intc[0x48 / 4] = 0x01;          // INTC_IDLE: sin modo inactivo
    intc[0x40 / 4] = 0x01;          // INTC_ITR0: limpio pendientes
    intc[0x44 / 4] = 0x01;          // INTC_ITR1
    intc[0x48 / 4] = 0x01;          // INTC_ITR2
    intc[0xC8 / 4] = (1 << 4);      // MIR_CLEAR2: desbloqueo IRQ 68
    put_string("Controlador de interrupciones configurado\n");

    // Inicio el temporizador
    put_string("Iniciando temporizador...\n");
    timer[0x2C / 4] = 0x03;         // TCLR: activo y configuro autorecarga
    timer[0x40 / 4] = 0x02;         // Disparo interrupción por desbordamiento
    put_string("Temporizador iniciado\n");
    return 1;
}

void main(void) {
    volatile uint32_t *wdt = (volatile uint32_t *)WDT1_BASE;
    volatile uint32_t *cm_wkup = (volatile uint32_t *)CM_WKUP_BASE;
    volatile uint32_t *cm_per = (volatile uint32_t *)CM_PER_BASE;
    volatile uint32_t *uart = (volatile uint32_t *)UART0_BASE;

    put_char('M');

    // Desactivo el watchdog para evitar interrupciones
    cm_wkup[CM_WKUP_WDT1_CLKCTRL / 4] = 0x02;
    for (volatile int i = 0; i < 10000; i++) {}
    wdt[0x48 / 4] = 0xAAAA;
    while (wdt[0x34 / 4] & (1 << 4)) {}
    wdt[0x48 / 4] = 0x5555;
    while (wdt[0x34 / 4] & (1 << 4)) {}
    put_string("Watchdog desactivado\n");

    // Configuro el UART para salida de texto
    cm_per[CM_PER_UART0_CLKCTRL / 4] = 0x02;
    for (volatile int i = 0; i < 100000; i++) {}
    uart[0x0C / 4] = 0x83;  // LCR: modo configuración
    uart[0x00 / 4] = 26;    // DLL: 115200 baudios
    uart[0x04 / 4] = 0;     // DLH
    uart[0x0C / 4] = 0x03;  // LCR: 8N1
    uart[0x08 / 4] = 0x07;  // FCR: limpio FIFOs
    uart[0x20 / 4] = 0x00;  // MDR1: modo UART
    put_string("UART0 inicializado\n");

    // Preparo los PCBs para los procesos
    pcbs[0].sp = 0x80018000;
    pcbs[0].pc = (uint32_t)process1_main;
    pcbs[0].cpsr = 0x10;  // Modo usuario
    pcbs[0].lr = 0;
    for (int i = 0; i < 13; i++) {
        *(&pcbs[0].r0 + i) = 0;
    }

    pcbs[1].sp = 0x80028000;
    pcbs[1].pc = (uint32_t)process2_main;
    pcbs[1].cpsr = 0x10;  // Modo usuario
    pcbs[1].lr = 0;
    for (int i = 0; i < 13; i++) {
        *(&pcbs[1].r0 + i) = 0;
    }
    put_string("PCBs inicializados\n");

    // Habilito las interrupciones
    asm volatile ("cpsie i");
    put_string("Interrupciones habilitadas\n");

    // Intento configurar el temporizador
    if (setup_timer_interrupt()) {
        put_string("Temporizador configurado correctamente\n");
        timer_enabled = 1;
        put_string("Probando temporizador durante 1 segundo...\n");
        int old_tick = scheduler_tick;
        for (volatile int i = 0; i < 1000000; i++) {
            if (i % 200000 == 0) put_char('.');
            if (scheduler_tick != old_tick) {
                put_string("\nInterrupción del temporizador funcionando\n");
                break;
            }
        }
        if (scheduler_tick == old_tick) {
            put_string("\nInterrupciones del temporizador no funcionan, paso a modo cooperativo\n");
            timer_enabled = 0;
            volatile uint32_t *timer = (volatile uint32_t *)DMTIMER2_BASE;
            timer[0x2C / 4] = 0x00;  // Detengo el temporizador
        }
    } else {
        put_string("Fallo al configurar temporizador, uso modo cooperativo\n");
        timer_enabled = 0;
    }

    put_string("Iniciando planificador round-robin\n");
    current_process = 0;

    if (timer_enabled) {
        put_string("Usando planificación preemptiva con intervalos de 1 segundo\n");
        load_context(&pcbs[0]);  // Inicio el primer proceso
        while (1) {
            // Espero interrupciones en este bucle
            for (volatile int i = 0; i < 1000; i++) {}
        }
    } else {
        put_string("Usando planificación cooperativa con turnos de 1 segundo\n");
        while (1) {
            // Proceso 1
            put_string("[P1]");
            current_process = 0;
            process1_main();
            // Simulo cambio de contexto
            asm volatile (
                "stmfd sp!, {r0-r12, lr}\n"
                "mrs r0, spsr\n"
                "stmfd sp!, {r0}\n"
                "str sp, %0\n"
                : "=m" (pcbs[current_process].sp)
            );

            // Proceso 2
            put_string("[P2]");
            current_process = 1;
            process2_main();
            // Simulo cambio de contexto
            asm volatile (
                "stmfd sp!, {r0-r12, lr}\n"
                "mrs r0, spsr\n"
                "stmfd sp!, {r0}\n"
                "str sp, %0\n"
                : "=m" (pcbs[current_process].sp)
            );
        }
    }
}


//go 0x80000000
//fatload mmc 0:1 0x80000000 os.bin

