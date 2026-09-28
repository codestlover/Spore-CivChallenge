
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

extern "C" void WINAPI CivLegacyEntry() {
    MessageBoxW(nullptr,
                L"Civ Challenge requires a current Spore ModAPI Launcher Kit with XML Mod Identity 1.0.1.2 support.\n\n"
                L"Update the Launcher Kit, then open CivChallenge.sporemod with Spore ModAPI Easy Installer.\n\n"
                L"https://launcherkit.sporecommunity.com/",
                L"Civ Challenge - update the mod installer", MB_OK | MB_ICONINFORMATION);
    ExitProcess(0);
}

