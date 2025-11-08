#pragma once
#include "../pap3_aircraft.h"
#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>
#include <vector>
#include <cstdint>
#include <cmath>

namespace pap3::aircraft {

class FF777PAP3Profile final : public PAP3AircraftProfile {
public:
    using State = pap3::aircraft::State;

    FF777PAP3Profile();
    ~FF777PAP3Profile() override;

    // ---- PAP3AircraftProfile API ----
    bool  isEligible() const override;
    void  start(StateCallback onChanged) override;
    void  stop() override;
    State current() const override;
    void  tick() override;

    void  syncSimToHardware() override;
    void  syncSimToHardwareFromRaw(const std::uint8_t* report, int len) override;

    uint8_t mcpPowerMask() const override;
    bool    mcpHasPower() const override;

    LcdDisplayConfig getLcdDisplayConfig() const override {
        LcdDisplayConfig config;
        config.showLabels = true;
        config.showDashesWhenInactive = false;
        return config;
    }

    // Input hooks
    void onButton(std::uint8_t off, std::uint8_t mask, bool pressed) override;
    void onEncoderDelta(std::uint8_t posOff, std::int8_t delta) override;

private:
    // --- Lazy initialization (commands may not be available at plugin load)
    void  initializeCommandsAndEncoders();
    bool  _commandsInitialized{false};

    // --- Polling: sim -> State
    void  poll();

    // --- Command helpers
    void  execOnce(XPLMCommandRef cmd);
    void  repeatCmd(XPLMCommandRef inc, XPLMCommandRef dec, int8_t delta, int stepPerTick = 1);

    // --- Debounce
    bool  debounce(float& lastTs, float minDeltaSec = 0.05f) const;

    // ---------- Intent logic ----------
    struct Intent {
        bool fd_left_on{false};
        bool fd_right_on{false};
        bool at_arm_on{false};
        bool ap_engaged{false};
    };

    void  applyIntent();
    void  seedFromSim();
    void  seedFromRaw(const std::uint8_t* r, int len);

    // Toggle helper
    static void toggleToPosIfNeeded(XPLMDataRef posRef, int desiredPos, XPLMCommandRef toggleCmd, float& guardTs, int& lastCommandedPos);
    
    static inline int  dr_get_pos(XPLMDataRef r) {
        if (!r) return 0;
        const XPLMDataTypeID ty = XPLMGetDataRefTypes(r);
        if (ty & xplmType_Int)    return XPLMGetDatai(r);
        if (ty & xplmType_Float)  return static_cast<int>(std::lround(XPLMGetDataf(r)));
        if (ty & xplmType_Double) return static_cast<int>(std::lround(XPLMGetDatad(r)));
        return XPLMGetDatai(r);
    }
    
    static inline bool dr_is_on(XPLMDataRef r) {
        if (!r) return false;
        const XPLMDataTypeID ty = XPLMGetDataRefTypes(r);
        if (ty & xplmType_Int)    return XPLMGetDatai(r) != 0;
        if (ty & xplmType_Float)  return XPLMGetDataf(r) > 0.5f;
        if (ty & xplmType_Double) return XPLMGetDatad(r) > 0.5;
        return XPLMGetDatai(r) != 0;
    }
    
    static inline bool bit(const uint8_t* r, int len, uint8_t off, uint8_t mask) {
        return (r && len > 0 && off < (uint8_t)len) && ((r[off] & mask) != 0);
    }

private:
    static constexpr int FD_ON_POS  = 1;
    static constexpr int FD_OFF_POS = 0;
    static constexpr int AT_ON_POS  = 1;
    static constexpr int AT_OFF_POS = 0;
    static constexpr int AP_ON_POS  = 1;
    static constexpr int AP_OFF_POS = 0;

    // ---- State ----
    State         _state{};
    StateCallback _cb;
    bool          _running{false};

    // ---- Intent ----
    Intent        _intent{};
    bool          _haveIntent{false};
    bool          _pendingIntentApply{false};

    // ---- Datarefs presence check ----
    XPLMDataRef _drFF777Check{nullptr}; // Check for FF 777

    // --- MCP numeric values ---
    XPLMDataRef _drSpd{nullptr};        // 1-sim/output/mcp/spd
    XPLMDataRef _drHdg{nullptr};        // 1-sim/output/mcp/hdg
    XPLMDataRef _drAlt{nullptr};        // 1-sim/output/mcp/alt
    XPLMDataRef _drVvi{nullptr};        // 1-sim/output/mcp/vs
    
    // --- MCP visibility/state flags ---
    XPLMDataRef _drIsMach{nullptr};     // 1-sim/output/mcp/isMachTrg
    XPLMDataRef _drSpdOpen{nullptr};    // 1-sim/output/mcp/isSpdOpen
    XPLMDataRef _drHdgTrg{nullptr};     // 1-sim/output/mcp/isHdgTrg
    XPLMDataRef _drVsOpen{nullptr};     // 1-sim/output/mcp/isVsOpen
    XPLMDataRef _drVsTrg{nullptr};      // 1-sim/output/mcp/isVsTrg
    XPLMDataRef _drMcpOk{nullptr};      // 1-sim/output/mcp/ok
    
    XPLMDataRef _drCrsCapt{nullptr};    // Course captain
    XPLMDataRef _drCrsFo{nullptr};      // Course FO

    // Illumination / brightness
    XPLMDataRef _drMcpBrightnessArr{nullptr};
    XPLMDataRef _drCockpitLightsArr{nullptr};

    // --- LEDs annunciators ---
    XPLMDataRef _drLedCaptAP{nullptr};  // 1-sim/ckpt/lampsGlow/mcpCaptAP
    XPLMDataRef _drLedAt{nullptr};      // 1-sim/ckpt/lampsGlow/mcpAT
    XPLMDataRef _drLedLnav{nullptr};    // 1-sim/ckpt/lampsGlow/mcpLNAV
    XPLMDataRef _drLedVnav{nullptr};    // 1-sim/ckpt/lampsGlow/mcpVNAV
    XPLMDataRef _drLedFlch{nullptr};    // 1-sim/ckpt/lampsGlow/mcpFLCH
    XPLMDataRef _drLedVs{nullptr};      // 1-sim/ckpt/lampsGlow/mcpVS
    XPLMDataRef _drLedAltHold{nullptr}; // 1-sim/ckpt/lampsGlow/mcpAltHOLD
    XPLMDataRef _drLedApp{nullptr};     // 1-sim/ckpt/lampsGlow/mcpAPP
    XPLMDataRef _drLedLoc{nullptr};     // 1-sim/ckpt/lampsGlow/mcpLOC

    // --- Maintained switch positions (0/1) ---
    XPLMDataRef _drFDLeftPos{nullptr};   // 1-sim/ckpt/mcpFdLSwitch/anim
    XPLMDataRef _drFDRightPos{nullptr};  // 1-sim/ckpt/mcpFdRSwitch/anim
    XPLMDataRef _drATArmPosLeft{nullptr};  // 1-sim/ckpt/mcpAtSwitchL/anim
    XPLMDataRef _drATArmPos{nullptr};    // 1-sim/ckpt/mcpAtSwitchR/anim
    XPLMDataRef _drApDiscPos{nullptr};   // 1-sim/ckpt/mcpApDiscSwitch/anim

    // --- Toggle commands ---
    XPLMCommandRef _cmdFDLeftToggle{nullptr};   // 1-sim/command/mcpFdLSwitch_trigger
    XPLMCommandRef _cmdFDRightToggle{nullptr};  // 1-sim/command/mcpFdRSwitch_trigger
    XPLMCommandRef _cmdATArmToggleLeft{nullptr};  // 1-sim/command/mcpAtSwitchL_trigger
    XPLMCommandRef _cmdATArmToggle{nullptr};    // 1-sim/command/mcpAtSwitchR_trigger
    XPLMCommandRef _cmdApDiscToggle{nullptr};   // 1-sim/command/mcpApDiscSwitch_trigger

    // --- Power ---
    XPLMDataRef _drHasApPower{nullptr};
    XPLMDataRef _drDcBus1{nullptr};
    XPLMDataRef _drDcBus2{nullptr};

    // --- Debounce & guards ---
    float _lastAtToggleTime{0.0f};
    float _lastApDiscToggleTime{0.0f};
    float _guardFdLeftTs{0.0f};
    float _guardFdRightTs{0.0f};
    float _guardAtLeftTs{0.0f};
    float _guardAtTs{0.0f};
    float _guardApDiscTs{0.0f};
    int   _lastFdLeftCmdPos{-1};
    int   _lastFdRightCmdPos{-1};
    int   _lastAtLeftCmdPos{-1};
    int   _lastAtCmdPos{-1};
    int   _lastApDiscCmdPos{-1};

    // Momentary buttons mapping (offset/mask -> cmd)
    struct BtnBinding {
        uint8_t       off;
        uint8_t       mask;
        XPLMCommandRef press{nullptr};
        XPLMCommandRef release{nullptr}; // optional
    };
    std::vector<BtnBinding> _btns;

    // Encoders mapping
    struct EncBinding {
        uint8_t       posOff;
        XPLMCommandRef inc{nullptr};
        XPLMCommandRef dec{nullptr};
        int           stepPerTick{1};
    };
    std::vector<EncBinding> _encs;
};

} // namespace pap3::aircraft

