#pragma once
#include "../pap3_aircraft.h"
#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>
#include <vector>
#include <cstdint>
#include <cmath>

namespace pap3::aircraft {

class RotateMD11PAP3Profile final : public PAP3AircraftProfile {
public:
    using State = pap3::aircraft::State;

    RotateMD11PAP3Profile();
    ~RotateMD11PAP3Profile() override;

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

    // Input hooks
    void onButton(std::uint8_t off, std::uint8_t mask, bool pressed) override;
    void onEncoderDelta(std::uint8_t posOff, std::int8_t delta) override;

private:
    // --- Polling: sim -> State
    void  poll();

    // --- Command helpers
    void  execOnce(XPLMCommandRef cmd);
    void  repeatCmd(XPLMCommandRef inc, XPLMCommandRef dec, int8_t delta, int stepPerTick = 1);
    void  writeDataRef(XPLMDataRef dr, int value);

    // --- Debounce
    bool  debounce(float& lastTs, float minDeltaSec = 0.05f) const;

    static inline int  dr_get_int(XPLMDataRef r) {
        if (!r) return 0;
        const XPLMDataTypeID ty = XPLMGetDataRefTypes(r);
        if (ty & xplmType_Int)    return XPLMGetDatai(r);
        if (ty & xplmType_Float)  return static_cast<int>(std::lround(XPLMGetDataf(r)));
        if (ty & xplmType_Double) return static_cast<int>(std::lround(XPLMGetDatad(r)));
        return XPLMGetDatai(r);
    }
    
    static inline float dr_get_float(XPLMDataRef r) {
        if (!r) return 0.0f;
        const XPLMDataTypeID ty = XPLMGetDataRefTypes(r);
        if (ty & xplmType_Float)  return XPLMGetDataf(r);
        if (ty & xplmType_Double) return static_cast<float>(XPLMGetDatad(r));
        if (ty & xplmType_Int)    return static_cast<float>(XPLMGetDatai(r));
        return XPLMGetDataf(r);
    }
    
    static inline bool dr_is_on(XPLMDataRef r) {
        if (!r) return false;
        const XPLMDataTypeID ty = XPLMGetDataRefTypes(r);
        if (ty & xplmType_Int)    return XPLMGetDatai(r) != 0;
        if (ty & xplmType_Float)  return XPLMGetDataf(r) > 0.5f;
        if (ty & xplmType_Double) return XPLMGetDatad(r) > 0.5;
        return XPLMGetDatai(r) != 0;
    }

private:
    // ---- State ----
    State         _state{};
    StateCallback _cb;
    bool          _running{false};

    // ---- Datarefs presence check ----
    XPLMDataRef _drRotateCheck{nullptr}; // Check for Rotate MD-11

    // --- MCP numeric values ---
    XPLMDataRef _drSpd{nullptr};        // Rotate/aircraft/systems/gcp_spd_presel_ias
    XPLMDataRef _drSpdMach{nullptr};    // Rotate/aircraft/systems/gcp_spd_presel_mach
    XPLMDataRef _drHdg{nullptr};        // Rotate/aircraft/systems/instr_hdg_trk_presel_bug_deg[1]
    XPLMDataRef _drAlt{nullptr};        // Rotate/aircraft/systems/gcp_alt_presel_ft
    XPLMDataRef _drVvi{nullptr};        // Rotate/aircraft/systems/gcp_vs_sel_fpm
    XPLMDataRef _drIasMach{nullptr};    // Rotate/aircraft/systems/gcp_active_ias_mach_mode
    XPLMDataRef _drCrsCapt{nullptr};    // Course captain
    XPLMDataRef _drCrsFo{nullptr};      // Course FO

    // Illumination / brightness
    XPLMDataRef _drMcpBrightnessArr{nullptr};
    XPLMDataRef _drCockpitLights{nullptr};  // Rotate/aircraft/controls/instr_panel_lts

    // --- Mode/LED datarefs ---
    XPLMDataRef _drRollMode{nullptr};      // Rotate/aircraft/systems/afs_roll_mode (LNAV=4, HDG=other)
    XPLMDataRef _drApEngaged{nullptr};     // Rotate/aircraft/systems/afs_ap_engaged
    XPLMDataRef _drApprEngaged{nullptr};   // Rotate/aircraft/systems/afs_appr_engaged
    XPLMDataRef _drAtEngaged{nullptr};     // Rotate/aircraft/systems/afs_at_engaged
    XPLMDataRef _drProfEngaged{nullptr};   // Rotate/aircraft/systems/afs_prof_engaged
    XPLMDataRef _drFmsSpdEngaged{nullptr}; // Rotate/aircraft/systems/afs_fms_spd_engaged
    XPLMDataRef _drHdgTrkSel{nullptr};     // Rotate/aircraft/systems/gcp_hdg_trk_sel_set

    // --- Control datarefs (for momentary buttons) ---
    XPLMDataRef _drCtrlNav{nullptr};         // Rotate/aircraft/controls/fgs_nav
    XPLMDataRef _drCtrlFmsSpd{nullptr};      // Rotate/aircraft/controls/fgs_fms_spd
    XPLMDataRef _drCtrlHdgModeSel{nullptr}; // Rotate/aircraft/controls/fgs_hdg_mode_sel_dn
    XPLMDataRef _drCtrlAltModeSel{nullptr}; // Rotate/aircraft/controls/fgs_alt_mode_sel_up
    XPLMDataRef _drCtrlIasMach{nullptr};     // Rotate/aircraft/controls/fgs_ias_mach
    XPLMDataRef _drCtrlProf{nullptr};        // Rotate/aircraft/controls/fgs_prof
    XPLMDataRef _drCtrlSpdSelMode{nullptr};  // Rotate/aircraft/controls/fgs_spd_sel_mode
    XPLMDataRef _drCtrlApprLand{nullptr};    // Rotate/aircraft/controls/fgs_appr_land
    XPLMDataRef _drCtrlAutoflight{nullptr};  // Rotate/aircraft/controls/fgs_autoflight
    XPLMDataRef _drCtrlAfsOvrd1{nullptr};    // Rotate/aircraft/controls/fgs_afs_ovrd_off_1
    XPLMDataRef _drCtrlAfsOvrd2{nullptr};    // Rotate/aircraft/controls/fgs_afs_ovrd_off_2
    XPLMDataRef _drCtrlVsFpa{nullptr};       // Rotate/aircraft/controls/fgs_vs_fpa

    // --- Commands ---
    XPLMCommandRef _cmdSpdSelUp{nullptr};    // Rotate/aircraft/controls_c/fgs_spd_sel_up
    XPLMCommandRef _cmdSpdSelDn{nullptr};    // Rotate/aircraft/controls_c/fgs_spd_sel_dn
    XPLMCommandRef _cmdHdgSelUp{nullptr};    // Rotate/aircraft/controls_c/fgs_hdg_sel_up
    XPLMCommandRef _cmdHdgSelDn{nullptr};    // Rotate/aircraft/controls_c/fgs_hdg_sel_dn
    XPLMCommandRef _cmdAltSelUp{nullptr};    // Rotate/aircraft/controls_c/fgs_alt_sel_up
    XPLMCommandRef _cmdAltSelDn{nullptr};    // Rotate/aircraft/controls_c/fgs_alt_sel_dn
    XPLMCommandRef _cmdPitchSelUp{nullptr};  // Rotate/aircraft/controls_c/fgs_pitch_sel_up
    XPLMCommandRef _cmdPitchSelDn{nullptr};  // Rotate/aircraft/controls_c/fgs_pitch_sel_dn
    XPLMCommandRef _cmdAutoflight{nullptr};  // Rotate/aircraft/controls_c/fgs_autoflight
    XPLMCommandRef _cmdFmsSpd{nullptr};      // Rotate/aircraft/controls_c/fgs_fms_spd
    XPLMCommandRef _cmdAltModeSelUp{nullptr};   // Rotate/aircraft/controls_c/fgs_alt_mode_sel_up
    XPLMCommandRef _cmdAltModeSelDn{nullptr};   // Rotate/aircraft/controls_c/fgs_alt_mode_sel_dn
    XPLMCommandRef _cmdHdgModeSelDn{nullptr};   // Rotate/aircraft/controls_c/fgs_hdg_mode_sel_dn
    XPLMCommandRef _cmdVsFpa{nullptr};          // Rotate/aircraft/controls_c/fgs_vs_fpa

    // --- Power - MD-11 specific battery switches ---
    XPLMDataRef _drBattPowered{nullptr};  // Rotate/aircraft/systems/batt_sw_action
    XPLMDataRef _drBatteryGrd{nullptr};    // Rotate/aircraft/controls/battery_grd

    // --- Debounce & guards ---
    float _lastButtonTime{0.0f};
    bool  _backlightInitialized{false};  // Track if backlight has been set after startup
    bool  _hdgTrkSelInitialized{false};  // Workaround: track if HDG/TRK selector has been properly initialized

    // Momentary buttons mapping (offset/mask -> dataref to write)
    struct BtnDataRefBinding {
        uint8_t     off;
        uint8_t     mask;
        XPLMDataRef dataref{nullptr};
        int         pressValue{1};
        int         releaseValue{0};
    };
    std::vector<BtnDataRefBinding> _btnDataRefs;

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

