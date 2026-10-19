# Revisión de melee-pc — 16 de octubre de 2026

## Nueva consulta — 19 de octubre de 2026

`git fetch --prune origin` actualizó `origin/master` a
`0cc24fee6b8e7dd7a9233daa126829d374fe1f40`: 11 commits después de
`88cda55`, 223 desde nuestra base. Cambian 42 archivos del lanzador,
ajustes F1, teclado, rutas de almacenamiento y empaquetado de PC/Android.
El diff de `src/melee` y `src/sysdolphin` está vacío. El cambio de
`music_stream.cpp` ajusta rutas para instalaciones portables de PC;
no cambia la decodificación HPS ni el backend XAudio2 de Xbox.
No se incorporan esos componentes SDL/RmlUi al XEX. La base y el
backport `memmove` descritos abajo se conservan.

## Consulta anterior — 16 de octubre de 2026

Se ejecutó `git fetch --prune origin` en el checkout existente, sin crear
otra copia del proyecto ni de la ISO.

| Referencia | Revisión |
|---|---|
| Base compilada por MELEE360 | `6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac` |
| `origin/master` consultado | `88cda55cf976f8e69bdffd8645fd5afeefa77631` |
| Avance respecto de la base | 212 commits; sin divergencia local |
| Cambios en código de juego y baselib | 1060 archivos; 41239 inserciones y 41689 eliminaciones |

## Cambios revisados

- [88cda55](https://github.com/999sian/melee-pc/commit/88cda55cf976f8e69bdffd8645fd5afeefa77631):
  la compactación de bancos SFX copiaba regiones solapadas con `memcpy`.
  Se incorpora su cambio a `memmove` al generar `synth_backported.c`.
  El manifiesto `tools/upstream_backports.json` conserva la procedencia;
  el generador exige encontrar exactamente el fragmento original.
- [4d883de](https://github.com/999sian/melee-pc/commit/4d883dea25ac39bfc084f99f59c9efd33e994d82):
  corrige un índice de kerning después de convertir tablas de bytes a
  `TextGlyphMetrics`. Nuestra base todavía usa tablas de bytes y el índice
  multiplicado por dos es correcto en esa representación. Aplicarlo solo
  sobre esta base produciría otro error; esa conversión no se incorpora.
- Los cambios de ARAM y de callbacks de escenario del mismo commit reciente
  corrigen direcciones de memoria de 64 bits en macOS/iOS. El backend XDK
  usa punteros PowerPC de 32 bits y sus propios lectores de archivos.
- La versión nueva también cambia cámara, escenarios, estructuras de
  luchadores y red. Esas modificaciones requieren una migración separada
  del ABI y nuevas pruebas en XDK. No se consideran integradas por haber
  actualizado las referencias remotas.

El checkout base permanece limpio. El build registra su hash y el hash del
manifiesto de correcciones; no depende de descargar `master` al compilar.

## Error local de sonido corregido

El port leía `audio/smash2.sem` (numeración japonesa) junto con los bancos
`audio/us/*.ssm`. Por ejemplo, Dr. Mario US termina justo antes del primer
sample de Falco US; un índice japonés fuera de rango podía terminar allí.

La ruta ahora usa exclusivamente `audio/us/smash2.sem`, como
`setup_audio_lang()` con `LANG_US` en el original. También se elimina la
búsqueda de muestras en bancos ajenos.

Además, `lbAudioAx_80024030` recibía índices de acción de menú y el puente
los trataba como IDs de sonido. Ahora se compilan sus selectores originales:
por ejemplo, la acción 2 envía `0xAE`, con el volumen, panorama y track
originales. Se conservan también los selectores `8002411C` y `800243F4`.

La auditoría de la ISO GALE01 1.02 resolvió 4035 streams. Antes registró
3068 muestras en su banco, 948 en bancos ajenos y 19 sin banco. Después,
las 4035 pertenecen al banco esperado, con cero cruces y cero faltantes.
La regresión compara además los 55 nombres SSM con la tabla original,
comprueba límites SEM y conserva los checks de PCM, volumen, panorama,
tokens y cancelación de voces. Esto verifica selección y envío de audio;
no prueba paridad auditiva de todo el intérprete SEM ni efectos de mezcla.
