
constexpr unsigned HostImageSize = 26271744;
constexpr unsigned HostTimestamp = 1727737552;
Hook hooks[] = {
    {"finish_city_capture", 0xbe8a20, reinterpret_cast<void**>(&finishCityCapture),
     reinterpret_cast<void*>(&FinishCityCaptureHook),
     "\x83\xec\x74\x53\x55\x56\x8b\xf1\x8b\x8e\x90\x05\x00\x00\x57\xc6", 16},
    {"choose_city_specialty", 0xcf7a30, reinterpret_cast<void**>(&chooseCitySpecialty),
     reinterpret_cast<void*>(&ChooseCitySpecialtyHook),
     "\x56\xe8\xca\x59\xe4\xff\x8b\xc8\xe8\x53\xe5\xe2\xff\x8b\xb0\x4c", 16},
    {"vehicle_order", 0xcac1a0, reinterpret_cast<void**>(&order), reinterpret_cast<void*>(&OrderHook),
     "\x83\xec\x18\x55\x57\x8b\xf9\xe8\xb4\xb1\xff\xff\x8b\x6c\x24\x24", 16},
    {"create_turret", 0xbe2100, reinterpret_cast<void**>(&createTurret), reinterpret_cast<void*>(&CreateTurretHook),
     "\x51\x53\x8b\xd9\xe8\xf7\xb2\xf5\xff\x68\x42\xf3\x36\x04\x8b\xc8", 16},
    {"auto_turret", 0xbe39a0, reinterpret_cast<void**>(&autoTurret), reinterpret_cast<void*>(&AutoTurretHook),
     "\x83\xec\x44\x56\x57\x8b\xf1\xe8\x64\x60\xff\xff\x8b\xf8\x85\xff", 16},
    {"slot_turret", 0xbe3ae0, reinterpret_cast<void**>(&slotTurret), reinterpret_cast<void*>(&SlotTurretHook),
     "\x8b\x44\x24\x04\x83\xec\x40\x56\x57\x8b\xf9\x50\x8d\x8f\xb4\x04", 16},
    {"begin_mine", 0xbfe590, reinterpret_cast<void**>(&beginMine), reinterpret_cast<void*>(&BeginMineHook),
     "\x8b\x44\x24\x04\x53\x8b\x5c\x24\x0c\x56\x8b\xf1\x57\xc7\x86\x10", 16},
    {"capture_mine", 0xbff410, reinterpret_cast<void**>(&captureMine), reinterpret_cast<void*>(&CaptureMineHook),
     "\x83\xec\x30\x53\x55\x56\x8b\xf1\x8b\x06\x8b\x50\x4c\x57\xff\xd2", 16},
    {"mine_damage", 0xbffc10, reinterpret_cast<void**>(&mineDamage), reinterpret_cast<void*>(&MineDamageHook),
     "\x81\xec\xa0\x00\x00\x00\x55\x56\x8b\xb4\x24\xbc\x00\x00\x00\x8b", 16},
    {"mine_update", 0xbff8e0, reinterpret_cast<void**>(&mineUpdate), reinterpret_cast<void*>(&MineUpdateHook),
     "\x56\x8b\xf1\x8b\x8e\xe0\x01\x00\x00\x57\x8b\xbe\xec\x01\x00\x00", 16},
    {"superweapon_available", 0xbf8b40, reinterpret_cast<void**>(&superweaponAvailable),
     reinterpret_cast<void*>(&SuperweaponAvailableHook),
     "\x56\x8b\x74\x24\x08\x57\x8b\xf9\x83\xfe\xff\x75\x07\x5f\x32\xc0", 16},
    {"launch_superweapon", 0xbf8bb0, reinterpret_cast<void**>(&launchSuperweapon),
     reinterpret_cast<void*>(&LaunchSuperweaponHook),
     "\x83\xec\x28\x53\x8b\x5c\x24\x30\x55\x56\x57\x53\x8b\xf1\xe8\x7d", 16},
    {"city_superweapon", 0xbe5d30, reinterpret_cast<void**>(&citySuperweapon),
     reinterpret_cast<void*>(&CitySuperweaponHook), "\x83\xec\x28\x53\x8b\x5c\x24\x30\x57\x8b\xf9\x83\xfb\x0b\x77\x1b",
     16},
    {"city_editor_capacity", 0xd07bf0, reinterpret_cast<void**>(&cityEditorCapacity),
     reinterpret_cast<void*>(&CityEditorCapacityHook),
     "\x8b\x44\x24\x04\x56\x57\x8b\xf1\x83\xf8\x05\x0f\x87\xb4\x00\x00", 16},
    {"select_superweapon", 0xcf1d40, reinterpret_cast<void**>(&selectSuperweapon),
     reinterpret_cast<void*>(&SelectSuperweaponHook),
     "\x56\x57\x8b\x7c\x24\x0c\x8b\xf1\x39\xbe\x40\x01\x00\x00\x0f\x84", 16},
    {"palette_click", 0x5f4a80, reinterpret_cast<void**>(&paletteClick), reinterpret_cast<void*>(&PaletteClickHook),
     "\x83\xec\x08\x56\x8b\xf1\x80\xbe\x84\x01\x00\x00\x00\x75\x5a\x83", 16},
    {"ui_dispatch", 0x959240, reinterpret_cast<void**>(&windowDispatch), reinterpret_cast<void*>(&WindowDispatchHook),
     "\x80\x7c\x24\x08\x00\x53\x8b\x5c\x24\x08\x57\x8b\xf9\x74\x5c\x8b", 16},
    {"civ_comm_action", 0xaeb810, reinterpret_cast<void**>(&commAction), reinterpret_cast<void*>(&CommActionHook),
     "\x53\x55\x56\x57\x8b\xf1\xe8\xf5\x58\x53\x00\x8b\x88\x3c\x01\x00", 16},
    {"comm_response_button", 0xdd2bf0, reinterpret_cast<void**>(&setResponse),
     reinterpret_cast<void*>(&SetResponseHook), "\x83\xec\x08\x56\x8b\xf1\x8b\x4e\x24\x8b\x01\x8b\x80\xd0\x00\x00", 16},
    {"load_saved_game", 0xb28ad0, reinterpret_cast<void**>(&loadSave), reinterpret_cast<void*>(&LoadSaveHook),
     "\xa1\x50\xdf\x67\x01\x56\x57\x50\x8b\xf1\xe8\xe1\xee\xff\xff\xe8", 16},
    {"new_game_session", 0xb32f90, reinterpret_cast<void**>(&newGame), reinterpret_cast<void*>(&NewGameHook),
     "\x56\xe8\xca\xa4\x00\x00\x8b\xc8\xe8\x73\xb0\x04\x00\xe8\x0e\xa4", 16},
};

NativeCheck audioChecks[] = {
    {0x436350, "\x55\x8b\xec\x83\xec\x08\xe8\x15\xa3\x5e\x00\x89\x45\xfc\x83\x7d", 16},
    {0x436390, "\x55\x8b\xec\x51\xe8\xd7\xa2\x5e\x00\x89\x45\xfc\x83\x7d\xfc\x00", 16},
};

NativeCheck captureChecks[] = {
    {0xbe7520, "\x53\x56\x8b\xf1\x8b\x4c\x24\x0c\x57\x3b\x8e\x40\x05\x00\x00\x74", 16},
};

NativeCheck startupChecks[] = {
    {0x67dcd0, "\xa1\xb0\xd8\x5f\x01\xc3\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc", 16},
    {0x8100a0, "\x8b\xc1\x33\xc9\x89\x48\x04\xc7\x00\x60\x83\x41\x01\x89\x48\x08", 16},
    {0x812250, "\x83\xec\x0c\x33\xc0\x56\x89\x44\x24\x04\x89\x44\x24\x08\x89\x44", 16},
    {0x810650, "\x8b\x49\x14\x85\xc9\x74\x05\xe9\x44\xfc\xff\xff\x33\xc0\xc2\x08", 16},
    {0x810610, "\x8b\x41\x14\x85\xc0\x74\x07\x8b\x80\x88\x00\x00\x00\xc3\x33\xc0", 16},
    {0x811bc0, "\x56\x57\x8b\xf9\xe8\x37\x0f\x15\x00\x8b\x77\x14\x85\xf6\x74\x3c", 16},
    {0x805170, "\xe8\xeb\x78\xe7\xff\x85\xc0\x74\x15\x8b\x10\x8b\xc8\x8b\x42\x04", 16},
};

NativeCheck settingsChecks[] = {
    {0x67ca60, "\xa1\x4c\xcc\x5f\x01\xc3\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc", 16},
    {0x9670a0, "\x8b\x44\x24\x08\x85\xc0\x75\x05\xe8\x73\xa1\xfe\xff\x50\x68\x98", 16},
    {0x806320, "\x83\xec\x24\x8b\x44\x24\x28\x53\x56\x8b\x74\x24\x34\xc7\x06\x00", 16},
    {0x8069c0, "\x53\x8b\x5c\x24\x08\x85\xdb\x75\x04\x32\xc0\x5b\xc3\x8b\x03\x8b", 16},
};

NativeCheck localeChecks[] = {
    {0x67de00, "\xa1\xf8\xd8\x5f\x01\xc3\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc", 16},
};

NativeCheck diplomacyChecks[] = {
    {0xb3d5a0, "\xa1\x44\xeb\x67\x01\xc3\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc", 16},
};

