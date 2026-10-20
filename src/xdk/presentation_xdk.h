#ifndef MELEE360_PRESENTATION_XDK_H
#define MELEE360_PRESENTATION_XDK_H

/* The authored camera aspect includes Melee's pixel correction. Multiply
 * it by the output-width change rather than replacing it with 16/9. */
#define M360_WORLD_ASPECT_SCALE (4.0f / 3.0f)

static float M360_PresentationX(float x, int world)
{
    return world ? x * 2.0f : 160.0f + x * 1.5f;
}

static float M360_PresentationWidth(float width, int world)
{
    return width * (world ? 2.0f : 1.5f);
}

#endif
