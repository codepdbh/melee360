# Probar una partida local

Esta guía corresponde a `v0.1.0-alpha.2`. Utiliza la selección original
con mano y fichas. [Detalles de los menús](ORIGINAL_MENUS_AUDIO.md).

El XEX tiene un recorrido de intro, pantalla de titulo, menu y partida VS.
Es una version experimental: las pruebas automatizadas corren en Xenia;
el funcionamiento visual, de audio y con mandos en Xbox 360 aun necesita
verificacion. Classic y Adventure siguen siendo secuencias provisionales.

Classic ya elige parejas originales de escenario y rival entre los escenarios
portados. Sus combates normales tienen cinco minutos y vidas limitadas. Si
se acaba el tiempo, pierdes una vida y A permite repetir el mismo encuentro
mientras te queden vidas. Si las agotas, A ofrece continuar con tres vidas
en el mismo encuentro. La secuencia de siete combates sigue siendo una
vista previa: incluye aliados, gigante y metal; faltan bonus y jefes.

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
Ahora incluye el codigo original de inclinacion y ajuste de piernas al suelo, con los datos de cada personaje.
Tambien usa la inicializacion y copia originales de los datos de colision; el contacto completo con paredes sigue pendiente.
Se corrigio la articulacion que mueve las colisiones del escenario: Destino Final ya no arrastra el suelo con una pieza decorativa. Las plataformas transportan a los personajes una sola vez por cuadro.
El render reutiliza los vertices originales ya decodificados y sigue aplicando las animaciones y materiales en cada cuadro.
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
y nivel y comportamiento de IA de cada encuentro. La seleccion original de Aventura
permite elegir Very Easy, Easy, Normal, Hard y Very Hard. Giga Bowser
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
4. Mueve la mano con el stick y coloca la ficha con A; B la retira.
5. Activa un rival CPU desde su panel HMN/CPU o conecta otro mando.
   Pulsa Start para abrir Stage Select y A para elegir escenario.
6. Espera la cuenta inicial. Al terminar, A vuelve a personajes conservando
   la configuracion; B vuelve al menu principal.

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

La mano y las fichas utilizan el menu original. X/Y cambia color; los
paneles HMN/CPU permiten configurar rivales y niveles. Rules abre reglas,
More Rules, Item Switch y Random Stage Switch. Damage Ratio y el handicap
manual se aplican al combate; las reglas avanzadas restantes siguen parciales.
No se admiten Coin/Bonus ni edicion de nombres todavia.

Los mandos adicionales se incorporan durante la seleccion; los puestos CPU
no se convierten en humanos durante el combate. En Stage Select, Start elige
un escenario al azar y B vuelve a personajes conservando la configuracion.

## Audio en Xenia

Xenia debe iniciarse sin `--mute=true`.
Para jugar, `./tools/run_xenia.ps1` selecciona la salida XAudio2 de Windows
y habilita el audio. Usa `-Mute` solo si quieres silenciar esa instancia.

Para una prueba automatica con musica y efectos habilitados:

```powershell
./tools/soak_xenia.ps1 -Build -Only 'zelda-sheik-transform' -Audio
```

Las pruebas automaticas silencian Xenia cuando no se indica `-Audio`.
La configuracion guardada de Xenia y el parametro de la instancia pueden
tener valores distintos. El XEX de pruebas usa entrada programada; para
jugar con el mando, reconstruye con `./tools/build_xex.ps1` sin opciones.
