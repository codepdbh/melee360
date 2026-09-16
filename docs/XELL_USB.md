# Prueba en Xbox 360 mediante XeLL

Esta prueba ejecuta el *platform test* de Melee360. Todavía no es el juego
completo: debe mostrar el triángulo de prueba, inicializar el mando, emitir un
tono y buscar una imagen legal de Melee con identificador `GALE01`.

## Preparar el USB

1. Formatear una memoria USB como FAT32.
2. Copiar `dist/xenon.elf` en la raíz de la memoria con el nombre exacto
   `xenon.elf`.
3. Para probar la detección del disco, crear `Melee360` en la raíz y copiar la
   ISO como `Melee360/melee.iso`. Este paso es opcional para el primer arranque.

Estructura recomendada:

```text
USB:/
|-- xenon.elf
`-- Melee360/
    `-- melee.iso       (opcional, no incluido en el repositorio)
```

## Arrancar

1. Apagar completamente la consola.
2. Conectar el USB directamente a la Xbox 360.
3. Iniciar XeLL (normalmente con el botón de expulsión en una consola
   RGH/JTAG; depende de la instalación concreta).
4. XeLL buscará `xenon.elf` en los dispositivos conectados y lo ejecutará.

No hacen falta `KV.bin`, una NAND, el archivo `xbox360-bios-v10.bin` ni un
`default.xex` para esta ruta.

## Resultado esperado

- Imagen de prueba generada por Xenos.
- Lectura del mando Xbox 360.
- Tono corto de aproximadamente 440 Hz.
- Mensaje de detección de `GALE01` si se incluyó la ISO.

Si la pantalla queda negra o XeLL vuelve al cargador, fotografiar la última
línea visible de XeLL y anotar el modelo de placa y la conexión de vídeo. Esa
información permite separar un fallo de carga ELF de uno de inicialización de
Xenos.
