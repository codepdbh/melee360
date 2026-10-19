# Candidato alpha.2 — validación del 19 de octubre de 2026

## Identidad del paquete

Compilación normal desde el árbol limpio `c527ad49211d5758fdeedc2082e03544d4d55f4a`;
base fijada `6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`. Sin BootToMatch,
entrada programada, CallTrace o carrera experimental. El diagnóstico de menús
sustituye PAD; el de combate añade entrada directa. Ambos conservan en su
registro la identidad del XEX normal y no sustituyen el de `dist`.

| Archivo | SHA-256 |
|---|---|
| XEX normal | `2CA5D0DB8B37E1F347CC2CD1991DBE547084B7CA9292A4663EF75817A90CC92D` |
| Mapa normal | `2992856782C313FC3EBDDC8AD7080F8E9C533AC312B8FBAAC8B78B937787002E` |
| Diagnóstico de menús | `2795CE414D61125C774AB2A9DDCD977957EAA6FE26546448A3EC9FCC96E70A0E` |
| Diagnóstico de combate | `17A5E57036F6B9535E7E3046A676DB9FAB28574AA26EBE5E5F17DF216AF6EC00` |
| ZIP alpha.2 | `3df7d8e983392b492ea4dc6344cf404c59f88a89a997fa285c24d1afda14cca6` |

ZIP local: `build-x360/releases/v0.1.0-alpha.2/MELEE360-v0.1.0-alpha.2-xbox360.zip`,
3.330.300 bytes, seis entradas verificadas individualmente con SHA-256.
Incluye XEX normal, atlas de interfaz, instrucciones, avisos y procedencia;
no incluye ISO, datos extraídos del disco, SDK ni scripts de entrada.
Su publicación está pendiente; la release publicada continúa siendo alpha.1.

## Código nuevo

- `gm_8016B248` transmite `StartMeleeRules.x30`, que `gm_80167BC8`
  calcula desde Damage Ratio. El adaptador anterior devolvía siempre 1,0.
  Se conserva el uso original del valor en las fórmulas de retroceso.
- `Player_GetHandicap` obtiene el hándicap del jugador configurado; antes
  devolvía siempre 9. Las proporciones ataque/defensa se conservan separadas.
  Los estados originales de agarre, sueño y enterramiento consultan este valor.
- Classic/Adventure conservan sus valores de configuración y no heredan
  el multiplicador de una partida VS anterior.
- Se compila `ftCo_0C60.c` original para la transición de bajada de plataforma
  con martillo; se retira el sustituto que devolvía siempre falso.
- README y guías describen mano/fichas, Rules, Stage Select y retorno a CSS.
  El empaquetador verifica también la identidad del manifiesto de backports.

## Host

Pasan `test_match_rules.ps1`, `test_hammer_pass.ps1`,
`test_campaign_setup.ps1`, `test_stage_music.ps1`, `test_audio_xdk.ps1`,
`test_sis_native.ps1` y los cuatro casos de `tests/test_menu_sources.py`.

Reglas: todos los valores 0,5–2,0 del menú, hándicaps 1–9 en los cuatro
puestos, Sudden Death, aislamiento de campañas y llamadas de los adaptadores
reales. El test del martillo simula la consulta de plataforma y verifica
HammerFall y restauración del estado; no prueba el objeto completo en consola.
Campañas: 26 personajes × 128 semillas en Classic; siete encuentros por cinco
dificultades; veinte fases Adventure × 26 personajes × cinco dificultades.
Audio: 4.035 streams SEM en su banco propio, 55 límites de banco, envíos PCM,
162 filas de música y 36 HPS completos, incluidos sus bucles originales.

## Xenia

Las 38 comprobaciones pasan sin errores de decodificación/historial de audio
ni watchdog. Cada resultado conserva el hash del XEX y del mapa; se verificó
que corresponde a los dos diagnósticos o al ejecutable normal de este informe.
Evidencia local: `build-x360/menu-port/alpha2/`.

- **27 entradas de campaña**: siete Classic y veinte Adventure, 1.200 cuadros
  cada una, ordinal/escena y número de participantes esperados, pista encontrada
  y música iniciada. No son victorias de los 27 encuentros.
- **8 recorridos de menús**: la prueba nueva de Damage Ratio y las siete
  rutas de VS, L/R, Stock, volver de escenarios, Classic, Adventure y resultados.

| Recorrido | Cuadros de bucle | Impactos |
|---|---:|---:|
| vs | 3000 | 24 |
| vs-music-lr | 3000 | 7 |
| rules-stock | 3600 | 24 |
| stage-back | 2100 | 0 |
| classic | 2400 | 7 |
| adventure | 2400 | 0 |
| results | 4500 | 0 |
| rules-damage (selector original a 1,1) | 3600 | 31 |

El trace de `rules-damage` confirma `match.rules.knockback_percent: 110`.
Las entradas de campaña conservan 100 y hándicap 9. Los resultados regresan
a CSS con los puestos, personajes y reglas; B desde escenarios vuelve a CSS.

- **2 objetivos con piloto de mando**: escape de Brinstar alcanza
  `match.adventure.escape_goal: 2223` y carga la escena 33; Mushroom Kingdom
  completa ambos checkpoints, registra 26 impactos y entra en la escena 3.
  Su `match.adventure.goal: 153` es el índice del punto de salida, no un cuadro.
  Últimas muestras de bucle: 2.100 y 4.800. No se modifican posiciones, vidas,
  daño o CPU para obtener la victoria.

- **XEX normal**: 1800 cuadros de bucle y 898 imágenes de intro
  presentadas, cero errores de película, sin cargar guion de entrada.
  El hash normal se mantiene idéntico al del ZIP.


## Alcance

Son pruebas de entradas, recorridos y objetivos concretos. No demuestran una
campaña completa ni paridad visual/audio. Classic omite bonus y jefes;
Adventure mantiene veinte fases parciales, sin carrera/escalada, peligros y
transiciones completos. Colisiones, CPU, resultados, nombres y guardado
siguen pendientes. Xbox 360 física no se ha validado.

La consulta del fork confirmó que su HEAD/default `master` continúa en
`0cc24fee6b8e7dd7a9233daa126829d374fe1f40`, ya auditado; `main` del fork
es una rama anterior (`1627cf1`), no una actualización del juego.
No se cambió el checkout base ni se incorporaron dependencias de PC.
[Auditoría anterior](UPSTREAM_AUDIT_20261016.md).
