# MELEE360

Port experimental del código reconstruido de **Super Smash Bros. Melee para Xbox 360**.
La ruta activa usa `default.xex`, PowerPC, D3D9/Xenos y XInput. El objetivo
es conservar el comportamiento y los recursos originales del juego.

[Descargar prerelease](https://github.com/codepdbh/melee360/releases) ·
[Estado técnico](docs/STATUS.md) · [Trabajo pendiente](docs/PLAYABLE_PORT_GAPS.md) ·
[Criterios de fidelidad](docs/PORT_FIDELITY.md)

## Estado de esta versión

**`v0.1.0-alpha.2` es experimental.** Hay pruebas automatizadas de ejecución
en Xenia Canary; todavía no se ha validado esta versión en una Xbox 360 física.
El proyecto no incluye la ISO ni recursos extraídos del disco.

| Área | Disponible / pendiente |
|---|---|
| Inicio | Intro desde la ISO, título, menús y selección original con mano y fichas |
| VS | Personajes, colores, escenarios, reglas, CPU y jugadores locales; paridad completa pendiente |
| Combate | Estados originales, ataques, daño, vidas, reaparición, IA y objetos; quedan acciones y colisiones por portar |
| Audio | Música, voces y efectos originales decodificados de HPS/SSM; envío a XAudio2 probado y audio confirmado por el usuario en Xenia |
| Classic | Siete combates originales, con aliados, gigante y metal; faltan bonus y jefes |
| Adventure | Veinte fases parcialmente implementadas; Mushroom Kingdom, laberinto y escape tienen pruebas de objetivo en Xenia; campaña actual completa sin validar |
| Presentación | Modelos, animaciones y texturas originales; materiales, cámara y efectos aún incompletos |
| Xbox 360 física | Compilación XEX disponible; ejecución, rendimiento y controles por validar en consola |

Adventure todavía necesita la carrera y escalada originales, peligros del
laberinto y transiciones. No presentamos los combates provisionales como
el modo original completo. Consulta el [informe de pendientes](docs/PLAYABLE_PORT_GAPS.md).

### Cambios de alpha.2

La compilación actual incorpora la **selección de personajes, Rules y Stage
Select originales**, con mano y fichas. El texto SIS usa las fuentes de tu
disco. [Recorrido y controles actuales](docs/ORIGINAL_MENUS_AUDIO.md).

También corrige el cruce entre la tabla de sonido japonesa y los bancos US:
4035 streams se resuelven ahora en su banco correcto. Se restauran los
selectores originales de sonidos de menú y la escala original del mando.
La música usa la tabla de cada escenario. VS ejecuta el selector original
de temas alternativos y la petición con L/R de todos los jugadores humanos.
Se verificaron las 36 pistas referenciadas por los 21 archivos de escenario examinados,
incluidos sus bucles. Los desbloqueos por trofeos/plantilla completa de
escenarios y la elección alternativa en campañas siguen pendientes.
[Selección de música y límites](docs/STAGE_MUSIC_SELECTION.md).
[Revisión del fork y correcciones incorporadas](docs/UPSTREAM_AUDIT_20261016.md).

El ZIP `v0.1.0-alpha.2` incluye estos menús y controles. Classic y Adventure
siguen incompletos; no se ha probado en consola física.

Classic ahora enlaza siete combates de su secuencia original, con aliados,
equipos, gigante y metal. Classic y Adventure usan los selectores originales
de nivel, comportamiento CPU, colores, proporciones y frecuencia de objetos.
VS respeta el reloj de Stock, Time sin límite y las reglas de autodestrucción
y daño entre compañeros. El multiplicador Damage Ratio y el hándicap elegido
se transmiten al combate; el martillo utiliza su transición original para
bajar de plataformas. [Detalle y límites](docs/CAMPAIGNS_VS_20261016.md).
[Compilación y comprobaciones del 19 de octubre](docs/VALIDATION_20261019.md).

Los últimos cambios corrigen la articulación de las colisiones del escenario,
el doble desplazamiento en plataformas y restauran funciones originales de
colisión, escala y ajuste de piernas. El render almacena los vértices GX
inmutables ya decodificados y aplica animaciones/materiales en cada dibujo.
Una prueba de cuatro CPU con objetos en Fountain pasó de 33,2 a 16,8 ms por
cuadro en Xenia, con contadores muestreados iguales. Es una medición concreta,
no una garantía de 60 FPS ni una prueba visual de paridad o de consola.

## Instalar en Xbox 360

### Requisitos

- Xbox 360 que **ya pueda ejecutar homebrew XEX**, por ejemplo RGH/JTAG con
  Aurora u otro cargador compatible. Una consola comercial sin modificar
  no puede ejecutar este archivo.
- Mando Xbox 360 y USB FAT32 o disco duro de la consola.
- Tu propia imagen de **Melee NTSC-U 1.02 (`GALE01`)**, ISO completa sin
  comprimir; la imagen habitual ocupa **1.459.978.240 bytes**. El lector
  actual no abre RVZ, WBFS ni una carpeta con archivos extraídos.

### Descarga y copia

1. Abre [Releases](https://github.com/codepdbh/melee360/releases) y descarga
   el asset **`MELEE360-v0.1.0-alpha.2-xbox360.zip`**. **Source code** es el
   código fuente y no contiene el ejecutable listo para usar.
2. Extrae el ZIP y coloca tu ISO dentro de `MELEE360`, llamada exactamente
   **`melee.iso`**. Activa las extensiones de Windows para evitar `.iso.iso`.
3. Copia la carpeta completa al USB o a `Hdd1:/Games/MELEE360/`:

   ```text
   Usb0:/Games/MELEE360/
     default.xex
     melee.iso                 ← tu imagen; no viene en la descarga
     assets/
       sprite_atlas.png
     INSTRUCCIONES.md
     AVISOS.txt
     BUILD.json
     SHA256SUMS.txt
   ```

4. En el explorador de archivos de Aurora u otro cargador compatible,
   abre **`default.xex`**. Si quieres añadirlo a la biblioteca de Aurora,
   incluye la carpeta de juegos entre sus rutas de escaneo.
   [Proyecto oficial Aurora](https://phoenix.xboxunity.net/).
5. Pulsa **Start** para pasar de la intro al título y luego al menú.
   En **VS → Melee**, mueve la mano con el stick y coloca la ficha sobre
   un personaje con **A**. Activa un rival CPU desde su panel o conecta
   otro mando. Pulsa **Start** para abrir Stage Select y **A** para elegir
   escenario; **Start** selecciona uno al azar. **B** vuelve a personajes.

Esta prerelease usa **XEX**: no requiere XeLL ni renombrar el archivo a ELF.
Copia la ISO completa al dispositivo; un enlace local de Windows no basta.
El paquete incluye una guía de instalación y hashes SHA-256; no necesitas
copiar SDK, logs, mapas ni carpetas de compilación.
[Guía independiente](docs/INSTALAR_XBOX360.md).

### Controles

| Acción | Mando Xbox 360 |
|---|---|
| Movimiento | Stick izquierdo |
| Ataque normal | A |
| Especial | B |
| Salto | X / Y o stick arriba |
| Ataques del C-stick | Stick derecho |
| Escudo | LB / LT / RT |
| Agarre | RB |
| Pausa / continuar | Start |
| Volver al menú desde pausa | B |
| Selección de personajes / menú al terminar | A / B |

En selección, mueve la mano con el stick, coloca/retira la ficha con A/B y
cambia color con X/Y. Configura los rivales y su nivel desde los paneles
HMN/CPU. **Rules** abre las reglas originales; **Start** abre escenarios.
En Classic/Adventure se ajustan dificultad y vidas desde sus controles originales.
Los mandos adicionales se incorporan en selección. Si se desconecta un
mando humano durante el combate, reconéctalo y pulsa Start.
[Recorrido y controles de los menús](docs/ORIGINAL_MENUS_AUDIO.md).

### Problemas comunes

| Problema | Qué comprobar |
|---|---|
| `melee.iso not found` | La ISO debe estar junto a `default.xex`; comprueba nombre y extensiones |
| `disc is not GALE01` / FST | Región, versión y formato de la imagen; renombrar un RVZ no lo convierte en ISO |
| El cargador rechaza el XEX | Capacidad de ejecutar homebrew y copia completa del asset binario |
| Pantalla negra o cuelgue en consola | Anota modelo, cargador y pasos; la ejecución física de esta versión sigue sin verificar |
| Classic / Adventure no coincide con GameCube | Son modos incompletos; consulta las fases pendientes arriba |

Reporta fallos en [Issues](https://github.com/codepdbh/melee360/issues) con
versión del port, consola/cargador o versión de Xenia, modo, personajes,
escenario y pasos para reproducirlos.

## Usar en Xenia

Pon `melee.iso` al lado de `default.xex` y abre el XEX en Xenia.
La [guía oficial de Xenia](https://github.com/xenia-project/xenia/wiki/Quickstart)
explica cómo abrir ejecutables y los requisitos de Windows.
Para audio, evita `--mute=true`. Desde este repositorio:

```powershell
./tools/run_xenia.ps1
```

El launcher habilita XAudio2; `-Mute` lo silencia. La prerelease usa el mando
real: no contiene el piloto de las pruebas automáticas.

## Compilar el port

La base principal es [`999sian/melee-pc`](https://github.com/999sian/melee-pc),
fijada en `6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`. La lógica reconstruida
se compila para PowerPC Xbox 360 y los servicios de GameCube se adaptan
progresivamente a la plataforma. El flujo activo utiliza el SDK/XDK local;
LibXenon/XeLL es una ruta histórica separada.

Necesitas Git, PowerShell y una instalación completa del SDK de Xbox 360
que estés autorizado a utilizar. El SDK no viene en el repositorio.
Desde la raíz del checkout, si todavía no tienes la base principal:

```powershell
git clone https://github.com/999sian/melee-pc.git upstream/melee-pc
git -C upstream/melee-pc checkout 6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac
$env:XEDK = 'C:\Program Files (x86)\Microsoft Xbox 360 SDK'
./tools/check_xdk.ps1
./tools/build_xex.ps1
```

El resultado normal es `dist/default.xex` y su registro `dist/build-info.json`.
Copia tu ISO a `dist/melee.iso` para ejecutarlo. Compila **sin** `-BootToMatch`,
`-InputScript`, `-CallTrace` ni `-ExperimentalRace` para jugar.
Las opciones de diagnóstico son exclusivas de pruebas y el empaquetador
rechaza esas variantes.

```powershell
./tools/package_prerelease.ps1 -Version v0.1.0-alpha.2
```

El empaquetador exige un checkout limpio y una compilación normal del mismo
commit, comprueba los hashes y crea un ZIP con una lista explícita de archivos.
No busca ni copia imágenes del disco. Las pruebas y el registro de compilación
están separados del paquete jugable.

## Documentación y tamaño del proyecto

- [Estado y evidencias](docs/STATUS.md)
- [Pendientes jugables](docs/PLAYABLE_PORT_GAPS.md)
- [Fidelidad al original](docs/PORT_FIDELITY.md)
- [Compilación XEX](docs/XEX_BUILD.md)
- [Dependencias y revisiones](docs/UPSTREAMS.md)
- [Limpieza y recuperación de archivos](docs/WORKSPACE_CLEANUP.md)
- [Ruta histórica XeLL/ELF](docs/XELL_USB.md)

La limpieza retiró checkouts antiguos, cachés y pruebas obsoletas. Se conserva
una sola ISO física local, con nombres enlazados para fuente y ejecución.
Las evidencias antiguas están comprimidas y los cambios del emulador anterior
se guardaron como patch. No hace falta recuperar esos checkouts para compilar
el XEX. Las carpetas de compilación y los recursos del disco no se publican.

## Código y avisos

Melee y sus recursos pertenecen a Nintendo/HAL Laboratory. La base reconstruida
no ofrece una licencia de redistribución para el código del juego; sus adiciones
del port tienen condiciones separadas. Consulta el
[LICENSE.md de la base fijada](https://github.com/999sian/melee-pc/blob/6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac/LICENSE.md)
y las [dependencias](docs/UPSTREAMS.md). No se declara una licencia única para
todo el proyecto. El paquete no contiene ISO, recursos del disco, SDK ni claves.
