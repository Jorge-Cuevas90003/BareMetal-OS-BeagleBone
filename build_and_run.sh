#!/bin/bash
set -e
echo "Compilando el sistema operativo para BeagleBone Black con planificador Round Robin..."

# Elimino archivos anteriores para empezar desde cero
rm -f *.o *.elf *.bin *.dis *.map

# Compilo el archivo de ensamblador en modo ARM para los vectores
echo "Compilando root.s..."
arm-none-eabi-gcc -c -mcpu=cortex-a8 -marm -O0 -nostartfiles -nostdlib -nodefaultlibs -fno-builtin -g root.s -o root.o || { echo "Error al compilar root.s"; exit 1; }

# Compilo los archivos en C en modo Thumb para mejor eficiencia
echo "Compilando os.c..."
arm-none-eabi-gcc -c -mcpu=cortex-a8 -mthumb -O0 -nostartfiles -nostdlib -nodefaultlibs -fno-builtin -g os.c -o os.o || { echo "Error al compilar os.c"; exit 1; }

echo "Compilando stdio.c..."
arm-none-eabi-gcc -c -mcpu=cortex-a8 -mthumb -O0 -nostartfiles -nostdlib -nodefaultlibs -fno-builtin -g stdio.c -o stdio.o || { echo "Error al compilar stdio.c"; exit 1; }

echo "Compilando process1.c..."
arm-none-eabi-gcc -c -mcpu=cortex-a8 -mthumb -O0 -nostartfiles -nostdlib -nodefaultlibs -fno-builtin -g process1.c -o process1.o || { echo "Error al compilar process1.c"; exit 1; }

echo "Compilando process2.c..."
arm-none-eabi-gcc -c -mcpu=cortex-a8 -mthumb -O0 -nostartfiles -nostdlib -nodefaultlibs -fno-builtin -g process2.c -o process2.o || { echo "Error al compilar process2.c"; exit 1; }

# Enlazo todos los objetos para crear el ejecutable
echo "Enlazando..."
arm-none-eabi-ld -T linker.ld root.o os.o stdio.o process1.o process2.o -o os.elf -Map os.map || { echo "Error al enlazar"; exit 1; }

# Genero el binario para BeagleBone
echo "Creando binario..."
arm-none-eabi-objcopy -O binary os.elf os.bin || { echo "Error al crear os.bin"; exit 1; }

# Creo el desensamblado para depuración
echo "Generando desensamblado..."
arm-none-eabi-objdump -d os.elf > os.dis || { echo "Error al crear os.dis"; exit 1; }

# Muestro el tamaño del binario
echo ""
echo "Compilación terminada!"
echo "Tamaño de os.bin:"
ls -lh os.bin

echo ""
echo "Distribución de memoria:"
arm-none-eabi-size os.elf

echo ""
echo "Para cargar en BeagleBone Black:"
echo "1. Copio os.bin a la tarjeta SD como 'os.bin'"
echo "2. Uso estos comandos en U-Boot:"
echo "   => fatload mmc 0:1 0x80000000 os.bin"
echo "   => go 0x80000000"

echo ""
echo "Comportamiento esperado:"
echo "- Intento primero planificar con temporizador (intervalos de 1 segundo)"
echo "- Si el temporizador falla, cambio a modo cooperativo"
echo "- Proceso 1 imprime: 1: a b c d e..."
echo "- Proceso 2 imprime: 2: 0 1 2 3 4..."
echo "- Los procesos se alternan cada ~1 segundo"
