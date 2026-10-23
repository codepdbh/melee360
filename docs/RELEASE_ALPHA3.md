# MELEE360 v0.1.0-alpha.3

Prerelease experimental del port XEX para Xbox 360. Classic y Adventure
siguen incompletos; la consola fisica y las campanas completas no estan
verificadas.

## Instalar

Descarga `MELEE360-v0.1.0-alpha.3-xbox360.zip`, extraelo y coloca tu propia
ISO Melee NTSC-U 1.02 (GALE01) como `melee.iso`, junto a `default.xex`.
Copia la carpeta `MELEE360` a una Xbox 360 que ya ejecute homebrew XEX
(RGH/JTAG) y abre el ejecutable desde un cargador compatible. En Windows
tambien puedes abrirlo con Xenia. El paquete incluye `INSTRUCCIONES.md`,
registro de compilacion y checksums. No incluye ISO, datos extraidos del
disco, SDK ni claves. Los archivos **Source code** no contienen el XEX.

## Cambios desde alpha.2

- Combate en 1280x720 con vista horizontal ampliada y proporciones conservadas.
  HUD, menus y videos mantienen su composicion original.
- Apariciones aleatorias de objetos habilitadas; seleccion de puntos y
  ponderacion originales. Off sigue desactivando las apariciones.
- Corneria carga los puntos originales de jugadores, objetos, camara y limites.
- Las piezas ocultas no mantienen colision activa. Las consultas respetan
  filtros de grupo; se restauran el orden original de busqueda de suelo y
  las proyecciones originales para seguir pendientes y segmentos unidos.
- Parada en bordes y equilibrio Ottotto con las funciones originales,
  desplazamiento de la caja de colision, comprobaciones de pared y limites
  de la palanca.
- Consulta original de modo de un jugador, con su tratamiento del C-stick
  en Classic y Adventure.

## Limites

El calculo completo de colisiones por cuadro, peligros y transformaciones
de escenarios permanece incompleto. Classic enlaza siete encuentros de
combate y todavia carece de bonus y jefes. Adventure contiene veinte fases
parciales, con recorridos y sistemas pendientes. Faltan pantallas, resultados
y transiciones originales. Las pruebas acotadas de Xenia no equivalen a
partidas completas sin caidas ni a validacion en hardware Xbox 360.

Publicada el 23 de octubre de 2026:
[release y descarga](https://github.com/codepdbh/melee360/releases/tag/v0.1.0-alpha.3).
Los hashes, commit y diez comprobaciones del paquete publicado estan en
[ALPHA3_VALIDATION.md](ALPHA3_VALIDATION.md).
