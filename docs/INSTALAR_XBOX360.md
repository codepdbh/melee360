# Instalar y jugar MELEE360 en Xbox 360

Version experimental `v0.1.0-alpha.3`. Probada mediante trazas de Xenia;
la ejecucion en Xbox 360 fisica todavia no esta verificada.
Classic y Adventure siguen incompletos.

## Necesitas

- Una Xbox 360 que ya ejecute homebrew XEX, por ejemplo RGH/JTAG con
  Aurora u otro cargador compatible. Una consola comercial sin modificar
  no puede abrir este ejecutable.
- Un mando Xbox 360 y almacenamiento USB FAT32 o el disco de la consola.
- Tu propia imagen de Melee **NTSC-U 1.02, GALE01**, en ISO sin comprimir
  (imagen completa habitual: 1.459.978.240 bytes). RVZ, WBFS y carpetas de
  archivos extraidos no son compatibles con este lector.

## Copiar y abrir

1. En https://github.com/codepdbh/melee360/releases descarga el asset
   `MELEE360-v0.1.0-alpha.3-xbox360.zip`. Los botones **Source code**
   descargan el codigo, no el juego ejecutable.
2. Extrae el ZIP. Copia tu ISO a la carpeta `MELEE360` y llamala
   exactamente `melee.iso`; activa las extensiones en Windows para evitar
   `melee.iso.iso`. El paquete no incluye el disco.
3. Copia esa carpeta completa al USB o a `Hdd1:/Games/MELEE360/`:

   ```text
   Usb0:/Games/MELEE360/
     default.xex
     melee.iso
     assets/
       sprite_atlas.png
     INSTRUCCIONES.md
     AVISOS.txt
     BUILD.json
     SHA256SUMS.txt
   ```

4. Abre el explorador de archivos del cargador y ejecuta `default.xex`.
   En Aurora puedes agregar la carpeta de juegos a sus rutas de escaneo.
   Referencia del proyecto Aurora: https://phoenix.xboxunity.net/.
5. Pulsa Start para pasar de la intro al titulo y de ahi al menu.
   En VS -> Melee, mueve la mano con el stick y coloca la ficha con A.
   Activa un rival CPU desde su panel HMN/CPU, o conecta otro mando.
   Pulsa Start para abrir Stage Select y A para elegir escenario. Start
   elige uno al azar; B regresa a personajes conservando la configuracion.

Este paquete es **XEX**. No necesita XeLL ni se debe renombrar a `.elf`.
Un enlace de archivo que funciona en tu PC no sustituye la copia completa
de la ISO al USB. No necesitas copiar SDK, mapas, logs ni carpetas de compilacion.

## Controles

| Accion | Mando Xbox 360 |
|---|---|
| Moverse | Stick izquierdo |
| Ataque | A |
| Especial | B |
| Saltar | X / Y o stick arriba |
| C-stick (VS) | Stick derecho |
| Escudo | LB / LT / RT |
| Agarre | RB |
| Pausa / continuar | Start |
| Salir al menu desde la pausa | B |
| Seleccion de personajes / menu al terminar | A / B |

En seleccion: stick mueve la mano, A coloca la ficha, B la retira y X/Y
cambia color. Ajusta CPU y su nivel en cada panel. Rules abre las reglas
originales y sus submenus. Start abre la seleccion de escenario.
Classic/Adventure se abren en 1-P -> Regular Match y conservan su seleccion
original de dificultad y vidas. Un mando adicional se incorpora en seleccion.
Si se desconecta un mando humano, reconectalo y pulsa Start para continuar.
En VS, A en resultados regresa a personajes; B vuelve al menu principal.

## Lo que puedes probar

VS tiene seleccion y combates con IA, objetos, musica y efectos originales
leidos de la ISO. Algunas acciones, escenarios y materiales siguen pendientes.
Classic ofrece siete combates originales, con aliados, gigante y metal;
faltan bonus y jefes. El multiplicador Damage Ratio y el handicap manual
de VS llegan al combate. El martillo conserva su bajada original de plataformas.
Adventure tiene veinte fases implementadas parcialmente, con recorridos de
Mushroom Kingdom, laberinto y escape de Brinstar. Faltan la carrera original,
escalada, peligros y transiciones; la campana completa actual no esta validada.
No es una version final ni tiene paridad completa con GameCube.

Alpha.3 incluye combate panoramico 16:9, apariciones aleatorias de objetos,
puntos originales de Corneria y correcciones de colision de suelo y bordes.
HUD, menus y videos conservan su composicion original. Classic y Adventure
vuelven a usar la consulta original de modo de un jugador, incluido su
tratamiento del C-stick. No se han completado todas las colisiones.

## Si no arranca

- `melee.iso not found`: la ISO debe estar al lado de `default.xex`.
- `disc is not GALE01` o fallo de FST: comprueba region, version y formato
  de tu imagen; renombrar una imagen comprimida no la convierte en ISO.
- El cargador rechaza el XEX: comprueba que la consola puede ejecutar
  homebrew y que copiaste el asset binario completo.
- Pantalla negra o cuelgue: conserva la version y los pasos exactos que
  lo reproducen. La compatibilidad de hardware aun necesita pruebas;
  no hay una solucion de consola confirmada para todos los casos.

En Windows tambien puedes abrir `default.xex` en Xenia con la ISO a su lado.
No uses `--mute=true` si quieres audio. Guia oficial:
https://github.com/xenia-project/xenia/wiki/Quickstart.

Reporta errores en https://github.com/codepdbh/melee360/issues e indica
consola/cargador o version de Xenia, modo, personajes, escenario y version
del port. Estado y codigo: https://github.com/codepdbh/melee360.
