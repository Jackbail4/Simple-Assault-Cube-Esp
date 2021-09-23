#include <Windows.h>
#include <iostream>
#include <vector>
#include <Psapi.h>
#include <string>

struct Vec2 { float x, y;};
struct Vec3 {float x, y, z;};
struct Vec4 {float x, y, z, w;};

class Ent
{
public:
    char pad_0000[4]; //0x0000
    Vec3 HeadPos; //0x0004
    char pad_0010[36]; //0x0010
    Vec3 PlayerPos; //0x0034
    char pad_0040[184]; //0x0040
    int32_t Health; //0x00F8
}; //Size: 0x018C

bool WorldToScreen(Vec3 pos, Vec2& screen, float matrix[16], Vec2 WindowSize){
    Vec4 clipCoords;
    clipCoords.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
    clipCoords.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
    clipCoords.z = pos.x * matrix[2] + pos.y * matrix[6] + pos.z * matrix[10] + matrix[14];
    clipCoords.w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

    if (clipCoords.w < 0.1f) {
        return false;
    }

    Vec3 NDC;
    NDC.x = clipCoords.x / clipCoords.w;
    NDC.y = clipCoords.y / clipCoords.w;
    NDC.z = clipCoords.z / clipCoords.w;

    screen.x = (WindowSize.x / 2 * NDC.x) + (NDC.x + WindowSize.x / 2);
    screen.y = -(WindowSize.y / 2 * NDC.y) + (NDC.y + WindowSize.y / 2);
    return true;
}

void DrawLine(Vec2 Start, Vec2 End, HDC hdc){
    HPEN hOPen;
    HPEN hNPen = CreatePen(PS_SOLID, 2, 0x000000FF);
    hOPen = (HPEN)SelectObject(hdc, hNPen);
    MoveToEx(hdc, Start.x, Start.y, NULL);
    LineTo(hdc, End.x, End.y);
    DeleteObject(SelectObject(hdc, hOPen));
}

void DrawBox(Vec2 Head, Vec2 Pos, HDC hdc) {
    float Height = Pos.y - Head.y;
    Height /= 4;
    DrawLine(Vec2{ Head.x + Height, Head.y }, Vec2{ Head.x - Height, Head.y }, hdc);
    DrawLine(Vec2{ Pos.x + Height, Pos.y }, Vec2{ Head.x + Height, Head.y }, hdc);
    DrawLine(Vec2{ Pos.x - Height, Pos.y }, Vec2{ Head.x - Height, Head.y }, hdc);
    DrawLine(Vec2{ Pos.x + Height, Pos.y }, Vec2{ Pos.x - Height, Pos.y }, hdc);
}

void _stdcall MainThread(HMODULE hModule) {
#ifdef _DEBUG
    AllocConsole();
    SetConsoleTitle("Debug Window");
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
#endif

    HWND GameHwnd = FindWindow(0,"AssaultCube");
    HDC hdc = GetDC(GameHwnd);

    Vec2 WindowSize = { 1280,720 };

    float matrix[16];
    uintptr_t BaseAddr = (uintptr_t)GetModuleHandle("ac_client.exe");
    uintptr_t EntList = *(uintptr_t*)(BaseAddr + 0x10F4F8);


    while (!GetAsyncKeyState(VK_F11)) {
        memcpy(&matrix, (PBYTE*)(0x501AE8), sizeof(matrix));
        for (int i = 1; i != 12; i++) {
            Ent* Entity = *(Ent**)(EntList + (i * 0x4));
            if (!Entity) { continue; }
            if (Entity->Health > 0 && Entity->Health <= 100) {
                Vec2 Pos2D, Head2D;
                WorldToScreen(Entity->PlayerPos, Pos2D, matrix ,WindowSize);
                WorldToScreen(Entity->HeadPos, Head2D, matrix, WindowSize);

                DrawBox(Pos2D, Head2D, hdc);
                DrawLine(Vec2{ WindowSize.x / 2, WindowSize.y }, Pos2D, hdc);
            }
        }
    }

#ifdef _DEBUG
    fclose(f);
#endif 
    FreeLibraryAndExitThread(hModule,0);
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MainThread, hModule, NULL, NULL);
    }
}