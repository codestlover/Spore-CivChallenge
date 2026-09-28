
#include <Windows.h>
#include "settings/Settings.hpp"
#include <cwchar>
#include <cstring>

namespace CivSettings {
bool rules[RuleCount] = {true, true, true, true, true, true};

namespace {
wchar_t directory[512]{};
wchar_t file[512]{};
const char* keys[RuleCount] = {"BlockLandRaids",    "LimitTurrets",     "LimitSpice",
                               "BlockSuperweapons", "BlockCompliments", "LimitGifts"};

bool Paths() {
    if (*file)
        return true;
    DWORD n = GetEnvironmentVariableW(L"APPDATA", directory, 450);
    if (!n || n >= 450)
        return false;
    wcscat_s(directory, L"\\CivChallenge");
    wcscpy_s(file, directory);
    wcscat_s(file, L"\\settings.ini");
    return true;
}

bool Save() {
    if (!Paths() || (!CreateDirectoryW(directory, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS))
        return false;
    char data[256] = "[Rules]\r\n";
    for (unsigned i = 0; i < RuleCount; ++i) {
        strcat_s(data, keys[i]);
        strcat_s(data, rules[i] ? "=1\r\n" : "=0\r\n");
    }
    DWORD size = DWORD(strlen(data));
    wchar_t temporary[MAX_PATH]{};
    if (!GetTempFileNameW(directory, L"civ", 0, temporary))
        return false;
    HANDLE handle = CreateFileW(temporary, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    bool saved = false;
    if (handle != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        saved = WriteFile(handle, data, size, &written, nullptr) && written == size;
        if (saved)
            saved = FlushFileBuffers(handle) != FALSE;
        CloseHandle(handle);
        if (saved)
            saved = MoveFileExW(temporary, file, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    }
    if (!saved)
        DeleteFileW(temporary);
    return saved;
}
}

void Load() {
    for (bool& value : rules)
        value = true;
    if (!Paths())
        return;
    for (unsigned i = 0; i < RuleCount; ++i) {
        wchar_t key[32]{};
        for (unsigned c = 0; keys[i][c] && c < 31; ++c)
            key[c] = wchar_t(keys[i][c]);
        wchar_t value[16]{};
        GetPrivateProfileStringW(L"Rules", key, L"1", value, 16, file);
        if (value[0] == L'0' && !value[1])
            rules[i] = false;
    }
}

bool Set(unsigned rule, bool value) {
    if (rule >= RuleCount)
        return false;
    rules[rule] = value;
    return Save();
}
}

