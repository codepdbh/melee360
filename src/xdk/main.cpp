#include <xtl.h>

#include "melee_pad_xdk.h"
#include "sprite_renderer.h"

extern "C" unsigned int lbTime_8000AEC8(unsigned int a, unsigned int b);
extern "C" unsigned int lbTime_8000AEE4(unsigned int a, int b);
extern "C" unsigned int lbTime_8000AF74(unsigned int a, int b);
extern "C" int powi(int base, int exponent);
extern "C" int lb_8000D148(float point0X, float point0Y,
                             float point1X, float point1Y,
                             float point2X, float point2Y,
                             float threshold);

namespace {

const unsigned kMaxRects = 4096;
const float kFloorY = 574.0f;
const float kPlayerWidth = 38.0f;
const float kPlayerHeight = 56.0f;

struct RectBatch {
    D3DRECT rects[kMaxRects];
    unsigned count;
    D3DCOLOR color;
};

struct GameState {
    float playerX;
    float playerY;
    float velocityX;
    float velocityY;
    bool grounded;
    unsigned damage;
    DWORD attackUntil;
};

RectBatch g_green = { {}, 0, D3DCOLOR_XRGB(107, 232, 52) };
RectBatch g_cyan = { {}, 0, D3DCOLOR_XRGB(79, 203, 247) };
RectBatch g_white = { {}, 0, D3DCOLOR_XRGB(238, 244, 252) };
RectBatch g_muted = { {}, 0, D3DCOLOR_XRGB(139, 158, 181) };
RectBatch g_panel = { {}, 0, D3DCOLOR_XRGB(24, 39, 61) };
RectBatch g_dynamic = { {}, 0, D3DCOLOR_XRGB(238, 244, 252) };

void AddRect(RectBatch& batch, LONG x, LONG y, LONG width, LONG height)
{
    if (batch.count >= kMaxRects || width <= 0 || height <= 0)
        return;
    D3DRECT& rect = batch.rects[batch.count++];
    rect.x1 = x;
    rect.y1 = y;
    rect.x2 = x + width;
    rect.y2 = y + height;
}

void AddOutline(RectBatch& batch, LONG x, LONG y, LONG width, LONG height,
                LONG thickness)
{
    AddRect(batch, x, y, width, thickness);
    AddRect(batch, x, y + height - thickness, width, thickness);
    AddRect(batch, x, y, thickness, height);
    AddRect(batch, x + width - thickness, y, thickness, height);
}

BYTE GlyphRow(char character, unsigned row)
{
    static const BYTE letters[26][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
        {31,4,4,4,4,4,31},      {7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},   {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},     {31,1,2,4,8,16,31}
    };
    static const BYTE digits[10][7] = {
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31},     {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2},     {31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14}
    };

    if (row >= 7)
        return 0;
    if (character >= 'a' && character <= 'z')
        character = static_cast<char>(character - 'a' + 'A');
    if (character >= 'A' && character <= 'Z')
        return letters[character - 'A'][row];
    if (character >= '0' && character <= '9')
        return digits[character - '0'][row];
    if (character == '-')
        return row == 3 ? 31 : 0;
    if (character == ':')
        return (row == 2 || row == 5) ? 4 : 0;
    if (character == '.')
        return row == 6 ? 4 : 0;
    if (character == '/')
        return static_cast<BYTE>(1u << (row < 5 ? row : 4));
    return 0;
}

void AddText(RectBatch& batch, LONG x, LONG y, const char* text, LONG scale)
{
    const LONG originX = x;
    while (*text) {
        const char character = *text++;
        if (character == '\n') {
            x = originX;
            y += 9 * scale;
            continue;
        }
        for (unsigned row = 0; row < 7; ++row) {
            const BYTE bits = GlyphRow(character, row);
            for (unsigned column = 0; column < 5; ++column) {
                if (bits & (1u << (4u - column)))
                    AddRect(batch, x + static_cast<LONG>(column) * scale,
                            y + static_cast<LONG>(row) * scale, scale, scale);
            }
        }
        x += 6 * scale;
    }
}

void AddNumber(RectBatch& batch, LONG x, LONG y, unsigned value, LONG scale)
{
    char text[4];
    text[3] = '\0';
    text[2] = static_cast<char>('0' + value % 10);
    text[1] = value >= 10 ? static_cast<char>('0' + (value / 10) % 10) : ' ';
    text[0] = value >= 100 ? static_cast<char>('0' + (value / 100) % 10) : ' ';
    AddText(batch, x, y, text, scale);
}

void BuildScene(bool meleeCodePassed)
{
    AddRect(g_green, 0, 86, 1280, 4);
    AddOutline(g_panel, 62, 125, 1156, 500, 3);
    AddRect(g_cyan, 62, 125, 7, 500);
    AddRect(g_panel, 95, 236, 1090, 2);
    AddRect(g_panel, 95, 574, 1090, 24);
    AddRect(g_panel, 225, 460, 230, 12);
    AddRect(g_panel, 715, 400, 230, 12);

    AddText(g_green, 62, 25, "MELEE360", 6);
    AddText(g_white, 430, 38, "TEXTURED XEX PROTOTYPE", 3);
    AddText(g_cyan, 96, 151, "L STICK MOVE", 3);
    AddText(g_cyan, 406, 151, "A JUMP", 3);
    AddText(g_cyan, 625, 151, "X ATTACK", 3);
    AddText(g_cyan, 901, 151, "START RESET", 3);
    AddText(g_muted, 96, 202,
            meleeCodePassed ? "MELEE CORE MODULES: OK" : "MELEE CORE MODULES: FAIL",
            2);
    AddText(g_muted, 500, 202, "HSD CONTROLLER LINKED", 2);
    AddText(g_muted, 1050, 202, "Y EXIT", 2);
    AddText(g_muted, 610, 220,
            "KEYBOARD A D MOVE / SEMICOLON JUMP / L ATTACK / X RESET / P EXIT",
            1);
    AddText(g_white, 952, 290, "DAMAGE", 2);
    AddText(g_muted, 76, 672, "NATIVE POWERPC / D3D9 / ORIGINAL LBTIME.C", 2);
}

SpriteColor ToSpriteColor(D3DCOLOR color, float alpha = 1.0f)
{
    SpriteColor result = {
        static_cast<float>((color >> 16) & 255) / 255.0f,
        static_cast<float>((color >> 8) & 255) / 255.0f,
        static_cast<float>(color & 255) / 255.0f,
        alpha
    };
    return result;
}

void RenderBatch(SpriteRenderer& renderer, const RectBatch& batch)
{
    const SpriteColor color = ToSpriteColor(batch.color);
    for (unsigned i = 0; i < batch.count; ++i) {
        const D3DRECT& rect = batch.rects[i];
        renderer.AddQuad(static_cast<float>(rect.x1),
                         static_cast<float>(rect.y1),
                         static_cast<float>(rect.x2 - rect.x1),
                         static_cast<float>(rect.y2 - rect.y1), color);
    }
}

void DrawRect(SpriteRenderer& renderer, LONG x, LONG y, LONG width,
              LONG height, D3DCOLOR color, float alpha = 1.0f)
{
    renderer.AddQuad(static_cast<float>(x), static_cast<float>(y),
                     static_cast<float>(width), static_cast<float>(height),
                     ToSpriteColor(color, alpha));
}

void RenderBackdrop(SpriteRenderer& renderer, DWORD now)
{
    const SpriteColor skyTop = { 0.035f, 0.020f, 0.120f, 1.0f };
    const SpriteColor skyBottom = { 0.010f, 0.100f, 0.160f, 1.0f };
    const SpriteColor horizonTop = { 0.10f, 0.20f, 0.30f, 0.45f };
    const SpriteColor horizonBottom = { 0.01f, 0.04f, 0.09f, 0.0f };
    renderer.AddGradientQuad(0.0f, 0.0f, 1280.0f, 720.0f,
                             skyTop, skyBottom);
    renderer.AddGradientQuad(0.0f, 300.0f, 1280.0f, 320.0f,
                             horizonTop, horizonBottom);

    const D3DCOLOR starColor = D3DCOLOR_XRGB(153, 220, 255);
    for (unsigned i = 0; i < 34; ++i) {
        const LONG x = static_cast<LONG>((i * 193u + 71u) % 1240u) + 20;
        const LONG y = static_cast<LONG>((i * 83u + 37u) % 310u) + 95;
        const LONG size = ((i + now / 350) % 5 == 0) ? 3 : 2;
        DrawRect(renderer, x, y, size, size, starColor, 0.72f);
    }

    const D3DCOLOR moon = D3DCOLOR_XRGB(171, 231, 245);
    DrawRect(renderer, 1045, 285, 74, 12, moon, 0.16f);
    DrawRect(renderer, 1032, 297, 100, 34, moon, 0.16f);
    DrawRect(renderer, 1025, 331, 114, 42, moon, 0.16f);
    DrawRect(renderer, 1032, 373, 100, 34, moon, 0.16f);
    DrawRect(renderer, 1045, 407, 74, 12, moon, 0.16f);

    const D3DCOLOR skyline = D3DCOLOR_XRGB(10, 25, 48);
    for (unsigned building = 0; building < 18; ++building) {
        const LONG x = 70 + static_cast<LONG>(building) * 68;
        const LONG height = 45 + static_cast<LONG>((building * 47) % 105);
        DrawRect(renderer, x, 574 - height, 52, height, skyline, 0.82f);
    }

    const SpriteColor panelTop = { 0.025f, 0.045f, 0.095f, 0.92f };
    const SpriteColor panelBottom = { 0.008f, 0.018f, 0.045f, 0.96f };
    renderer.AddGradientQuad(62.0f, 125.0f, 1156.0f, 500.0f,
                             panelTop, panelBottom);
}

void ResetGame(GameState& game)
{
    game.playerX = 145.0f;
    game.playerY = kFloorY - kPlayerHeight;
    game.velocityX = 0.0f;
    game.velocityY = 0.0f;
    game.grounded = true;
    game.damage = 0;
    game.attackUntil = 0;
}

void UpdateGame(GameState& game, const HSD_PadStatus& input, float elapsed,
                DWORD now)
{
    const M360U32 buttons = input.button;
    const M360U32 pressed = input.trigger;

    if (pressed & HSD_PAD_START)
        ResetGame(game);

    float direction = 0.0f;
    if (buttons & HSD_PAD_DPADLEFT)
        direction = -1.0f;
    else if (buttons & HSD_PAD_DPADRIGHT)
        direction = 1.0f;
    else if (input.nml_stickX < -0.08f || input.nml_stickX > 0.08f)
        direction = input.nml_stickX;

    game.velocityX = direction * 0.36f;
    if ((pressed & HSD_PAD_A) && game.grounded) {
        game.velocityY = -0.72f;
        game.grounded = false;
    }

    game.velocityY += 0.00175f * elapsed;
    game.playerX += game.velocityX * elapsed;
    game.playerY += game.velocityY * elapsed;

    if (game.playerX < 96.0f)
        game.playerX = 96.0f;
    if (game.playerX > 1135.0f)
        game.playerX = 1135.0f;
    if (game.playerY + kPlayerHeight >= kFloorY) {
        game.playerY = kFloorY - kPlayerHeight;
        game.velocityY = 0.0f;
        game.grounded = true;
    }

    const float attackStartX = game.playerX + kPlayerWidth * 0.5f;
    const float attackY = game.playerY + 30.0f;
    const int attackHit = lb_8000D148(
        attackStartX, attackY, attackStartX + 88.0f, attackY,
        873.0f, 540.0f, 38.0f);
    if ((pressed & HSD_PAD_X) && attackHit) {
        game.damage = lbTime_8000AF74(game.damage, 8);
        game.attackUntil = now + 130;
    }
}

void RenderGame(SpriteRenderer& renderer, const GameState& game, DWORD now)
{
    const LONG x = static_cast<LONG>(game.playerX);
    const LONG y = static_cast<LONG>(game.playerY);
    DrawRect(renderer, x - 8, 568, 54, 7, D3DCOLOR_XRGB(0, 0, 0), 0.40f);
    const SpriteColor white = { 1.0f, 1.0f, 1.0f, 1.0f };
    renderer.AddSprite(static_cast<float>(x - 13), static_cast<float>(y - 8),
                       65.0f, 64.0f, 8.0f / 128.0f, 0.0f,
                       56.0f / 128.0f, 1.0f, white);

    const float damageTint = static_cast<float>(game.damage) / 637.5f;
    const SpriteColor targetTint = {
        1.0f, 1.0f - damageTint, 1.0f - damageTint, 1.0f
    };
    DrawRect(renderer, 840, 568, 66, 7, D3DCOLOR_XRGB(0, 0, 0), 0.40f);
    renderer.AddSprite(849.0f, 510.0f, 48.0f, 64.0f,
                       68.0f / 128.0f, 0.0f,
                       116.0f / 128.0f, 1.0f, targetTint);

    if (now < game.attackUntil)
        DrawRect(renderer, x + 38, y + 22, 65, 18,
                 D3DCOLOR_XRGB(255, 224, 94), 0.82f);

    g_dynamic.count = 0;
    AddNumber(g_dynamic, 1004, 326, game.damage, 5);
    RenderBatch(renderer, g_dynamic);
}

} // namespace

void __cdecl main()
{
    OutputDebugStringA("[M360][XEX] starting playable native prototype\n");

    const bool meleeCodePassed =
        lbTime_8000AEC8(0xfffffff0u, 0x20u) == 0xffffffffu &&
        lbTime_8000AEE4(4u, -10) == 0u &&
        lbTime_8000AF74(250u, 8) == 255u &&
        powi(3, 4) == 81 &&
        lb_8000D148(0.0f, 0.0f, 10.0f, 0.0f, 5.0f, 0.0f, 1.0f) == 1;

    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
        return;

    D3DPRESENT_PARAMETERS present;
    ZeroMemory(&present, sizeof(present));
    present.BackBufferWidth = 1280;
    present.BackBufferHeight = 720;
    present.BackBufferFormat = D3DFMT_A8R8G8B8;
    present.FrontBufferFormat = D3DFMT_LE_X8R8G8B8;
    present.BackBufferCount = 1;
    present.MultiSampleType = D3DMULTISAMPLE_NONE;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    IDirect3DDevice9* device = 0;
    HRESULT result = d3d->CreateDevice(0, D3DDEVTYPE_HAL, 0,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &present, &device);
    if (FAILED(result) || !device) {
        d3d->Release();
        return;
    }

    SpriteRenderer renderer;
    if (!renderer.Initialize(device)) {
        OutputDebugStringA("[M360][XEX] sprite renderer initialization failed\n");
        device->Release();
        d3d->Release();
        return;
    }

    BuildScene(meleeCodePassed);
    M360_HSDPadInit();
    GameState game;
    ZeroMemory(&game, sizeof(game));
    ResetGame(game);
    DWORD previousTick = GetTickCount();

    for (;;) {
        const DWORD now = GetTickCount();
        DWORD tickDelta = now - previousTick;
        previousTick = now;
        if (tickDelta > 33)
            tickDelta = 33;

        HSD_PadRenewStatus();
        const HSD_PadStatus& input = HSD_PadGameStatus[0];
        if (input.button & HSD_PAD_Y)
            break;

        UpdateGame(game, input, static_cast<float>(tickDelta), now);

        device->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(3, 7, 18),
                      1.0f, 0);
        renderer.Begin();
        RenderBackdrop(renderer, now);
        RenderBatch(renderer, g_panel);
        RenderBatch(renderer, g_cyan);
        RenderBatch(renderer, g_white);
        RenderBatch(renderer, g_muted);
        RenderBatch(renderer, g_green);
        RenderGame(renderer, game, now);
        renderer.End(device);
        device->Present(0, 0, 0, 0);
    }

    renderer.Shutdown();
    device->Release();
    d3d->Release();
}
