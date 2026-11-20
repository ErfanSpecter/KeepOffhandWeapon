// KeepOffhandWeapon - RDR2 (single player only) - Script Hook RDR2 .asi
// Goal: if a gun is equipped in the OFF HAND and the weapon wheel is opened,
// the gun must not drop back into its holster. It is put back in the off hand.
//
// Build: Visual Studio 2022, x64, Release, "Dynamic Library (DLL)".
// Add the Script Hook RDR2 SDK (main.h, natives.h/nativeCaller.h, ScriptHookRDR2.lib)
// from http://www.dev-c.com/rdr2/scripthookrdr2/ ; output name -> KeepOffhandWeapon.asi
// Copy the .asi next to RDR2.exe (needs ScriptHookRDR2.dll already installed).
//
// !!! UNVERIFIED VALUES - not tested in-game (no way to run RDR2 where this was written):
//   * ATTACH_OFFHAND  (which attach point index means "left/off hand")
//   * WHEEL_KEY       (keyboard key that opens the weapon wheel, default Tab; keyboard only)
// Press F9 in game with a gun in the off hand: the script writes a probe to
// KeepOffhandWeapon.log showing which attach point returns your off-hand gun.
// Fix the two constants below from that log if needed.

#include <windows.h>
#include <fstream>
#include <cstdio>
#include "main.h"          // Script Hook RDR2 SDK: scriptRegister, WAIT
#include "nativeCaller.h"  // invoke<T>(hash, ...)

typedef unsigned int Hash;
typedef int Ped;

// ---- natives (hashes confirmed by the public native DB, signatures from memory) ----
static const unsigned long long N_PLAYER_PED_ID       = 0x096275889B8E0EE0ULL;
static const unsigned long long N_GET_CURRENT_WEAPON  = 0x3A87E44BB9A01D54ULL; // (ped, Hash*, p2, attachPoint, p4)
static const unsigned long long N_SET_CURRENT_WEAPON  = 0xADF692B254977C0CULL; // (ped, weapon, equipNow, attachPoint, p4, p5)

// ---- values to verify with the F9 probe ----
static const int  ATTACH_OFFHAND = 0;          // 0 = main hand (the gun you are holding). VERIFY with F9 log
static const int  WHEEL_KEY      = VK_TAB;     // weapon wheel key on keyboard (hold). Change if you rebound it.
static const Hash WEAPON_UNARMED = 0xA2719263;

static void Log(const char* msg)
{
    std::ofstream f("KeepOffhandWeapon.log", std::ios::app);
    f << msg << "\n";
}

static Hash GetHeld(Ped ped, int attach)
{
    Hash w = 0;
    invoke<int>(N_GET_CURRENT_WEAPON, ped, &w, 0, attach, 0);
    return w;
}

static bool WheelHeld()
{
    return (GetAsyncKeyState(WHEEL_KEY) & 0x8000) != 0;
}

void ScriptMain()
{
    Hash lastOff   = 0;    // off-hand weapon seen before the wheel opened
    bool wheelWas  = false;
    bool restore   = false;
    DWORD wheelEnd = 0;

    while (true)
    {
        Ped ped = invoke<Ped>(N_PLAYER_PED_ID);

        // Probe: F9 logs what every attach point currently returns.
        if (GetAsyncKeyState(VK_F9) & 1)
        {
            for (int a = 0; a < 4; a++)
            {
                char b[96];
                sprintf_s(b, "attachPoint %d -> weapon 0x%08X", a, GetHeld(ped, a));
                Log(b);
            }
        }

        Hash off = GetHeld(ped, ATTACH_OFFHAND);
        bool wheel = WheelHeld();

        if (!wheel && !wheelWas)
        {
            // normal play: remember what is in the off hand
            if (off != 0 && off != WEAPON_UNARMED) lastOff = off;
            else if (!restore) lastOff = 0; // player really put it away
        }

        if (wheel && !wheelWas && lastOff != 0)
            restore = true;                 // wheel just opened with an off-hand gun

        if (!wheel && wheelWas)
            wheelEnd = GetTickCount();      // wheel just closed

        if (restore && !wheel && GetTickCount() - wheelEnd > 150)
        {
            // Put the gun back in the off hand only if the wheel emptied it
            // and the player did not pick something else on purpose.
            if (off == 0 || off == WEAPON_UNARMED)
                invoke<int>(N_SET_CURRENT_WEAPON, ped, lastOff, 1, ATTACH_OFFHAND, 0, 0);
            restore = false;
        }

        wheelWas = wheel;
        WAIT(0);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) scriptRegister(hModule, ScriptMain);
    else if (reason == DLL_PROCESS_DETACH) scriptUnregister(hModule);
    return TRUE;
}
