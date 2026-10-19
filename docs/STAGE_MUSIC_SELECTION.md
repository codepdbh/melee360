# Música original de escenarios — 19 de octubre de 2026

La música de combate sigue procediendo de HPS dentro de la ISO del usuario.
No se incluyen recursos extraídos en el repositorio.

## Cambios

`generate_stage_music_sources.py` extrae sin sustituir su algoritmo:

- `Ground_801C24F8` de `gr/ground.c`: probabilidad, condiciones de
  desbloqueo, alternativas ausentes y selección de columnas StageParam.
- `fn_8016E5C0` de `gm/gmvs.c`: todos los participantes humanos deben
  mantener L o R para pedir el tema alternativo.
- `lbAudioAx_8002305C` y su tabla original de temas por personaje:
  resuelven las filas que usan el sentinel −2.

El adaptador mantiene un contexto musical separado de las colisiones.
Rechaza filas ausentes antes de la aserción del original. Guarda el tema
al iniciar el combate: consultar la música cada cuadro no consume RNG ni
vuelve a sortear. Los IDs siguen usando `hps_files` del original.
La selección se realiza después de inicializar objetos y HUD, en el orden
de `fn_8016E730`: el temporizador de aparición de objetos comparte el RNG
con la música y debe consumir primero su tirada.

La selección ordinaria de VS usa flags 4, como `Stage_80225074` fuera de
muerte súbita y Super Sudden Death; L/R añade 0x40. Antes se elegía siempre
`xC`, omitiendo esta lógica. El selector conserva las probabilidades y las
comprobaciones de Marth, Young Link y todos los personajes, según la
disponibilidad actual del port.

## Límites

La persistencia de trofeos y el conjunto completo de escenarios no están
portados. Sus condiciones musicales permanecen desactivadas. Classic y
Adventure conservan su pista principal y el StKind exacto de cada encuentro;
su selección alternativa y los cambios musicales de escenas originales
siguen pendientes. Tampoco se completa con esto AX/SEM ni la mezcla original.

## Pruebas

```powershell
./tools/test_stage_music.ps1
./tools/test_audio_xdk.ps1
```

La primera ejecuta el selector extraído y el adaptador real: nueve reglas,
32 combinaciones de desbloqueos, 100 valores de tirada, con y sin L/R.
También comprueba las 162 filas del disco, sus campos principales y
alternativos, los temas por personaje, las alternativas ausentes y la
petición de todos los humanos. Los servicios de PAD/RNG son controlados
en esta prueba; no constituye una comparación auditiva.

La segunda audita las 4035 muestras resolubles SEM/SSM, los 55 bancos y
decodifica las 36 pistas de estos escenarios a través de su bucle original.
El registro incluye 17 escenarios de VS y cuatro cursos; el curso de
F-Zero permanece detrás de la opción de compilación experimental.
