# Menús y audio originales — desarrollo del 16 de octubre de 2026

Estos cambios pertenecen al código de `main`, después de `v0.1.0-alpha.1`.
El ZIP de aquella prerelease conserva su ejecutable anterior.

## Entrar a una partida

1. Pulsa Start para pasar de la intro al título y después al menú.
2. Entra en **VS → Melee**. La selección ejecuta `mncharsel.c` original:
   mueve la mano con el stick, coloca la ficha con A y retírala con B.
3. Usa X/Y para cambiar el color. Los botones HMN/CPU de cada panel permiten
   activar rivales CPU y sus controles originales permiten ajustar su nivel.
4. **Rules** abre las reglas originales; también están enlazadas sus pantallas
   More Rules, Item Switch y Random Stage Switch. Sus opciones avanzadas aún
   necesitan completar la conexión con el motor de combate.
5. Cuando haya dos participantes preparados, pulsa Start para abrir
   **Stage Select**. Confirma un escenario con A; Start elige uno al azar.
   B regresa a personajes conservando la configuración.
6. En resultados, A regresa a selección con los personajes y colores
   conservados. B vuelve al menú principal. Los resultados siguen usando
   una pantalla provisional del port.

Los escenarios todavía no soportados por el backend aparecen bloqueados.
Coin y Bonus no inician un combate con reglas de otro modo: muestran su
estado pendiente. La edición de nombres también sigue pendiente.

**1-P → Regular Match → Classic / Adventure** abre la selección original
de esos modos y conecta con sus encuentros disponibles. Classic conserva
su vista previa de cinco combates normales. Adventure conserva veinte
fases parcialmente portadas; faltan peligros, transiciones, carrera y
escalada originales. Estos cambios no completan las campañas.

## Implementación

- Se compilan `mncharsel.c`, `mnmainrule.c`, `mnruleplus.c`, `mnitemsw.c`,
  `mnstagesw.c` y `mnstagesel.c` del checkout fijado.
- Se ejecutan la asignación de fichas, selección de colores, menús de CPU,
  animaciones, modelos y recursos originales. El adaptador conserva los
  índices de jugador y distingue CharacterKind de FighterKind.
- El intérprete SIS y su composición de texto son originales. El atlas
  se obtiene de `main.dol` en la ISO del usuario, sin copiarlo al repositorio.
  Solo el envío de los cuádruples GX pasa al render D3D9/Xenos.
- El controlador restaura la configuración de `gmMain_8015FD24`: stick de
  80 unidades, gatillos analógicos de 140 y normalización correspondiente.
  LB conserva el escudo digital; LT/RT llegan al pulsador digital al final
  del recorrido.
- Se respetan los colores de equipos elegidos en CSS. El final de Stock
  detecta el último equipo vivo y Time suma las puntuaciones por equipo.
- Los sonidos US utilizan su propia tabla SEM y banco SSM. Los selectores
  de acciones de menú se extraen del original, incluyendo IDs y tracks.
  [Auditoría de sonido y upstream](UPSTREAM_AUDIT_20261016.md).

## Música de cada escenario

La pista principal se obtiene de `grGroundParam::stage_params` del escenario:
VS usa su columna VS y Classic/Adventure la fila de su `StKind` y columna
de un jugador. El ID se convierte a archivo con `hps_files` original.
Por ejemplo, Fountain usa `izumi.hps`, Corneria `corneria.hps`, Onett
`onetto.hps` y Final Destination `sp_end.hps`.

La auditoría lee los 21 archivos de escenario disponibles, sus 162 filas y
las 36 pistas referenciadas, incluidas alternativas. Todas están presentes
en la ISO GALE01 1.02. La regresión decodifica cada pista completa, cruza su
punto de bucle original y comprueba PCM no silencioso y cero desajustes de
historial ADPCM. No extrae música al repositorio.

La elección aleatoria/condicional de música alternativa y su activación con
L/R todavía están pendientes; disponer del archivo no implementa esa lógica.

## Repetir las comprobaciones

```powershell
python tests/test_menu_sources.py
./tools/test_sis_native.ps1
./tools/test_audio_xdk.ps1
./tools/test_match_rules.ps1
./tools/build_xex.ps1
./tools/build_menu_diagnostic.ps1 -OutDir build-x360/menu-check
./tools/smoke_xenia_menu.ps1 -BuildDir build-x360/menu-check `
    -ScriptPath tests/fixtures/menu_vs_original.txt
```

El diagnóstico conserva el inicio normal y sustituye solamente PAD por
entrada programada. Usa una carpeta nueva por prueba. Las otras fixtures
cubren Rules/Stock, volver desde escenarios, Classic, Adventure y regresar
de resultados al selector; requieren
ajustar `-Expect`, `-MinHits`, `-ExpectedFlow` y `-TargetFrame` según el caso.
`result.json` conserva hashes del XEX, mapa y guion junto al registro.

La pasada final en Xenia del 16 de octubre completó estos recorridos:

| Recorrido | Cuadro final | Resultado |
|---|---:|---|
| VS desde intro y menús | 3000 | PASS; 24 impactos |
| Rules → Stock → combate | 3600 | PASS; 24 impactos |
| Volver desde Stage Select | 2100 | PASS; regreso al menú |
| Classic desde su selector | 2400 | PASS; primer encuentro |
| Adventure desde su selector | 2400 | PASS; primera ruta |
| Stock de una vida → resultados → selector | 4500 | PASS; selección conservada |

Los seis casos usan el mismo XEX de diagnóstico, con cero errores de
decodificación GX, cero desajustes del historial de audio y sin watchdog.
La última fixture es `tests/fixtures/menu_result_return.txt`; comprueba
`match.select.resume: 0`, con `-MinHits 0 -TargetFrame 4500`. El registro
conserva Mario humano, Luigi CPU y los otros dos espacios cerrados antes
y después de la partida.

El XEX normal, sin entrada programada, también pasó una prueba de intro de
1800 cuadros: 985 imágenes presentadas, cero errores de vídeo y cero
desajustes de audio. Las evidencias locales están en
`build-x360/menu-port/oct16-matrix` y
`build-x360/menu-port/final-normal-opening`.

Estas comprobaciones cubren el recorrido y los datos enviados al render/audio.
La comparación visual y auditiva completa con GameCube y la ejecución en una
Xbox 360 física siguen pendientes.
