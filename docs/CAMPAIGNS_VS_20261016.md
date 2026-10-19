# Campañas y reglas VS — 16 de octubre de 2026

Estos cambios corresponden a `main`, después del ZIP alpha.1.

## Classic

La secuencia provisional de cinco combates normales se sustituye por los
siete encuentros de combate anteriores al jefe en `gmclassic.c`:

| Ordinal original | Encuentro |
|---:|---|
| 0 | Individual |
| 1 | Equipos: aliado CPU y dos rivales |
| 3 | Individual |
| 4 | Gigante: dos aliados CPU y un rival gigante |
| 6 | Individual |
| 7 | Equipo de diez rivales, tres activos |
| 9 | Rival metálico en el StKind 175 original |

El generador extrae las tablas, la mezcla de candidatos y la selección de
`gmClassic_InitMatchupOrder`, `gmClassic_801B2BA4` y `gmClassic_801B2D54`.
Conserva sus comprobaciones de personajes, escenarios y repeticiones.
Los cuatro bucles de error del original se convierten en un retorno de
indisponibilidad para evitar colgar el port si no hay candidatos soportados.

`gm_8017DB88`, su selector de aliados y `fn_8017DD7C` también son código
original extraído. El adaptador aplica a los jugadores nativos los colores,
equipos, niveles, comportamientos, escalas y proporciones originales de
`gm_17EB.c`. Los reintentos conservan la ruta elegida y las vidas restantes.

Los bonus, Master Hand/Crazy Hand, introducciones, créditos y guardado siguen
pendientes. La ruta termina como vista previa después del rival metálico.
El equipo de diez todavía reutiliza tres slots en lugar del generador original.

## Adventure

Las veinte fases disponibles usan sus ordinales originales y los selectores
de `gm_17E4.c`, sin desplazar la dificultad por los cursos todavía omitidos.
Se conserva la lista vacía de aliados de Adventure: el flag de gigante no
crea aliados por sí solo. Se restaura también la frecuencia de objetos de
cada encuentro; antes el adaptador los desactivaba en toda campaña.

No se completan con esto la escalada, tráfico de la carrera, peligros del
laberinto ni transiciones originales. Los cursos y las oleadas conservan sus
adaptaciones parciales. La campaña completa en consola sigue sin validar.

## VS

El tipo de partida se distingue de la existencia de un reloj. Stock con
límite de tiempo mantiene vidas finitas y decide por vidas restantes; Time
sin límite mantiene reapariciones. Las puntuaciones de equipos se suman y
solo los líderes empatados pasan a muerte súbita.

Se conecta Friendly Fire y la penalización original de autodestrucción
(0, −1 o −2), incluido el tratamiento de bajas entre compañeros. Un mando
ya no puede apropiarse de un slot CPU durante el combate. El HUD conserva
Stock con reloj y el indicador de equipos.

## Comprobaciones

```powershell
./tools/test_campaign_setup.ps1
./tools/test_match_rules.ps1
```

La primera prueba ejecuta selección y configuración original para los 26
personajes, 128 semillas por personaje, siete combates y cinco dificultades.
Desde el 19 de octubre usa también `HSD_Rand`/`HSD_Randi` extraídos del
original. Cubre además las veinte fases de Adventure, los 26 personajes y
las cinco dificultades, incluidos sus niveles, ratios y objetos.
La segunda ejecuta las funciones nativas de entrada, combate, fin, reintento
y salida junto con el módulo original de campañas. Simula posiciones,
render, carga y contactos; cubre reglas y ciclo de vida, no paridad visual
ni una partida real completa.
