#include <xtl.h>

namespace {

const unsigned kMaxRects = 4096;

struct RectBatch {
    D3DRECT rects[kMaxRects];
    unsigned count;
    D3DCOLOR color;
};

RectBatch g_green = { {}, 0, D3DCOLOR_XRGB(107, 232, 52) };
RectBatch g_cyan = { {}, 0, D3DCOLOR_XRGB(79, 203, 247) };
RectBatch g_white = { {}, 0, D3DCOLOR_XRGB(238, 244, 252) };
RectBatch g_muted = { {}, 0, D3DCOLOR_XRGB(139, 158, 181) };
RectBatch g_panel = { {}, 0, D3DCOLOR_XRGB(24, 39, 61) };

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

void BuildStatusScreen()
{
    AddRect(g_green, 0, 100, 1280, 5);
    AddOutline(g_panel, 72, 160, 1136, 430, 3);
    AddRect(g_cyan, 72, 160, 8, 430);
    AddRect(g_panel, 112, 240, 1056, 2);
    AddRect(g_panel, 112, 410, 1008, 28);

    AddText(g_green, 72, 34, "MELEE360", 7);
    AddText(g_white, 480, 50, "NATIVE XEX PORT", 3);
    AddText(g_cyan, 112, 190, "POWERPC / D3D9 / XENIA", 4);
    AddText(g_muted, 112, 275, "CPU", 3);
    AddText(g_muted, 390, 275, "MEMORY", 3);
    AddText(g_muted, 750, 275, "EXECUTABLE", 3);
    AddText(g_green, 112, 315, "OK", 5);
    AddText(g_green, 390, 315, "OK", 5);
    AddText(g_green, 750, 315, "DEFAULT.XEX", 5);
    AddText(g_white, 112, 480, "FIRST NATIVE XEX MILESTONE RUNNING", 3);
    AddText(g_muted, 112, 535, "PRESS Y TO EXIT", 2);
    AddText(g_muted, 72, 650, "MELEE360 CLEAN-ROOM PORT", 2);
}

void RenderBatch(IDirect3DDevice9* device, const RectBatch& batch)
{
    if (batch.count)
        device->Clear(batch.count, batch.rects, D3DCLEAR_TARGET, batch.color,
                      1.0f, 0);
}

} // namespace

void __cdecl main()
{
    OutputDebugStringA("[M360][XEX] starting native XDK milestone\n");

    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        OutputDebugStringA("[M360][XEX] Direct3DCreate9 failed\n");
        return;
    }

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
        OutputDebugStringA("[M360][XEX] D3D device creation failed\n");
        d3d->Release();
        return;
    }

    BuildStatusScreen();
    OutputDebugStringA("[M360][XEX] D3D status screen ready\n");

    for (;;) {
        XINPUT_STATE input;
        ZeroMemory(&input, sizeof(input));
        if (XInputGetState(0, &input) == ERROR_SUCCESS &&
            (input.Gamepad.wButtons & XINPUT_GAMEPAD_Y))
            break;

        device->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(7, 12, 24),
                      1.0f, 0);
        RenderBatch(device, g_panel);
        RenderBatch(device, g_cyan);
        RenderBatch(device, g_white);
        RenderBatch(device, g_muted);
        RenderBatch(device, g_green);

        D3DRECT progress = { 112, 410, 112 + 300 +
            static_cast<LONG>((GetTickCount() / 8) % 500), 438 };
        device->Clear(1, &progress, D3DCLEAR_TARGET, g_green.color, 1.0f, 0);
        device->Present(0, 0, 0, 0);
    }

    device->Release();
    d3d->Release();
}
