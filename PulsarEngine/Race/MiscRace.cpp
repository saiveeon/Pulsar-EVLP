#include <kamek.hpp>
#include <MarioKartWii/Archive/ArchiveMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/UI/Ctrl/CtrlRace/CtrlRaceBalloon.hpp>
#include <MarioKartWii/UI/Ctrl/CtrlRace/CtrlRaceResult.hpp>
#include <MarioKartWii/GlobalFunctions.hpp>
#include <MarioKartWii/Driver/DriverManager.hpp>
#include <Settings/Settings.hpp>

namespace Pulsar {
namespace Race {
	
// change transmission, code from brawlbox and thegamingbram
static void *GetCustomKartParam(ArchiveMgr *archive, ArchiveSource type, const char *name, u32 *length){
	const u8 ekwdrift = Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_EKW, SETTINGEKW_DRIFTS);
	const u8 ekwstat = Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_EKW, SETTINGEKW_STATMOD);
	if(System::sInstance->IsContext(PULSAR_MEGATC)) {
		if (ekwdrift == EKWSETTING_DRIFTS_INSIDE){ 
			name = "kartParamVI.bin";
		}
		if (ekwdrift == EKWSETTING_DRIFTS_OUTSIDE){ 
			name = "kartParamVO.bin";
		}
		if (ekwdrift == EKWSETTING_DRIFTS_BIKEIN){ 
			name = "kartParamVB.bin";
		}
		if (ekwdrift == EKWSETTING_DRIFTS_KARTIN){ 
			name = "kartParamVK.bin";
		}
		if (ekwdrift == EKWSETTING_DRIFTS_INVERT){ 
			name = "kartParamVX.bin";
		}
		if (ekwstat == EKWSETTING_STATMOD_ENABLED) {
			if (ekwdrift == EKWSETTING_DRIFTS_DEFAULT){ 
				name = "kartParamA.bin";
			}
			if (ekwdrift == EKWSETTING_DRIFTS_INSIDE){ 
				name = "kartParamI.bin";
			}
			if (ekwdrift == EKWSETTING_DRIFTS_OUTSIDE){ 
				name = "kartParamO.bin";
			}
			if (ekwdrift == EKWSETTING_DRIFTS_BIKEIN){ 
				name = "kartParamB.bin";
			}
			if (ekwdrift == EKWSETTING_DRIFTS_KARTIN){ 
				name = "kartParamK.bin";
			}
			if (ekwdrift == EKWSETTING_DRIFTS_INVERT){ 
				name = "kartParamX.bin";
			}
		}
	}
	return archive->GetFile(type, name, length);
}
kmCall(0x80591a30, GetCustomKartParam);

static void *GetCustomItemSlot(ArchiveMgr *archive, ArchiveSource type, const char *name, u32 *length){
    const u8 ekwitem = Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_EKW, SETTINGEKW_CPUITEM);
    if (ekwitem == EKWSETTING_CPUITEM_BRUTAL){
		name = "ItemSlotZ.bin";
    }
    return archive->GetFile(type, name, length);
}
kmCall(0x807bb128, GetCustomItemSlot);
kmCall(0x807bb030, GetCustomItemSlot);
kmCall(0x807bb200, GetCustomItemSlot);
kmCall(0x807bb53c, GetCustomItemSlot);
kmCall(0x807bbb58, GetCustomItemSlot);

void *GetCustomKartAIParam(ArchiveMgr *archive, ArchiveSource type, const char *name, u32 *length){
    const u8 ekwicp = Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_EKW, SETTINGEKW_IMPOSSIBLECPU);
    if (ekwicp == EKWSETTING_IMPOSSIBLECPU1){
		name = "kartAISpdParam1.bin";
    }
	if (ekwicp == EKWSETTING_IMPOSSIBLECPU2){
		name = "kartAISpdParam2.bin";
    }
	if (ekwicp == EKWSETTING_IMPOSSIBLECPU3){
		name = "kartAISpdParam3.bin";
    }
    return archive->GetFile(type, name, length);
}
kmCall(0x8073ae9c, GetCustomKartAIParam);

static void NonGhostPlayerCount(RacedataScenario& scenario, u8* playerCount, u8* screenCount, u8* localPlayerCount) {
    scenario.ComputePlayerCounts(playerCount, screenCount, localPlayerCount);
    System* system = System::sInstance;
    u8 realPlayers = *playerCount;
    if (scenario.settings.gamemode != MODE_TIME_TRIAL) for (int i = 0; i < 12; ++i) if (scenario.players[i].playerType == PLAYER_GHOST) --realPlayers;
    system->nonTTGhostPlayersCount = realPlayers;
}
kmCall(0x8052fc78, NonGhostPlayerCount);

kmWrite32(0x807997e0, 0x60000000);
//Starting item for OTT and TT, id is TRIPLE_MUSHROOM by default
static void SetStartingItem(Item::PlayerInventory& inventory, ItemId id, bool isItemForcedDueToCapacity) {
    register u32 playerId;
    asm(mr playerId, r29;);
    if (Racedata::sInstance->racesScenario.players[playerId].playerType == PLAYER_CPU) return;
    const System* system = System::sInstance;
    const bool isTT = DriverMgr::isTT;
    if (isTT || system->IsContext(PULSAR_MODE_OTT)) {
        bool isFeather;
        if (isTT) {
            const TTMode mode = system->ttMode;
            isFeather = (mode == TTMODE_150_FEATHER || mode == TTMODE_200_FEATHER);
        }
        else isFeather = system->IsContext(PULSAR_FEATHER);
        if (isFeather) id = BLOOPER;
        inventory.SetItem(id, isItemForcedDueToCapacity);
        if (isFeather) inventory.currentItemCount = 3;
    }
}
kmCall(0x80799808, SetStartingItem);

//From JoshuaMK, ported to C++ by Brawlbox and adapted as a setting
static int MiiHeads(Racedata* racedata, u32 unused, u32 unused2, u8 id) {
    CharacterId charId = racedata->racesScenario.players[id].characterId;
    if (System::sInstance->IsContext(PULSAR_MIIHEADS)) {
        if (charId < MII_M) {
            if (id == 0) charId = MII_M;
            else if (RKNet::Controller::sInstance->connectionState != 0) charId = MII_M;
        }
    }
    return charId;
}
kmCall(0x807eb154, MiiHeads);
kmWrite32(0x807eb15c, 0x60000000);
kmWrite32(0x807eb160, 0x88de01b4);

//credit to XeR for finding the float address
static void BattleGlitchEnable() {
    const u8 val = Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_BATTLE);
    float maxDistance = 7500.0f;
    if (val == RACESETTING_BATTLE_GLITCH_ENABLED) maxDistance = 75000.0f;
    System* system = System::sInstance;
    if (system->IsContext(PULSAR_MODE_OTT)) {
        const Input::RealControllerHolder* controllerHolder = SectionMgr::sInstance->pad.padInfos[0].controllerHolder;
        const ControllerType controllerType = controllerHolder->curController->GetType();
        const u16 inputs = controllerHolder->inputStates[0].buttonRaw;
        const u16 newInputs = (inputs & ~controllerHolder->inputStates[1].buttonRaw);
        u32 toggleInput = PAD::PAD_BUTTON_Y;
        switch (controllerType) {
        case NUNCHUCK:
            toggleInput = WPAD::WPAD_BUTTON_DOWN;
            break;
        case WHEEL:
            toggleInput = WPAD::WPAD_BUTTON_MINUS;
            break;
        case CLASSIC:
            toggleInput = WPAD::WPAD_CL_TRIGGER_ZL;
            break;
        }
        if ((newInputs & toggleInput) == toggleInput) system->ottHideNames = !system->ottHideNames;
        if (system->ottHideNames) maxDistance -= maxDistance;
    }
    RaceBalloons::maxDistanceNames = maxDistance;
}
RaceFrameHook BattleGlitch(BattleGlitchEnable);

//wi-fi time limit expansion
kmWrite32(0x8053F3B8,0x3C60005C);
kmWrite32(0x8053EDA8,0x38838D7E);

//don't hide position tracker (MrBean35000vr)
kmWrite32(0x807F4DB8, 0x38000001);

//Draggable blue shells
static void DraggableBlueShells(Item::PlayerObj& sub) {
    if (Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_BLUES) == RACESETTING_DRAGGABLE_BLUES_DISABLED) {
        sub.isNotDragged = true;
    }
}
kmBranch(0x807ae8ac, DraggableBlueShells);

//Coloured Minimap
kmWrite32(0x807DFC24, 0x60000000);

//No Team Invincibility
kmWrite32(0x8056fd24, 0x38000000); //KartCollision::CheckKartCollision()
kmWrite32(0x80572618, 0x38000000); //KartCollision::CheckItemCollision()
kmWrite32(0x80573290, 0x38000000); //KartCollision::HandleFIBCollision()
kmWrite32(0x8068e2d0, 0x38000000); //PlayerEffects ctor
kmWrite32(0x8068e314, 0x38000000); //PlayerEffects ctor
kmWrite32(0x807a7f6c, 0x38c00000); //FIB are always red
kmWrite32(0x807b0bd4, 0x38000000); //pass TC to teammate
kmWrite32(0x807bd2bc, 0x38000000); //RaceGlobals
kmWrite32(0x807f18c8, 0x38000000); //TC alert

//Accurate Explosion Damage (MrBean, CLF)
kmWrite16(0x80572690, 0x4800);
kmWrite16(0x80569F68, 0x4800);


//CtrlItemWindow
kmWrite24(0x808A9C16, 'PUL'); //item_window_new -> item_window_PUL

const char* ChangeItemWindowPane(ItemId id, u32 itemCount) {
    const bool feather = System::sInstance->IsContext(PULSAR_FEATHER);
    const char* paneName;
    if (id == BLOOPER && feather) {
        if (itemCount == 2) paneName = "feather_2";
        else if (itemCount == 3) paneName = "feather_3";
        else paneName = "feather";
    }
    else paneName = GetItemIconPaneName(id, itemCount);
    return paneName;
}
kmCall(0x807f3648, ChangeItemWindowPane);
kmCall(0x807ef168, ChangeItemWindowPane);
kmCall(0x807ef3e0, ChangeItemWindowPane);
kmCall(0x807ef444, ChangeItemWindowPane);

kmWrite24(0x808A9FF3, 'PUL');
}//namespace Race
}//namespace Pulsar