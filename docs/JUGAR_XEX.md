# Probar una partida local

El XEX tiene un recorrido de intro, pantalla de titulo, menu y partida VS.
Es una version experimental: las pruebas automatizadas corren en Xenia;
el funcionamiento visual, de audio y con mandos en Xbox 360 aun necesita
verificacion. Classic y Adventure siguen siendo secuencias provisionales.

Classic ya elige parejas originales de escenario y rival entre los escenarios
portados. Sus combates normales tienen cinco minutos y vidas limitadas. Si
se acaba el tiempo, pierdes una vida y A permite repetir el mismo encuentro
mientras te queden vidas. Si las agotas, A ofrece continuar con tres vidas
en el mismo encuentro. La secuencia de cinco combates sigue siendo una
vista previa: no incluye todavia las fases especiales ni el jefe final.

Adventure tiene veinte fases, incluido el laberinto experimental. En Mushroom Kingdom, avanza a la derecha,
derrota a los diez Yoshis del checkpoint y alcanza la salida. Este recorrido
completo ya paso una prueba de Xenia con una vida restante. Brinstar tambien
tiene una fase de escape: sube a la plataforma superior antes de cuarenta
segundos. La subida completa ya paso en Xenia sin perder vidas, y despues
cargo el siguiente combate contra Kirby. En Underground Maze, busca la
Trifuerza entre seis salas: los simbolos falsos inician un combate con Link.
Al derrotarlo puedes seguir explorando; tienes siete minutos por intento.
Esta version corrige el paso entre salas despues de vencer a Link.
Tambien restaura el calculo original de velocidad de las superficies para los objetos.
Usa las tablas originales de materiales para la friccion y la seleccion de sonidos y efectos al pisar o caer.
Tambien recupera las funciones originales de escala del modelo y los datos de camara de los luchadores.
Las consultas de suelo reconocen los enlaces originales entre segmentos; los objetos tambien reciben la identidad correcta del escenario y su variante de campaña.
Una prueba de Xenia completo el laberinto despues de un timeout y reintento.
Faltan sus enemigos del recorrido y animaciones de transicion. La carrera
y la escalada originales siguen pendientes.
Una prueba continua completo las diecisiete fases obligatorias disponibles
en Easy con una version anterior sin laberinto, con tres Continues y vuelta
al menu. Giant Kirby y Giga Bowser dependen de condiciones originales.
La secuencia completa de veinte fases no se ha validado todavia, ni la
ejecucion en Xbox 360. Consulta
`PLAYABLE_PORT_GAPS.md` para el estado y la version de cada prueba.

Los enemigos de Adventure usan los valores originales de ataque, resistencia
y nivel y comportamiento de IA de cada encuentro. En la seleccion de Aventura,
LB cambia entre Very Easy, Easy, Normal, Hard y Very Hard. Giga Bowser
solo aparece desde dificultad Normal y al completar
los combates anteriores en menos de dieciocho minutos acumulados.

## Archivos

Coloca estos dos archivos en la misma carpeta del dispositivo:

- `default.xex`, generado en `dist/`.
- Tu imagen de Melee NTSC-U 1.02 (`GALE01`), llamada `melee.iso`.

En esta PC, `dist/melee.iso` es un enlace local a la imagen existente. Al
pasarlo a otro dispositivo, copia el archivo ISO completo. El juego no
descarga recursos ni incluye los datos del disco en el repositorio.

## Entrar a VS

1. Abre `default.xex` en Xenia o en un cargador compatible con XEX en tu consola.
2. Pulsa Start para saltar la intro al titulo y otra vez para entrar al menu.
3. Selecciona VS y su primera opcion de combate.
4. Elige tu personaje con izquierda/derecha y confirma con A.
5. Si juegas solo, elige tambien el personaje de la CPU y confirma con A.
   Con un segundo mando conectado, P2 elige y confirma su propio personaje.
6. Espera la cuenta inicial. Al terminar la partida, A inicia una revancha
   con los mismos personajes, colores y reglas; B vuelve al menu principal.

## Controles del mando Xbox

| En combate | Control |
|---|---|
| Moverse | Stick izquierdo |
| Ataque normal | A |
| Especial | B |
| Saltar | X / Y, o stick arriba |
| Ataque con C-stick | Stick derecho |
| Escudo | LB / LT / RT |
| Agarre | RB |
| Pausa / continuar | Start de un jugador humano |
| Volver al menu desde la pausa | B |

Si se desconecta un mando de un jugador humano, el combate se pausa.
Reconecta los mandos indicados y pulsa Start para continuar. Otro jugador
conectado puede usar B para salir desde la pausa.

## Opciones antes del combate

P1 configura la partida desde la seleccion de personajes:

- X/Y: cambiar color del personaje que estas eligiendo.
- Cruceta arriba/abajo: cambiar escenario.
- RB: cambiar vidas; LB: cambiar nivel de CPU.
- RT: cambiar cantidad de jugadores, de dos a cuatro.
- Start: cambiar frecuencia de objetos.
- Stick derecho arriba/abajo: cambiar entre vidas y limite de tiempo.
- B: deshacer la confirmacion o volver al menu.

Las opciones disponibles tambien aparecen en pantalla. Un mando adicional
puede pulsar un boton durante la seleccion para incorporarse. Durante el
combate puede tomar un puesto CPU existente. No se crean nuevos puestos a
mitad de partida.

## Audio en Xenia

Xenia debe iniciarse sin `--mute=true`.
Para jugar, `./tools/run_xenia.ps1` selecciona la salida XAudio2 de Windows
y habilita el audio. Usa `-Mute` solo si quieres silenciar esa instancia.

Para una prueba automatica con musica y efectos habilitados:

```powershell
./tools/soak_xenia.ps1 -Only 'p1-Kirby-*' -Audio -Seconds 150
```

Las pruebas automaticas silencian Xenia cuando no se indica `-Audio`.
La configuracion guardada de Xenia y el parametro de la instancia pueden
tener valores distintos. El XEX de pruebas usa entrada programada; para
jugar con el mando, reconstruye con `./tools/build_xex.ps1` sin opciones.
