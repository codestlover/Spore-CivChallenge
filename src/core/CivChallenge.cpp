
#include <Windows.h>
#include <detours.h>
#include <Spore/ModAPI.h>
#include "core/SdkCompat.hpp"
#include "core/HookChain.hpp"
#include "settings/Settings.hpp"
#include "localization/Localization.hpp"
#include <Spore/App/IMessageManager.h>
#include <Spore/App/IGameModeManager.h>
using eastl::intrusive_ptr;
#include <Spore/Simulator/SubSystem/GameNounManager.h>
#include <Spore/Simulator/SubSystem/GameModeManager.h>
#include <Spore/Simulator/cCivilization.h>
#include <Spore/Simulator/cCommodityNode.h>
#include <Spore/Simulator/cTribeHut.h>
#include <Spore/Simulator/SubSystem/cStrategy.h>
#include <Spore/UTFWin/IWindow.h>
#include <cstring>
#include <cstddef>

static_assert(sizeof(Simulator::cCommodityNode) == 0x290, "Commodity ABI");
static_assert(sizeof(Simulator::cCity) == 0x818, "City ABI");
static_assert(sizeof(Simulator::cVehicle) == 0xd98, "Vehicle ABI");
static_assert(offsetof(Simulator::cCity, mTurrets) == 0x354, "City turret vector ABI");
static_assert(offsetof(Simulator::cCity, mpCivilization) == 0x590, "City owner ABI");
static_assert(offsetof(Simulator::cCivilization, mIsPlayerOwned) == 0x89, "Player flag ABI");
static_assert(offsetof(Simulator::cCommodityNode, mConstructingPoliticalID) == 0x214, "Mine construction ABI");

namespace App {
IMessageManager* IMessageManager::Get() {
    return reinterpret_cast<IMessageManager* (*)()>(GetAddress(IMessageManager, Get))();
}
}

namespace Simulator {
cGameNounManager* cGameNounManager::Get() {
    return reinterpret_cast<cGameNounManager* (*)()>(GetAddress(cGameNounManager, Get))();
}

cCivilization* cGameNounManager::GetPlayerCivilization() {
    return reinterpret_cast<cCivilization*(__thiscall*)(cGameNounManager*)>(
        GetAddress(cGameNounManager, GetPlayerCivilization))(this);
}

uint32_t GetGameModeID() {
    auto* m = reinterpret_cast<cGameModeManager* (*)()>(GetAddress(cGameModeManager, Get))();
    return m ? m->mActiveModeID : uint32_t(-1);
}
}

using namespace Simulator;
extern "C" LONG WINAPI DetourTransactionAbort();

namespace {
constexpr unsigned MaxTurrets = 3, MaxSpice = 4, MaxGifts = 3;
bool enabled = false, normalizing = false;
DWORD lastRejection = 0;
bool rejectionPlayed = false;
using CreateAudioTrackFn = int (*)();
using PlayAudioFn = void (*)(uint32_t, int);
CreateAudioTrackFn createAudioTrack;
PlayAudioFn playAudio;
uintptr_t cityEditorLimitsVtable = 0;

void RejectSound() {
    DWORD now = GetTickCount();
    if (rejectionPlayed && now - lastRejection < 180)
        return;
    if (!createAudioTrack || !playAudio)
        return;
    lastRejection = now;
    rejectionPlayed = true;
    playAudio(0x594303b7, createAudioTrack());
}

using AppModesFn = App::IGameModeManager* (*)();
using LayoutCtorFn = void(__thiscall*)(void*);
using LayoutLoadFn = bool(__thiscall*)(void*, const char16_t*, uint32_t, bool, uint32_t);
using LayoutFindFn = UTFWin::IWindow*(__thiscall*)(void*, uint32_t, bool);
using LayoutContainerFn = UTFWin::IWindow*(__thiscall*)(void*);
using LayoutDeleteFn = void(__thiscall*)(void*, int);
using UiTimeFn = float (*)();
AppModesFn getAppModes;
LayoutCtorFn layoutCtor;
LayoutLoadFn layoutLoad;
LayoutFindFn layoutFind;
LayoutContainerFn layoutContainer;
LayoutDeleteFn layoutDelete;
UiTimeFn getUiTime;
alignas(4) unsigned char startupLayout[0x18];
UTFWin::IWindow* startupRoot = nullptr;
UTFWin::IWindow* startupIcon = nullptr;
UTFWin::IWindow* startupRibbon = nullptr;
UTFWin::IWindow* startupText = nullptr;
int startupRibbonShade = 0, startupTextShade = 0;
bool startupShown = false, startupWaiting = false, startupLoaded = false;
bool startupHovered = false;
DWORD startupEntered = 0, startupRetry = 0;
float startupAge = 0, startupLastTick = 0;
unsigned startupAttempts = 0;

void CloseStartupBanner() {
    if (startupLoaded) {
        layoutDelete(startupLayout, 1);
        startupLoaded = false;
    }
    startupRoot = nullptr;
    startupIcon = nullptr;
    startupRibbon = nullptr;
    startupText = nullptr;
    startupHovered = false;
}

void SetStartupHover(bool hovered) {
    if (startupHovered == hovered)
        return;
    startupHovered = hovered;

    int state = startupIcon->GetState();
    startupIcon->SetState(hovered ? state | UTFWin::kStateHover : state & ~UTFWin::kStateHover);

    startupRibbon->SetShadeColor(Math::Color(hovered ? 0xff70dfff : startupRibbonShade));
    startupText->SetShadeColor(Math::Color(hovered ? 0xffa8efff : startupTextShade));
}

bool ShowStartupBanner() {
    layoutCtor(startupLayout);
    startupLoaded = true;

    if (!layoutLoad(startupLayout, u"EventLogItem", 0x40464100, true, 0x3469e7a3)) {
        CloseStartupBanner();
        return false;
    }
    auto* container = layoutContainer(startupLayout);
    auto* root = layoutFind(startupLayout, 0x35ee914, true);
    auto* unused = layoutFind(startupLayout, 0x40005d8, true);
    auto* panel = layoutFind(startupLayout, 0x2b7a911, true);
    auto* ribbon = layoutFind(startupLayout, 0x3fffadc, true);
    auto* text = layoutFind(startupLayout, 0x47d4388, true);
    auto* icon = layoutFind(startupLayout, 0x3ffb8f0, true);
    if (!container || !root || !unused || !panel || !ribbon || !text || !icon) {
        CloseStartupBanner();
        return false;
    }

    icon->AddRef();
    auto* oldParent = icon->GetParent();
    if (oldParent)
        oldParent->RemoveWindow(icon);
    root->AddWindow(icon);
    icon->Release();
    unused->SetVisible(false);
    icon->SetArea(Math::Rectangle(0, 0, 33, 36));
    icon->SetVisible(true);
    icon->SetEnabled(true);
    icon->SetIgnoreMouse(true);
    panel->SetArea(Math::Rectangle(27, 7, 330, 31));
    ribbon->SetArea(Math::Rectangle(0, 0, 303, 24));
    if (auto* placeholder = ribbon->FindWindowByID(0x35f06c8, true))
        placeholder->SetVisible(false);
    text->SetCaption(CivText::Get(CivText::Banner));
    text->SetArea(Math::Rectangle(8, 1, 293, 23));
    text->SetVisible(true);
    panel->SetVisible(true);
    ribbon->SetVisible(true);
    root->SetVisible(true);
    root->SetIgnoreMouse(true);
    root->SetFlag(UTFWin::kWinFlagIgnoreMouseChildren, true);
    container->SetIgnoreMouse(true);
    container->SetFlag(UTFWin::kWinFlagIgnoreMouseChildren, true);
    startupRoot = root;
    startupIcon = icon;
    startupRibbon = ribbon;
    startupText = text;
    startupRibbonShade = ribbon->GetShadeColor();
    startupTextShade = text->GetShadeColor();
    return true;
}

void StartupBannerTick() {
    if (!enabled || !getAppModes || (startupShown && !startupLoaded))
        return;
    auto* modes = getAppModes();
    bool inGalaxy = modes && modes->GetActiveModeID() == kGGEMode;
    DWORD now = GetTickCount();
    if (!inGalaxy) {
        CloseStartupBanner();
        startupWaiting = false;
        return;
    }
    if (!startupShown) {
        DWORD foregroundProcess = 0;
        auto foreground = GetForegroundWindow();
        if (foreground)
            GetWindowThreadProcessId(foreground, &foregroundProcess);
        if (foregroundProcess != GetCurrentProcessId()) {
            startupWaiting = false;
            return;
        }
        if (!startupWaiting) {
            startupEntered = now;
            startupWaiting = true;
        }
        if (now - startupEntered < 1500 || startupAttempts >= 5 || (startupAttempts && now - startupRetry < 2000))
            return;
        startupRetry = now;
        ++startupAttempts;
        if (!ShowStartupBanner()) {
            return;
        }
        startupShown = true;
        startupAge = 0;
        startupLastTick = getUiTime();
    }

    float uiNow = getUiTime();
    float elapsed = uiNow - startupLastTick;
    startupLastTick = uiNow;

    if (!startupHovered && elapsed > 0.0f)
        startupAge += elapsed;
    if (startupAge >= 12.0f) {
        CloseStartupBanner();
        return;
    }
    auto* container = layoutContainer(startupLayout);
    if (!container) {
        CloseStartupBanner();
        return;
    }
    float height = container->GetRealArea().GetHeight();

    float y = height > 240 ? height - 226 : 14;
    startupRoot->SetArea(Math::Rectangle(16, y, 18, y + 2));
    POINT mouse{};
    DWORD foregroundProcess = 0;
    auto foreground = GetForegroundWindow();
    if (foreground)
        GetWindowThreadProcessId(foreground, &foregroundProcess);
    bool hovered = foregroundProcess == GetCurrentProcessId() && GetCursorPos(&mouse) &&
                   ScreenToClient(foreground, &mouse) && mouse.x >= 16 && mouse.x < 346 && mouse.y >= y &&
                   mouse.y < y + 36;
    SetStartupHover(hovered);

    unsigned alpha = hovered             ? 255
                     : startupAge < 0.3f ? unsigned(startupAge * 255 / 0.3f)
                                         : (startupAge > 10.0f ? unsigned((12.0f - startupAge) * 255 / 2.0f) : 255);
    startupRoot->SetShadeColor(Math::Color((alpha << 24) | 0xffffff));
}

cCivilization* Player() {
    if (!enabled || !IsCivGame())
        return nullptr;
    auto* p = GameNounManager.GetPlayerCivilization();
    return p && p->mIsPlayerOwned ? p : nullptr;
}

bool PlayerCity(cCity* city) {
    auto* p = Player();
    return p && city && city->mpCivilization.get() == p;
}

bool PlayerCivilization(cCivilization* civilization) {
    auto* p = Player();
    return p && civilization == p;
}

bool PlayerVehicle(cVehicle* vehicle) {
    auto* p = Player();
    return p && vehicle && vehicle->cGameData::mPoliticalID == p->mPoliticalID;
}

unsigned SpiceCount(cCivilization* player, cCommodityNode* exclude = nullptr, bool pending = false) {
    unsigned count = 0;
    for (auto& ptr : GetData<cCommodityNode>()) {
        auto* n = ptr.get();
        if (!n || n == exclude || n->mbIsDestroyed)
            continue;
        if (n->cGameData::mPoliticalID == player->mPoliticalID ||
            (pending && n->mMineState == 1 && n->mConstructingPoliticalID == player->mPoliticalID))
            ++count;
    }
    return count;
}

bool BlockSpice(cCommodityNode* node, uint32_t id, bool pending = false) {
    if (!CivSettings::Enabled(CivSettings::Spice))
        return false;
    auto* p = Player();
    return !normalizing && node && p && id == p->mPoliticalID && node->cGameData::mPoliticalID != id &&
           SpiceCount(p, node, pending) >= MaxSpice;
}

bool ForeignSource(cCommodityNode* node, uint32_t id) {
    uint32_t owner = node->cGameData::mPoliticalID;
    return owner != uint32_t(-1) && owner != id;
}

bool RefuseSpiceOrder(cVehicle* v, cCommodityNode* node) {
    return PlayerVehicle(v) && BlockSpice(node, v->cGameData::mPoliticalID, true) &&
           (!ForeignSource(node, v->cGameData::mPoliticalID) || v->mPurpose == kVehicleEconomic);
}

bool BlockRaid(cVehicle* v, cGameData* target) {
    return CivSettings::Enabled(CivSettings::LandRaids) && target && v && v->mLocomotion == kVehicleLand &&
           PlayerVehicle(v) && (object_cast<cTribeHut>(target) || object_cast<cTribe>(target));
}

using OrderFn = void(__thiscall*)(cVehicle*, cGameData*, int, int);
using CreateTurretFn = cTurret*(__thiscall*)(cCity*);
using AutoTurretFn = bool(__thiscall*)(cCity*);
using SlotTurretFn = bool(__thiscall*)(cCity*, int);
using BeginMineFn = void(__thiscall*)(cCommodityNode*, uint32_t, cVehicle*);
using CaptureMineFn = void(__thiscall*)(cCommodityNode*, uint32_t, int);
using MineDamageFn = int(__thiscall*)(cCombatant*, float, uint32_t, int, const Math::Vector3&, cCombatant*);
using MineUpdateFn = void(__thiscall*)(cCommodityNode*);
using ClearOrderFn = void(__thiscall*)(cVehicle*);
using RemoveTurretFn = void(__thiscall*)(cCity*, cTurret*, bool);
using ReleaseMineFn = void(__thiscall*)(cCommodityNode*);
using SuperweaponAvailableFn = bool(__thiscall*)(cCivilization*, int);
using LaunchSuperweaponFn = void(__thiscall*)(cCivilization*, int, const Math::Vector3&);
using CitySuperweaponFn = void(__thiscall*)(cCity*, int, const Math::Vector3&);

struct CityEditorLimitsView {
    void* vtable;
    int references;
    cCity* city;
};

static_assert(sizeof(CityEditorLimitsView) == 0xc, "City editor limits ABI");
using CityEditorCapacityFn = int(__thiscall*)(CityEditorLimitsView*, int);
using SelectSuperweaponFn = void(__thiscall*)(void*, int);
using PaletteClickFn = void(__thiscall*)(void*);

struct UiMessageView {
    UTFWin::IWindow* source;
    UTFWin::IWindow* destination;
    int type;
    uint32_t argument0, argument1, argument2, argument3;
};

static_assert(sizeof(UiMessageView) == 0x1c, "UI message ABI");
using WindowDispatchFn = int(__thiscall*)(void*, UiMessageView*, bool);

struct CnvActionView {
    uint32_t id;
    uint32_t instance, type, group;
};

static_assert(sizeof(CnvActionView) == 0x10, "Conversation action ABI");
using CommActionFn = void(__thiscall*)(void*, const CnvActionView*, cCivilization*, cCity*, cCity*);
using SetResponseFn = void(__thiscall*)(void*, int, const char16_t*, uint32_t);
using CommManagerFn = char* (*)();
constexpr uint32_t ComplimentAction = 0x70c14c34, ContinueAction = 0xcaff0f72, GotoDialogAction = 0x02d5dcec;
constexpr uint32_t GiftDialog = 0xee4f8f7f, CivConversation = 0xdbf385bf;
constexpr uint32_t ResponseCommand = 0x04b417d4, ResponseListID = 0x04adc3bf;

OrderFn order;
CreateTurretFn createTurret;
AutoTurretFn autoTurret;
SlotTurretFn slotTurret;
BeginMineFn beginMine;
CaptureMineFn captureMine;
MineDamageFn mineDamage;
MineUpdateFn mineUpdate;
ClearOrderFn clearOrder;
RemoveTurretFn removeTurret;
ReleaseMineFn releaseMine;
SuperweaponAvailableFn superweaponAvailable;
LaunchSuperweaponFn launchSuperweapon;
CitySuperweaponFn citySuperweapon;
CityEditorCapacityFn cityEditorCapacity;
SelectSuperweaponFn selectSuperweapon;
PaletteClickFn paletteClick;
WindowDispatchFn windowDispatch;
CommActionFn commAction;
SetResponseFn setResponse;
CommManagerFn commManager;
constexpr unsigned MaxNations = 64;
constexpr uint32_t LedgerID = 0x8cbde32d, LedgerMagic = 0x4c474343;
constexpr uint16_t LedgerVersion = 2;

struct GiftTally {
    uint32_t nation, gifts;
};

static_assert(sizeof(GiftTally) == 8, "Saved gift tally");
GiftTally giftTally[MaxNations]{};
unsigned giftNations = 0;

uint32_t NationOf(cGameData* object) {
    auto* civilization = object ? object_cast<cCivilization>(object) : nullptr;
    return civilization ? civilization->cGameData::mPoliticalID : uint32_t(-1);
}

unsigned GiftsTo(uint32_t nation) {
    for (unsigned i = 0; i < giftNations; ++i)
        if (giftTally[i].nation == nation)
            return giftTally[i].gifts;
    return 0;
}

void CountGift(uint32_t nation) {
    for (unsigned i = 0; i < giftNations; ++i)
        if (giftTally[i].nation == nation) {
            if (giftTally[i].gifts < 1000)
                ++giftTally[i].gifts;
            return;
        }
    if (giftNations < MaxNations)
        giftTally[giftNations++] = {nation, 1};
}

IO::IStream* LedgerStream(Resource::IRecord* record) {
    return record ? record->GetStream() : nullptr;
}

}

namespace CivChallenge {
class GiftLedger final : public Simulator::ISimulatorStrategy {
public:
    int AddRef() override {
        return 2;
    }

    int Release() override {
        return 1;
    }

    void Initialize() override {}

    void Dispose() override {}

    const char* GetName() const override {
        return "CivChallenge::GiftLedger";
    }

    void OnModeExited(uint32_t, uint32_t) override {}

    void OnModeEntered(uint32_t, uint32_t) override {}

    uint32_t GetLastGameMode() const override {
        return uint32_t(-1);
    }

    uint32_t GetCurrentGameMode() const override {
        return uint32_t(-1);
    }

    bool func24h(uint32_t) override {
        return false;
    }

    bool Write(Simulator::ISerializerStream* stream) override {
        auto* s = stream ? LedgerStream(static_cast<Simulator::ISerializerWriteStream*>(stream)->GetRecord()) : nullptr;
        if (!s)
            return false;
        uint32_t record[3 + 2 * MaxNations]{LedgerMagic, LedgerVersion | uint32_t(4 + 8 * giftNations) << 16,
                                            giftNations};
        for (unsigned i = 0; i < giftNations; ++i) {
            record[3 + 2 * i] = giftTally[i].nation;
            record[4 + 2 * i] = giftTally[i].gifts;
        }
        s->Write(record, 12 + 8 * giftNations);
        return true;
    }

    bool Read(Simulator::ISerializerStream* stream) override {
        giftNations = 0;
        auto* s = stream ? LedgerStream(static_cast<Simulator::ISerializerReadStream*>(stream)->GetRecord()) : nullptr;
        uint32_t header[2]{};
        if (!s || s->GetAvailable() < 8 || s->Read(header, 8) != 8 || header[0] != LedgerMagic)
            return false;
        int body = int(header[1] >> 16), used = 0;
        if (s->GetAvailable() < body)
            return false;
        uint32_t count = 0;
        if ((header[1] & 0xffff) >= LedgerVersion && body >= 4) {
            if (s->Read(&count, 4) != 4)
                return false;
            used = 4;
            while (count-- && used + 8 <= body) {
                GiftTally tally{};
                if (s->Read(&tally, 8) != 8)
                    return false;
                used += 8;
                if (giftNations < MaxNations && tally.gifts)
                    giftTally[giftNations++] = {tally.nation, tally.gifts > 1000 ? 1000 : tally.gifts};
            }
        }
        if (body > used)
            s->SetPosition(body - used, IO::PositionType::Current);
        return true;
    }

    void OnLoad(const Simulator::cSavedGameHeader&) override {}

    bool WriteToXML(Simulator::XmlSerializer*) override {
        return false;
    }

    void Update(int, int) override {}

    void PostUpdate(int, int) override {}

    void func40h(uint32_t) override {}

    void func44h(uint32_t) override {}

    void func48h() override {}

    void func4Ch() override {}
};
}

namespace {
CivChallenge::GiftLedger ledger;

using LoadSaveFn = void(__thiscall*)(void*);
using NewGameFn = void (*)(uint32_t);
LoadSaveFn loadSave;
NewGameFn newGame;

void __fastcall LoadSaveHook(void* persistence, void*) {
    giftNations = 0;
    loadSave(persistence);
}

void NewGameHook(uint32_t mode) {
    giftNations = 0;
    newGame(mode);
}

int GiftCost(uint32_t action) {
    switch (action) {
    case 0xd648ed4d:
        return 4000;
    case 0x5ea6d3ed:
        return 2000;
    case 0x410c4a65:
        return 1000;
    }
    return 0;
}

char* CurrentCivEvent() {
    auto* manager = commManager && Player() ? commManager() : nullptr;
    auto* event = manager ? *reinterpret_cast<char**>(manager + 0x20) : nullptr;
    return event && *reinterpret_cast<int*>(event + 0xc) == 1 &&
                   *reinterpret_cast<uint32_t*>(event + 0x38) == CivConversation
               ? event
               : nullptr;
}

const CnvActionView* ResponseAction(char* event, int index) {
    auto* fixed = reinterpret_cast<CnvActionView**>(event + 0x64);
    auto* dynamic = reinterpret_cast<CnvActionView**>(event + 0x78);
    int fixedCount = int(fixed[1] - fixed[0]), dynamicCount = int(dynamic[1] - dynamic[0]);
    if (index < 0)
        return nullptr;
    if (index < fixedCount)
        return fixed[0] + index;
    index -= fixedCount;
    return index < dynamicCount ? dynamic[0] + index : nullptr;
}

uint32_t EventNation(char* event) {
    auto* partner = *reinterpret_cast<cGameData**>(event + 0x28);
    if (!partner) {
        auto* city = *reinterpret_cast<cCity**>(event + 0x20);
        partner = city ? city->mpCivilization.get() : nullptr;
    }
    return NationOf(partner);
}

bool ResponseBlocked(char* event, const CnvActionView* action) {
    if (!action)
        return false;
    if (action->id == ComplimentAction)
        return CivSettings::Enabled(CivSettings::Compliments);
    bool gift = GiftCost(action->id) ||
                (action->id == GotoDialogAction && action->instance == GiftDialog && action->group == CivConversation);
    return gift && CivSettings::Enabled(CivSettings::Gifts) && GiftsTo(EventNation(event)) >= MaxGifts;
}

int ResponseIndex(UTFWin::IWindow* button) {
    auto* list = button->GetParent();
    if (!list || list->GetControlID() != ResponseListID)
        return -1;
    int index = 0;
    for (auto it = list->GetChildrenBegin(); it != list->GetChildrenEnd(); ++it, ++index)
        if (static_cast<UTFWin::IWindow*>(&static_cast<UTFWin::Window_intrusive_list_node&>(*it)) == button)
            return index;
    return -1;
}

void __fastcall SetResponseHook(void* ui, void*, int index, const char16_t* caption, uint32_t enabled) {
    if (enabled & 0xff)
        if (auto* event = CurrentCivEvent())
            if (ResponseBlocked(event, ResponseAction(event, index)))
                enabled = 0;
    setResponse(ui, index, caption, enabled);
}

void __fastcall SelectSuperweaponHook(void* controller, void*, int ability) {
    if (CivSettings::Enabled(CivSettings::Superweapons) && ability >= 0 && ability < 12 && Player()) {
        RejectSound();
        return;
    }
    selectSuperweapon(controller, ability);
}

int __fastcall WindowDispatchHook(void* manager, void*, UiMessageView* msg, bool sendToSource) {
    if (CivSettings::Enabled(CivSettings::Superweapons) && msg && msg->type == 6 && msg->argument3 == 1000 &&
        Player()) {
        auto* window = sendToSource ? msg->source : msg->destination;
        for (unsigned depth = 0; window && depth < 24; ++depth) {
            uint32_t id = window->GetControlID();
            if (id == 0x5a55b10 || id == 0x5a55b20 || id == 0x5a55b30 || id == 0x5a55b40) {
                RejectSound();
                break;
            }
            window = window->GetParent();
        }
    }
    if (msg && msg->type == 6 && msg->argument3 == 1000)
        if (auto* event = CurrentCivEvent()) {
            auto* button = sendToSource ? msg->source : msg->destination;
            if (button && button->GetCommandID() == ResponseCommand &&
                !(button->GetFlags() & UTFWin::kWinFlagEnabled) &&
                ResponseBlocked(event, ResponseAction(event, ResponseIndex(button))))
                RejectSound();
        }
    return windowDispatch(manager, msg, sendToSource);
}

void __fastcall PaletteClickHook(void* viewer, void*) {
    if (CivSettings::Enabled(CivSettings::Turrets) && viewer && Player()) {
        auto* bytes = static_cast<char*>(viewer);
        auto* item = *reinterpret_cast<char**>(bytes + 0x180);
        auto* info = *reinterpret_cast<char**>(bytes + 0x17c);
        if (item && info && *reinterpret_cast<uint32_t*>(item + 0x24) == 0x0142462a) {
            auto* limits = *reinterpret_cast<CityEditorLimitsView**>(info + 8);
            if (limits && reinterpret_cast<uintptr_t>(limits->vtable) == cityEditorLimitsVtable &&
                PlayerCity(limits->city) && limits->city->mTurrets.size() >= MaxTurrets) {
                RejectSound();
                return;
            }
        }
    }
    paletteClick(viewer);
}

int __fastcall CityEditorCapacityHook(CityEditorLimitsView* limits, void*, int index) {
    int available = cityEditorCapacity(limits, index);
    if (CivSettings::Enabled(CivSettings::Turrets) && index == 2 && limits && PlayerCity(limits->city)) {
        unsigned count = limits->city->mTurrets.size();
        int remaining = count >= MaxTurrets ? 0 : int(MaxTurrets - count);
        if (available > remaining)
            available = remaining;
    }
    return available;
}

bool __fastcall SuperweaponAvailableHook(cCivilization* civilization, void*, int ability) {
    return (!CivSettings::Enabled(CivSettings::Superweapons) || !PlayerCivilization(civilization)) &&
           superweaponAvailable(civilization, ability);
}

void __fastcall LaunchSuperweaponHook(cCivilization* civilization, void*, int ability, const Math::Vector3& target) {
    if (CivSettings::Enabled(CivSettings::Superweapons) && PlayerCivilization(civilization)) {
        RejectSound();
        return;
    }

    launchSuperweapon(civilization, ability, target);
}

void __fastcall CitySuperweaponHook(cCity* city, void*, int ability, const Math::Vector3& target) {
    if (CivSettings::Enabled(CivSettings::Superweapons) && PlayerCity(city)) {
        return;
    }
    citySuperweapon(city, ability, target);
}

void __fastcall OrderHook(cVehicle* v, void*, cGameData* target, int action, int flags) {
    if (BlockRaid(v, target)) {
        RejectSound();
        return;
    }
    if (target && RefuseSpiceOrder(v, object_cast<cCommodityNode>(target))) {
        RejectSound();
        return;
    }
    order(v, target, action, flags);
}

void __fastcall CommActionHook(void* manager, void*, const CnvActionView* action, cCivilization* source,
                               cCity* sourceCity, cCity* targetCity) {
    auto* p = action ? Player() : nullptr;
    bool limited = CivSettings::Enabled(CivSettings::Gifts);
    int cost = p && limited ? GiftCost(action->id) : 0;
    uint32_t nation = cost ? NationOf(reinterpret_cast<cGameData*>(source)) : 0;
    if (p && ((action->id == ComplimentAction && CivSettings::Enabled(CivSettings::Compliments)) ||
              (cost && GiftsTo(nation) >= MaxGifts))) {
        RejectSound();
        CnvActionView back{ContinueAction, 0, 0, 0};
        commAction(manager, &back, source, sourceCity, targetCity);
        return;
    }
    bool paid = cost && float(cost) <= p->mWealth;
    commAction(manager, action, source, sourceCity, targetCity);
    if (paid)
        CountGift(nation);
}

bool TurretBlocked(cCity* city) {
    if (CivSettings::Enabled(CivSettings::Turrets) && PlayerCity(city) && city->mTurrets.size() >= MaxTurrets) {
        RejectSound();
        return true;
    }
    return false;
}

cTurret* __fastcall CreateTurretHook(cCity* city, void*) {
    return TurretBlocked(city) ? nullptr : createTurret(city);
}

bool __fastcall AutoTurretHook(cCity* city, void*) {
    return !TurretBlocked(city) && autoTurret(city);
}

bool __fastcall SlotTurretHook(cCity* city, void*, int slot) {
    return !TurretBlocked(city) && slotTurret(city, slot);
}

void ReleaseSource(cCommodityNode* node) {
    bool nested = normalizing;
    normalizing = true;
    releaseMine(node);
    normalizing = nested;
    node->mConstructingPoliticalID = uint32_t(-1);
    node->mConstructingVehicle = nullptr;
}

void __fastcall BeginMineHook(cCommodityNode* node, void*, uint32_t id, cVehicle* v) {
    if (BlockSpice(node, id, true)) {
        if (ForeignSource(node, id)) {
            ReleaseSource(node);
            RejectSound();
        }
        return;
    }
    beginMine(node, id, v);
}

void __fastcall CaptureMineHook(cCommodityNode* node, void*, uint32_t id, int method) {
    if (method == 0 ? BlockSpice(node, id) : method > 0 && BlockSpice(node, id, true)) {
        if (method > 0 && ForeignSource(node, id)) {
            ReleaseSource(node);
            if (method == 2 || method == 3)
                RejectSound();
        }
        return;
    }
    captureMine(node, id, method);
}

int __fastcall MineDamageHook(cCombatant* combatant, void*, float amount, uint32_t id, int kind,
                              const Math::Vector3& direction, cCombatant* attacker) {
    auto* node = reinterpret_cast<cCommodityNode*>(reinterpret_cast<char*>(combatant) - 0x108);
    if (BlockSpice(node, id, true) && !ForeignSource(node, id)) {
        return 0;
    }
    return mineDamage(combatant, amount, id, kind, direction, attacker);
}

void __fastcall MineUpdateHook(cCommodityNode* node, void*) {
    if (node->mMineState == 1 && BlockSpice(node, node->mConstructingPoliticalID))
        ReleaseSource(node);
    mineUpdate(node);
}

void Update() {
    CivSettings::UpdateUI();
    StartupBannerTick();
    auto* p = Player();
    if (!p)
        return;
    for (auto& cp : p->mCities) {
        auto* city = cp.get();
        if (!city || city->mpCivilization.get() != p)
            continue;
        while (CivSettings::Enabled(CivSettings::Turrets) && city->mTurrets.size() > MaxTurrets) {
            size_t before = city->mTurrets.size();
            auto* turret = city->mTurrets.back().get();
            cGameDataPtr keepAlive = static_cast<cGameData*>(turret);
            removeTurret(city, turret, false);
            if (city->mTurrets.size() >= before) {
                break;
            }
        }
    }

    unsigned retained = 0;
    auto nodes = GetData<cCommodityNode>().data;
    for (auto& np : nodes) {
        auto* n = np.get();
        if (!n || n->mbIsDestroyed)
            continue;
        if (CivSettings::Enabled(CivSettings::Spice) && n->mMineState == 3 &&
            n->cGameData::mPoliticalID == uint32_t(-1)) {
            ReleaseSource(n);
            continue;
        }
        if (n->cGameData::mPoliticalID != p->mPoliticalID)
            continue;
        if (CivSettings::Enabled(CivSettings::Spice) && ++retained > MaxSpice)
            ReleaseSource(n);
    }
    for (auto& vp : p->mVehicles) {
        auto* v = vp.get();
        if (!v)
            continue;

        auto* begin = *reinterpret_cast<char**>(reinterpret_cast<char*>(v) + 0xb68);
        auto* end = *reinterpret_cast<char**>(reinterpret_cast<char*>(v) + 0xb6c);
        if (begin && begin < end) {
            auto* target = *reinterpret_cast<cGameData**>(begin);
            auto* n = target ? object_cast<cCommodityNode>(target) : nullptr;
            if (BlockRaid(v, target) || RefuseSpiceOrder(v, n))
                clearOrder(v);
        }
    }
}

}

namespace CivChallenge {
class Listener final : public App::IUnmanagedMessageListener {
    bool HandleMessage(uint32_t, void*) override {
        Update();
        return false;
    }
};
}

namespace {
CivChallenge::Listener listener;

struct Hook {
    const char* name;
    uint32_t address;
    void** original;
    void* replacement;
    const char* expected;
    size_t size;
};

struct NativeCheck {
    uint32_t address;
    const char* expected;
    size_t size;
};

#include "core/Hooks.hpp"

bool HostMatches(uintptr_t base) {
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    return nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 && nt->OptionalHeader.SizeOfImage == HostImageSize &&
           nt->FileHeader.TimeDateStamp == HostTimestamp;
}

constexpr unsigned HookCount = sizeof(hooks) / sizeof(hooks[0]);
bool chained[HookCount]{};

bool Install() {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (!HostMatches(base))
        return false;
    for (unsigned i = 0; i < HookCount; ++i) {
        auto& h = hooks[i];
        auto* ptr = reinterpret_cast<void*>(base + h.address - 0x400000);
        if (!memcmp(ptr, h.expected, h.size)) {
            *h.original = ptr;
            continue;
        }
        uintptr_t next = HookChain::ForeignTarget(ptr, base, HostImageSize);
        if (!next)
            return false;
        *h.original = reinterpret_cast<void*>(next);
        chained[i] = true;
    }
    for (auto& h : audioChecks) {
        if (memcmp(reinterpret_cast<void*>(base + h.address - 0x400000), h.expected, h.size)) {
            return false;
        }
    }
    for (auto& h : startupChecks) {
        if (memcmp(reinterpret_cast<void*>(base + h.address - 0x400000), h.expected, h.size)) {
            return false;
        }
    }
    for (auto& h : settingsChecks) {
        if (memcmp(reinterpret_cast<void*>(base + h.address - 0x400000), h.expected, h.size))
            return false;
    }
    for (auto& h : diplomacyChecks) {
        if (memcmp(reinterpret_cast<void*>(base + h.address - 0x400000), h.expected, h.size))
            return false;
    }
    getAppModes = reinterpret_cast<AppModesFn>(base + 0x67dcd0 - 0x400000);
    layoutCtor = reinterpret_cast<LayoutCtorFn>(base + 0x8100a0 - 0x400000);
    layoutLoad = reinterpret_cast<LayoutLoadFn>(base + 0x812250 - 0x400000);
    layoutFind = reinterpret_cast<LayoutFindFn>(base + 0x810650 - 0x400000);
    layoutContainer = reinterpret_cast<LayoutContainerFn>(base + 0x810610 - 0x400000);
    layoutDelete = reinterpret_cast<LayoutDeleteFn>(base + 0x811bc0 - 0x400000);
    getUiTime = reinterpret_cast<UiTimeFn>(base + 0x805170 - 0x400000);
    CivSettings::BindUI(base);
    createAudioTrack = reinterpret_cast<CreateAudioTrackFn>(base + 0x436350 - 0x400000);
    playAudio = reinterpret_cast<PlayAudioFn>(base + 0x436390 - 0x400000);
    cityEditorLimitsVtable = base + 0x14793f4 - 0x400000;
    clearOrder = reinterpret_cast<ClearOrderFn>(base + 0xcaaaa0 - 0x400000);
    removeTurret = reinterpret_cast<RemoveTurretFn>(base + 0xbe21c0 - 0x400000);
    releaseMine = reinterpret_cast<ReleaseMineFn>(base + 0xbff890 - 0x400000);
    commManager = reinterpret_cast<CommManagerFn>(base + 0xb3d5a0 - 0x400000);
    LONG err = DetourTransactionBegin();
    if (err != NO_ERROR)
        return false;
    err = DetourUpdateThread(GetCurrentThread());
    for (unsigned i = 0; i < HookCount; ++i)
        if (err == NO_ERROR && !chained[i])
            err = DetourAttach(hooks[i].original, hooks[i].replacement);
    if (err != NO_ERROR) {
        DetourTransactionAbort();
        return false;
    }
    err = DetourTransactionCommit();
    if (err != NO_ERROR) {
        return false;
    }
    for (unsigned i = 0; i < HookCount; ++i)
        if (chained[i] &&
            !HookChain::JumpTo(reinterpret_cast<void*>(base + hooks[i].address - 0x400000), hooks[i].replacement))
            return false;
    return true;
}

void Init() {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    auto* locale = reinterpret_cast<void*>(base + localeChecks[0].address - 0x400000);
    if (HostMatches(base) && !memcmp(locale, localeChecks[0].expected, localeChecks[0].size))
        CivText::Bind(reinterpret_cast<uintptr_t>(locale));
    enabled = Install();
    if (enabled) {
        CivSettings::Load();
        ModAPI::AddSimulatorStrategy(&ledger, LedgerID);
        App::IMessageManager::Get()->AddUnmanagedListener(&listener, App::kMsgAppUpdate);
    } else
        MessageBoxW(nullptr, reinterpret_cast<const wchar_t*>(CivText::Get(CivText::ErrorText)),
                    reinterpret_cast<const wchar_t*>(CivText::Get(CivText::ErrorTitle)), MB_OK | MB_ICONERROR);
}
}

BOOL WINAPI DllMain(HMODULE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        ModAPI::AddPostInitFunction(Init);
    }
    return TRUE;
}

