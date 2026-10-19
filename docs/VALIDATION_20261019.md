# Compilación y comprobaciones — 19 de octubre de 2026

## Ejecutable normal

`dist/default.xex` se compiló con el SDK XDK desde el árbol limpio
`c74481ea3835b47ca54d9e16b9b94f60bd875f07`, con el checkout original
fijado en `6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`.
No lleva BootToMatch, entrada programada, CallTrace ni carrera experimental.

| Archivo | SHA-256 |
|---|---|
| XEX normal | `ED9EC95EEC107D9970323410358D47FF662708F61121610972AFF7A68524A178` |
| Mapa normal | `DDD13E94FF04B0E70BBABF435CAC013B1FEE1BEEB4AC339A0B3E654B0C01C566` |
| Diagnóstico de menús | `06006258F579A26EAC8657F0101C5607A5EB753213CAF0C0AECFE3A4CB713731` |
| Diagnóstico de combate | `0B1D2E37D6B1CC083BD21C4EF449390D27FB32BCAF464419803D85D4EE1ED305` |

Los diagnósticos se enlazan por separado desde los objetos normales:
el primero sustituye PAD; el segundo añade también la entrada directa
de diagnóstico a combate. Sus `build-info.json` conservan los hashes del
XEX y mapa normales. No reemplazan el archivo de `dist`.

## Correcciones

- Selector original de música, probabilidades y petición L/R de todos los
  humanos. El sorteo se hace una vez después de objetos y HUD, conservando
  el orden de consumo del RNG. [Detalle](STAGE_MUSIC_SELECTION.md).
- Adventure recarga los pesos y la escala de objetos de su StKind después
  de elegir el encuentro. Antes podía retener otra fila al reutilizar el
  mismo escenario; el reproducer de host falló antes de corregirlo.
- El test de selección de Classic utiliza ahora el HSD RNG original.
  Los siete combates, aliados, colores y tablas de dificultad proceden del
  port anterior. [Campañas y reglas](CAMPAIGNS_VS_20261016.md).
- Nueva revisión de upstream a `0cc24fe`: once commits de PC/plataforma,
  sin cambios nuevos en juego o baselib. [Auditoría](UPSTREAM_AUDIT_20261016.md).

## Pruebas de host

| Prueba | Cobertura observada |
|---|---|
| `test_campaign_setup.ps1` | 26 personajes × 128 semillas; siete combates y cinco dificultades; veinte fases Adventure × 26 personajes × cinco dificultades |
| `test_match_rules.ps1` | Ciclo de vida, vidas, reintentos, equipos, Stock con reloj, Time sin límite, penalizaciones, orden/caché de música y recarga real de pesos/escala de objetos |
| `test_stage_music.ps1` | Nueve reglas, 32 combinaciones de desbloqueos, 100 tiradas, L/R, 162 filas y temas por personaje |
| `test_audio_xdk.ps1` | 4035 streams en su banco; cero cruces/faltantes; 55 bancos; 36 HPS decodificados hasta sus bucles |
| `test_menu_sources.py` / `test_sis_native.ps1` | Extracción de menús y envío de texto, matrices, UV, color y alpha independiente |

Los servicios de render/contacto/PAD se simulan donde indica cada harness.
Estas pruebas no establecen paridad visual o auditiva ni ejecución en consola.

## Xenia: entradas y selección musical

La pasada final usa los diagnósticos identificados arriba, con audio activo.
Las siete entradas Classic y las veinte Adventure pasaron 1200 cuadros cada
una, conservando los ordinales/escenas y la cantidad esperada de jugadores.
Todas cargaron la música referenciada; los 27 casos terminaron con cero
errores de decodificación GX, cero desajustes de historial ADPCM y sin
watchdog. Son comprobaciones de entrada/ejecución, no de victoria en cada fase.

En VS de Battlefield, la semilla de diagnóstico 31415 alcanzó el BGM
alternativo 38 con `audio.match.alternate: 1`, archivo encontrado y audio
iniciado. Pasó 600 cuadros y registró ocho impactos, sin errores de GX/audio.
Las otras nueve semillas probadas cargaron su pista principal correctamente.
No se modifica el generador ni las probabilidades del ejecutable normal.

Evidencia local de esta compilación:

- `build-x360/menu-port/campaign-items-final/entries/summary.json`
- `build-x360/menu-port/campaign-items-final/vs-music/summary.json`
- `build-x360/menu-port/campaign-items-final/vs-music-spread/summary.json`

Cada caso conserva su mapa/XEX por referencia al diagnóstico canónico,
hashes, guion, configuración, resultado y trace. Las pruebas reutilizan el
directorio de ejecución existente. Para conservar estos registros, el
builder de diagnósticos exige una carpeta de salida nueva o vacía.

## Xenia: menús originales

Los siete casos usan el diagnóstico de menús de esta compilación. Todos
pasaron, con cero errores de GX/historial de audio y sin watchdog:

| Recorrido | Cuadro final | Impactos registrados |
|---|---:|---:|
| Intro → VS → selección → combate | 3000 | 24 |
| Petición L/R desde Stage Select | 3000 | 7 |
| Rules → Stock → combate | 3600 | 24 |
| Volver desde Stage Select | 2100 | 0 |
| Classic desde su selección | 2400 | 7 |
| Adventure desde su selección | 2400 | 0 |
| Resultados → selección conservada | 4500 | 0 |

L/R llegó al selector con flags 68; el escenario elegido no tenía una
alternativa y conservó correctamente su pista principal. La selección
aleatoria de Battlefield descrita arriba comprueba una alternativa existente.
Los resultados y traces están en
`build-x360/menu-port/campaign-items-final/menus`.

Con el XEX normal y sus objetos ya compilados, repite las matrices con una
carpeta `$check` nueva. El directorio `$run` se reutiliza entre pasadas:

```powershell
$check='build-x360/check-20261019'
$run='build-x360/menu-port/full-vs-smoke'
New-Item -ItemType Directory -Force $run | Out-Null
./tools/build_menu_diagnostic.ps1 -OutDir "$check/match-image" -BootToMatch
./tools/build_menu_diagnostic.ps1 -OutDir "$check/menu-image"
./tools/test_campaign_xenia.ps1 -DiagnosticDir "$check/match-image" `
    -OutDir "$check/entries" -BuildDir $run
./tools/test_campaign_xenia.ps1 -DiagnosticDir "$check/menu-image" `
    -OutDir "$check/menus" -BuildDir $run -Menus
```

`-BuildDir` permite reutilizar el directorio de ejecución con la ISO ya
presente. `-CaseName` limita la pasada a casos concretos de la matriz.

## Xenia: objetivos de Adventure e intro normal

Con el diagnóstico de combate final, los pilotos de PAD de Jigglypuff en
Very Easy y semilla 12000 completaron:

| Objetivo | Resultado observado | Último cuadro de bucle registrado |
|---|---|---:|
| Escape de Brinstar | `escape_goal: 2223` y entrada en escena 33 | 2100 |
| Mushroom Kingdom | Dos checkpoints, `goal: 153` y entrada en escena 3; 26 impactos | 4800 |

Los cuadros de bucle se muestrean cada 300; los marcadores de objetivo se
escriben en el momento de la transición. Ambos pilotos pasaron sin errores
de GX/audio ni watchdog. Conservan resultados y traces bajo
`build-x360/menu-port/campaign-items-final/pilots`.

Finalmente se ejecutó el XEX normal de `dist`, con entrada humana: llegó al
cuadro 1800 con 900 imágenes de la intro presentadas, cero errores de vídeo,
cero desajustes de historial de audio, sin watchdog y sin cargar un guion de
entrada. Su hash se mantuvo igual. Evidencia:
`build-x360/menu-port/campaign-items-final/normal-opening/result.json`.

## Alcance de las campañas

Classic sigue siendo una ruta parcial de siete combates anteriores al jefe;
faltan bonus, Master/Crazy Hand y presentación original. El piloto continuo
del diagnóstico de `51419f2` ganó cinco encuentros, reintentó el tercero y
llegó al equipo de diez. Terminó su presupuesto de 360 segundos en ese
sexto encuentro, a cuadro de bucle 21000, sin watchdog ni errores de audio/GX.
El objetivo de completar los siete no se alcanzó. La evidencia conserva
`Pass: false` en
`build-x360/menu-port/campaign-vs-final/pilots/classic-through-seven`.

Las veinte fases de Adventure conservan cursos, oleadas y transiciones
parciales. Carrera/tráfico, escalada y peligros originales siguen pendientes.
Las pruebas de entrada y los pilotos de objetivos comprueban recorridos
concretos; no demuestran una campaña completa, todos los personajes en
cada curso ni juego en una Xbox 360 física. El ZIP alpha.1 publicado conserva
su ejecutable anterior a estos cambios.
