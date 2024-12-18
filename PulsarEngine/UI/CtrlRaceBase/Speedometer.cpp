#include <UI/CtrlRaceBase/Speedometer.hpp>
#include <Settings/Settings.hpp>

// CTGP SPEEDOMETER WRITTEN BY KAZUKI!!! thank you kazuki i love you

namespace Pulsar {
namespace UI {
u32 CtrlRaceSpeedo::Count() {
    //disable multi players
	const u8 val = Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_SOM);
    if(val == RACESETTING_SOM_DISABLED) return 0;
    u32 localPlayerCount = Racedata::sInstance->racesScenario.localPlayerCount;
    if(localPlayerCount != 1)return 0;
    return 1;
}

void CtrlRaceSpeedo::Create(Page& page, u32 index, u32 count) {
    for(int i = 0; i < count; ++i) {
        CtrlRaceSpeedo* som = new(CtrlRaceSpeedo);
        page.AddControl(index + i, *som, 0);
        som->Load("CtrlSpeedometer_1_0", i);
    }
}
void CtrlRaceSpeedo::DigitizingNumbers(wchar_t *string){
    int index = 0;
    while(string[index] != 0){
        if(string[index] >= 0x30 && string[index] < 0x3a)string[index] +=  0x2430; //0~9
        if(string[index] == 0x2E)string[index] = 0x246b;//.
        index++;
    }
}

static CustomCtrlBuilder SOM(CtrlRaceSpeedo::Count, CtrlRaceSpeedo::Create);

void CtrlRaceSpeedo::Load(const char* variant, u8 id) {
    this->hudSlotId = id;
    ControlLoader loader(this);
    const char* anims[] ={
        nullptr,
        nullptr
    };

    loader.Load(UI::raceFolder, "speedometer", "CtrlSpeedometer_1_0", anims);
    return;
}

void CtrlRaceSpeedo::Init() {
    LayoutUIControl::Init();
    return;
}

void CtrlRaceSpeedo::OnUpdate() {
    this->UpdatePausePosition();
    const u8 digits = Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_SCROLL_SOM);
    const Kart::Pointers& pointers = Kart::Manager::sInstance->players[this->GetPlayerId()]->pointers;
    const Kart::Physics* physics = pointers.kartBody->kartPhysicsHolder->physics;

    Vec3 sum;
    MTX::PSVECAdd(&physics->engineSpeed, &physics->speed2, &sum);
    MTX::PSVECAdd(&physics->speed3, &sum, &sum);
    float speed = MTX::PSVECMag(&sum);
    float speedCap = pointers.kartMovement->hardSpeedLimit;
    if(speed > speedCap) speed = speedCap;
    wchar_t speedString[0x10];
    switch (digits){
        default:
        case 0:
        swprintf(speedString, (unsigned long)0x10, L"%.0f", speed);
        break;
        case 1:
        swprintf(speedString, (unsigned long)0x10, L"%.1f", speed);
        break;
        case 2:
        swprintf(speedString, (unsigned long)0x10, L"%.2f", speed);
        break;
        case 3:
        swprintf(speedString, (unsigned long)0x10, L"%.3f", speed);
        break;
    }
    DigitizingNumbers(speedString);
    Text::Info textInfo;
    textInfo.strings[0] = speedString;
    SetMessage(BMG_TEXT, &textInfo);
    return;
}
}//namespace UI
}//namespace Pulsar