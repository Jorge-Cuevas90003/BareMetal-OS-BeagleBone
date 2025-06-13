.section .vectors, "ax"
    .align 2
    .global _reset

_reset:
    b start
    b undefined_instruction
    b software_interrupt
    b prefetch_abort
    b data_abort
    nop
    b irq_handler
    b fiq_handler

start:
    cpsid if
    mov r0, #0x13  // Configuro modo supervisor
    msr cpsr_c, r0
    ldr sp, =0x80008000  // Establezco la pila del supervisor

    mov r0, #'A'
    bl put_char

    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
bss_loop:
    cmp r0, r1
    bge bss_done
    str r2, [r0], #4  // Limpio la sección .bss
    b bss_loop
bss_done:
    bl main  // Llamo a la función principal
    mov r0, #'Z'
    bl put_char
    b .  // Entro en bucle infinito si termino

undefined_instruction:
    mov r0, #'U'
    bl put_char  // Muestro U para instrucción no válida
    b .

software_interrupt:
    stmfd sp!, {r0-r3, lr}
    mov r0, #'S'
    bl put_char  // Muestro S para interrupción de software
    ldmfd sp!, {r0-r3, lr}
    movs pc, lr

prefetch_abort:
    mov r0, #'P'
    bl put_char  // Muestro P para error de precarga
    b .

data_abort:
    mov r0, #'D'
    bl put_char  // Muestro D para error de acceso a datos
    b .

irq_handler:
    sub lr, lr, #4
    stmfd sp!, {r0-r3, r12, lr}
    mov r0, #'I'
    bl put_char  // Muestro I al recibir interrupción
    bl timer_interrupt_handler
    mov r0, #'!'
    bl put_char  // Muestro ! al terminar el manejador
    ldmfd sp!, {r0-r3, r12, pc}^

fiq_handler:
    mov r0, #'F'
    bl put_char  // Muestro F para interrupción rápida
    b .

.global load_context
load_context:
    // r0 contiene el puntero al PCB
    ldmia r0!, {r1-r12}      // Cargo registros r0-r12
    ldr sp, [r0], #4         // Cargo la pila
    ldr lr, [r0], #4         // Cargo el link register
    ldr r1, [r0]             // Cargo el CPSR
    msr cpsr_c, r1           // Establezco el modo
    ldr r0, [r0, #-4]        // Cargo el PC
    bx r0                    // Salto al PC

    .extern put_char
    .extern timer_interrupt_handler
