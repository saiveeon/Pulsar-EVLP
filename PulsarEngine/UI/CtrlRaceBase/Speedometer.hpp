#ifndef _PULSOM_
#define _PULSOM_
#include <kamek.hpp>
#include <MarioKartWii/Kart/KartManager.hpp>
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <UI/CtrlRaceBase/CustomCtrlRaceBase.hpp>

// CTGP SPEEDOMETER WRITTEN BY KAZUKI!!! thank you kazuki i love you

//Simple speedometer that uses TPLs instead of text because the default font numbers have a very ugly black outline
namespace Pulsar {
namespace UI {
class CtrlRaceSpeedo : public CtrlRaceBase {
public:
    void Init() override;
    void OnUpdate() override;
    static u32 Count();
    static void Create(Page& page, u32 index, u32 count);
    static void DigitizingNumbers(wchar_t *string);
private:
    void Load(const char* variant, u8 id);
};
}//namespace UI
}//namespace Pulsar

#endif