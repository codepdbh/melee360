#include <xtl.h>

extern "C" unsigned int lbTime_8000AEC8(unsigned int a, unsigned int b);
extern "C" unsigned int lbTime_8000AEE4(unsigned int a, int b);
extern "C" unsigned int lbTime_8000AF74(unsigned int a, int b);

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
    WORD previousButtons;
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
    AddText(g_white, 430, 38, "PLAYABLE XEX PROTOTYPE", 3);
    AddText(g_cyan, 96, 151, "L STICK MOVE", 3);
    AddText(g_cyan, 406, 151, "A JUMP", 3);
    AddText(g_cyan, 625, 151, "X ATTACK", 3);
    AddText(g_cyan, 901, 151, "START RESET", 3);
    AddText(g_muted, 96, 202,
            meleeCodePassed ? "MELEE LBTIME LINKED: OK" : "MELEE LBTIME LINKED: FAIL",
            2);
    AddText(g_muted, 918, 202, "Y EXIT", 2);
    AddText(g_muted, 610, 220,
            "KEYBOARD A D MOVE / SEMICOLON JUMP / L ATTACK / X RESET / P EXIT",
            1);
    AddText(g_white, 952, 290, "DAMAGE", 2);
    AddText(g_muted, 76, 672, "NATIVE POWERPC / D3D9 / ORIGINAL LBTIME.C", 2);
}

void RenderBatch(IDirect3DDevice9* device, const RectBatch& batch)
{
    if (batch.count)
        device->Clear(batch.count, batch.rects, D3DCLEAR_TARGET, batch.color,
                      1.0f, 0);
}

void DrawRect(IDirect3DDevice9* device, LONG x, LONG y, LONG width,
              LONG height, D3DCOLOR color)
{
    D3DRECT rect = { x, y, x + width, y + height };
    device->Clear(1, &rect, D3DCLEAR_TARGET, color, 1.0f, 0);
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

void UpdateGame(GameState& game, const XINPUT_STATE& input, float elapsed,
                DWORD now)
{
    const WORD buttons = input.Gamepad.wButtons;
    const WORD pressed = static_cast<WORD>(buttons & ~game.previousButtons);
    game.previousButtons = buttons;

    if (pressed & XINPUT_GAMEPAD_START)
        ResetGame(game);

    float direction = 0.0f;
    if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
        direction = -1.0f;
    else if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
        direction = 1.0f;
    else if (input.Gamepad.sThumbLX < -7000)
        direction = static_cast<float>(input.Gamepad.sThumbLX) / 32768.0f;
    else if (input.Gamepad.sThumbLX > 7000)
        direction = static_cast<float>(input.Gamepad.sThumbLX) / 32767.0f;

    game.velocityX = direction * 0.36f;
    if ((pressed & XINPUT_GAMEPAD_A) && game.grounded) {
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

    const float playerCenter = game.playerX + kPlayerWidth * 0.5f;
    if ((pressed & XINPUT_GAMEPAD_X) && playerCenter > 790.0f &&
        playerCenter < 980.0f && game.playerY > 430.0f) {
        game.damage = lbTime_8000AF74(game.damage, 8);
        game.attackUntil = now + 130;
    }
}

void RenderGame(IDirect3DDevice9* device, const GameState& game, DWORD now)
{
    const LONG x = static_cast<LONG>(game.playerX);
    const LONG y = static_cast<LONG>(game.playerY);
    DrawRect(device, x + 8, y, 22, 18, D3DCOLOR_XRGB(238, 244, 252));
    DrawRect(device, x, y + 18, 38, 30, D3DCOLOR_XRGB(79, 203, 247));
    DrawRect(device, x + 4, y + 48, 10, 8, D3DCOLOR_XRGB(107, 232, 52));
    DrawRect(device, x + 24, y + 48, 10, 8, D3DCOLOR_XRGB(107, 232, 52));

    const BYTE red = static_cast<BYTE>(80 + (game.damage * 175) / 255);
    DrawRect(device, 850, 510, 46, 64, D3DCOLOR_XRGB(red, 83, 97));
    DrawRect(device, 858, 493, 30, 20, D3DCOLOR_XRGB(238, 184, 96));

    if (now < game.attackUntil)
        DrawRect(device, x + 38, y + 22, 65, 18,
                 D3DCOLOR_XRGB(255, 224, 94));

    g_dynamic.count = 0;
    AddNumber(g_dynamic, 1004, 326, game.damage, 5);
    RenderBatch(device, g_dynamic);
}

} // namespace

void __cdecl main()
{
    OutputDebugStringA("[M360][XEX] starting playable native prototype\n");

    const bool meleeCodePassed =
        lbTime_8000AEC8(0xfffffff0u, 0x20u) == 0xffffffffu &&
        lbTime_8000AEE4(4u, -10) == 0u &&
        lbTime_8000AF74(250u, 8) == 255u;

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

    BuildScene(meleeCodePassed);
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

        XINPUT_STATE input;
        ZeroMemory(&input, sizeof(input));
        XInputGetState(0, &input);
        if (input.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
            break;

        UpdateGame(game, input, static_cast<float>(tickDelta), now);

        device->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(7, 12, 24),
                      1.0f, 0);
        RenderBatch(device, g_panel);
        RenderBatch(device, g_cyan);
        RenderBatch(device, g_white);
        RenderBatch(device, g_muted);
        RenderBatch(device, g_green);
        RenderGame(device, game, now);
        device->Present(0, 0, 0, 0);
    }

    device->Release();
    d3d->Release();
}
