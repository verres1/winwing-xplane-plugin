#include "rotatemd11_profile.h"
#include "../../device/pap3_device.h"

#include "config.h"

#include <algorithm>
#include <cmath>

#include <XPLMProcessing.h>
#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>

namespace pap3::aircraft {

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static inline XPLMCommandRef Cmd(const char* s) { return XPLMFindCommand(s); }
static inline XPLMDataRef DR(const char* s) { return XPLMFindDataRef(s); }

// -----------------------------------------------------------------------------
// Ctor / Dtor
// -----------------------------------------------------------------------------
RotateMD11PAP3Profile::RotateMD11PAP3Profile() {
    
    // Check for Rotate MD-11 presence
    _drRotateCheck = DR("Rotate/aircraft/systems/gcp_alt_presel_ft");
    
    // MCP values - Rotate MD-11 uses "Rotate/aircraft/systems/gcp_*"
    _drAlt     = DR("Rotate/aircraft/systems/gcp_alt_presel_ft");
    _drSpd     = DR("Rotate/aircraft/systems/gcp_spd_presel_ias");
    _drSpdMach = DR("Rotate/aircraft/systems/gcp_spd_presel_mach");
    _drHdg     = DR("Rotate/aircraft/systems/gcp_hdg_presel_deg");
    _drVvi     = DR("Rotate/aircraft/systems/gcp_vs_sel_fpm");
    _drIasMach = DR("Rotate/aircraft/systems/gcp_active_ias_mach_mode");
    
    // Course - using standard X-Plane datarefs as fallback
    _drCrsCapt = DR("sim/cockpit/radios/nav1_obs_degm");
    _drCrsFo   = DR("sim/cockpit/radios/nav2_obs_degm");
    
    // Brightness - use FGS panel brightness for button backlight
    _drMcpBrightnessArr = DR("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
    _drCockpitLights    = DR("Rotate/aircraft/systems/light_fgs_panel_brt_ratio");

    // Mode/LED datarefs
    _drRollMode       = DR("Rotate/aircraft/systems/afs_roll_mode");
    _drApEngaged      = DR("Rotate/aircraft/systems/afs_ap_engaged");
    _drApprEngaged    = DR("Rotate/aircraft/systems/afs_appr_engaged");
    _drAtEngaged      = DR("Rotate/aircraft/systems/afs_at_engaged");
    _drProfEngaged    = DR("Rotate/aircraft/systems/afs_prof_engaged");
    _drFmsSpdEngaged  = DR("Rotate/aircraft/systems/afs_fms_spd_engaged");
    _drHdgTrkSel      = DR("Rotate/aircraft/systems/gcp_hdg_trk_sel_set");

    // Control datarefs (for momentary buttons)
    _drCtrlNav          = DR("Rotate/aircraft/controls/fgs_nav");
    _drCtrlFmsSpd       = DR("Rotate/aircraft/controls/fgs_fms_spd");
    _drCtrlHdgModeSel = DR("Rotate/aircraft/controls/fgs_hdg_mode_sel");
    _drCtrlAltModeSel = DR("Rotate/aircraft/controls/fgs_alt_mode_sel");
    _drCtrlIasMach      = DR("Rotate/aircraft/controls/fgs_ias_mach");
    _drCtrlProf         = DR("Rotate/aircraft/controls/fgs_prof");
    _drCtrlSpdSelMode   = DR("Rotate/aircraft/controls/fgs_spd_sel_mode");
    _drCtrlApprLand     = DR("Rotate/aircraft/controls/fgs_appr_land");
    _drCtrlAutoflight   = DR("Rotate/aircraft/controls/fgs_autoflight");
    _drCtrlAfsOvrd1     = DR("Rotate/aircraft/controls/fgs_afs_ovrd_off_1");
    _drCtrlAfsOvrd2     = DR("Rotate/aircraft/controls/fgs_afs_ovrd_off_2");
    _drCtrlVsFpa        = DR("Rotate/aircraft/controls/fgs_vs_fpa");

    // Commands
    _cmdSpdSelUp   = Cmd("Rotate/aircraft/controls_c/fgs_spd_sel_up");
    _cmdSpdSelDn   = Cmd("Rotate/aircraft/controls_c/fgs_spd_sel_dn");
    _cmdHdgSelUp   = Cmd("Rotate/aircraft/controls_c/fgs_hdg_sel_up");
    _cmdHdgSelDn   = Cmd("Rotate/aircraft/controls_c/fgs_hdg_sel_dn");
    _cmdAltSelUp   = Cmd("Rotate/aircraft/controls_c/fgs_alt_sel_up");
    _cmdAltSelDn   = Cmd("Rotate/aircraft/controls_c/fgs_alt_sel_dn");
    _cmdPitchSelUp = Cmd("Rotate/aircraft/controls_c/fgs_pitch_sel_up");
    _cmdPitchSelDn = Cmd("Rotate/aircraft/controls_c/fgs_pitch_sel_dn");
    _cmdAutoflight = Cmd("Rotate/aircraft/controls_c/fgs_autoflight");
    _cmdFmsSpd     = Cmd("Rotate/aircraft/controls_c/fgs_fms_spd");
    _cmdAltModeSelUp = Cmd("Rotate/aircraft/controls_c/fgs_alt_mode_sel_up");
    _cmdAltModeSelDn = Cmd("Rotate/aircraft/controls_c/fgs_alt_mode_sel_dn");
    _cmdHdgModeSelDn = Cmd("Rotate/aircraft/controls_c/fgs_hdg_mode_sel_dn");
    _cmdVsFpa        = Cmd("Rotate/aircraft/controls_c/fgs_vs_fpa");

    // Power - MD-11 specific battery switches
    _drBattPowered = DR("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd");
    // Buttons using dataref writes (momentary: press=1, release=0)
    auto BD = [&](uint8_t off, uint8_t mask, XPLMDataRef dr, int pressVal=1, int releaseVal=0){
        _btnDataRefs.push_back({off, mask, dr, pressVal, releaseVal});
    };

    // 0x01 - Main autopilot mode buttons
    BD(0x01, 0x02, _drCtrlFmsSpd, 1, 0);                                // SPEED
    BD(0x01, 0x04, _drCtrlProf, 1, 0);                                  // VNAV (PROF in MD-11)
    BD(0x01, 0x08, _drCtrlAltModeSel, -1, 0);                          // LVL CHG
    BD(0x01, 0x10, _drCtrlHdgModeSel, -1, 0);                          // HDG SEL
    BD(0x01, 0x20, _drCtrlNav, 1, 0);                                   // LNAV (NAV in MD-11)
    BD(0x01, 0x80, _drCtrlApprLand, 1, 0);                              // APP (APPR/LAND)

    // 0x02 - Additional autopilot buttons
    BD(0x02, 0x01, _drCtrlAltModeSel, 1, 0);                          // ALT HOLD
    BD(0x02, 0x02, _drCtrlVsFpa, 1, 0);                                 // V/S
    BD(0x02, 0x04, _drCtrlAutoflight, 1, 0);                            // CMD A (AP engage)
    BD(0x02, 0x08, _drCtrlAfsOvrd1, 1, 0);                              // CWS A (AFS override 1)
    BD(0x02, 0x10, _drCtrlAutoflight, 1, 0);                            // CMD B (AP engage - same as A)
    BD(0x02, 0x20, _drCtrlAfsOvrd2, 1, 0);                              // CWS B (AFS override 2)
    BD(0x02, 0x80, _drCtrlSpdSelMode, -1, 0);                           // SPD INTERV (SPD SEL)

    // 0x03
    BD(0x03, 0x01, _drCtrlAltModeSel, -1, 0);                          // ALT INTERV

    // C/O button - mapped to IAS/MACH toggle (offset 0x02, bit 0x40)
    BD(0x02, 0x40, _drCtrlIasMach, 1, 0);         // C/O (IAS/MACH toggle)

    // Encoders - mapping PAP3 encoder positions to MD-11 commands
    auto E = [&](uint8_t posOff, XPLMCommandRef inc, XPLMCommandRef dec, int step=1){
        _encs.push_back({posOff, inc, dec, step});
    };
    
    E(0x15, Cmd("sim/autopilot/heading_up"), Cmd("sim/autopilot/heading_down"));  // CRS CAPT
    E(0x17, _cmdSpdSelUp, _cmdSpdSelDn);        // SPD
    E(0x19, _cmdHdgSelUp, _cmdHdgSelDn);        // HDG
    E(0x1B, _cmdAltSelUp, _cmdAltSelDn);        // ALT
    E(0x1D, _cmdPitchSelUp, _cmdPitchSelDn);    // V/S (pitch in MD-11)
    E(0x1F, Cmd("sim/autopilot/heading_up"), Cmd("sim/autopilot/heading_down"));  // CRS FO
    
    // Initialize state - HDG display should be visible on startup for MD-11
    _state.hdgVisible = true;
}

RotateMD11PAP3Profile::~RotateMD11PAP3Profile() { 
    stop(); 
}

// -----------------------------------------------------------------------------
// Interface
// -----------------------------------------------------------------------------
bool RotateMD11PAP3Profile::isEligible() const { 
    bool eligible = (_drRotateCheck != nullptr);
    return eligible; 
}

void RotateMD11PAP3Profile::start(StateCallback onChanged) {
    if (_running) {
        return;
    }
    _cb = std::move(onChanged);
    _running = true;
    _backlightInitialized = false;  // Reset backlight initialization flag
    _hdgTrkSelInitialized = false;  // Reset HDG/TRK selector initialization flag
}

void RotateMD11PAP3Profile::stop() {
    if (!_running) {
        return;
    }
    _running = false;
}

pap3::aircraft::State RotateMD11PAP3Profile::current() const { return _state; }

void RotateMD11PAP3Profile::tick() {
    if (!_running) return;
    poll(); // Read sim -> State -> callback device
}

// -----------------------------------------------------------------------------
// Poll sim -> state
// -----------------------------------------------------------------------------
void RotateMD11PAP3Profile::poll() {
    // Speed - check if IAS or MACH mode
    if (_drIasMach) {
        const int mode = dr_get_int(_drIasMach);
        if (mode == 0 && _drSpd) {
            // IAS mode
            _state.spd = dr_get_float(_drSpd);
            _state.spdVisible = true;
        } else if (mode == 1 && _drSpdMach) {
            // MACH mode
            _state.spd = dr_get_float(_drSpdMach);
            _state.spdVisible = true;
        }
    } else if (_drSpd) {
        _state.spd = dr_get_float(_drSpd);
        _state.spdVisible = true;
    }
    
    // Control SPD LCD visibility based on FMS SPD engagement
    // When FMS SPD is engaged (value = 1), hide the SPD display
    // When FMS SPD is not engaged (value = 0), show the SPD display
    if (_drFmsSpdEngaged) {
        const int fmsSpdEngaged = dr_get_int(_drFmsSpdEngaged);
        if (fmsSpdEngaged == 1) {
            _state.spdVisible = false;  // FMS controls speed, hide LCD
        }
        // If fmsSpdEngaged == 0, keep current spdVisible state (already set above)
    }
    
    if (_drHdg) {
        int hdg_raw = dr_get_int(_drHdg);
        // Wrap heading to 0-359 range (handle values > 360)
        _state.hdg = hdg_raw % 360;
        if (_state.hdg < 0) _state.hdg += 360;  // Handle negative values
    }
    
    // Control HDG LCD visibility based on HDG/TRK selector
    // Workaround for aircraft bug: first value is 0 but should be 1 on startup
    if (_drHdgTrkSel) {
        const int hdgTrkSel = dr_get_int(_drHdgTrkSel);
        
        if (!_hdgTrkSelInitialized) {
            // Not yet initialized - handle first values
            if (hdgTrkSel == 0) {
                // First value is 0 (bug) - treat as 1
                _state.hdgVisible = true;
            } else if (hdgTrkSel == 1) {
                // Received proper value of 1 - mark as initialized and use it
                _hdgTrkSelInitialized = true;
                _state.hdgVisible = true;
            }
        } else {
            // Already initialized - use value directly
            _state.hdgVisible = (hdgTrkSel == 1);  // Active when HDG mode (1), inactive when TRK mode (0)
        }
    } else {
        _state.hdgVisible = true;  // Default to visible if dataref not available
    }
    if (_drAlt) {
        _state.alt = dr_get_int(_drAlt);
    }
    if (_drVvi) {
        _state.vvi = dr_get_float(_drVvi);
        // V/S display: only show when V/S is non-zero
        _state.vviVisible = (std::abs(_state.vvi) > 0.5f);  // >0.5 fpm threshold
    }
    
    // MD-11 doesn't have CRS displays on the MCP - disable them
    _state.crsVisible = false;
    if (_drCrsCapt) {
        int crs_raw = dr_get_int(_drCrsCapt);
        _state.crsCapt = crs_raw % 360;
        if (_state.crsCapt < 0) _state.crsCapt += 360;
    }
    if (_drCrsFo) {
        int crs_raw = dr_get_int(_drCrsFo);
        _state.crsFo = crs_raw % 360;
        if (_state.crsFo < 0) _state.crsFo += 360;
    }
    
    // A/T arm status - MD-11 uses engagement status
    if (_drAtEngaged) {
        _state.atArmOn = dr_is_on(_drAtEngaged);
    }

    // MD-11 brightness configuration:
    // - LCD backlight: 100% when powered, 0% when not powered
    // - Button backlight: 0% at startup, then adjustable via FGS panel lights when powered, 0% when not powered
    // Power check: Battery switch OR ground power
    bool hasPower = dr_is_on(_drBattPowered);
    
    if (hasPower) {
        // MCP has power - LCD at full brightness, buttons adjustable
        _state.mcpBrightness = 1.0f;  // LCD always at full brightness when powered
        
        if (_drCockpitLights) {
            float panelLights = dr_get_float(_drCockpitLights);
            _state.cockpitLights = std::clamp(panelLights, 0.0f, 1.0f);
            
            // Set button backlight: 0 at startup, then follow panel lights
            if (!_backlightInitialized) {
                _state.ledsBrightness = 0.0f;  // Button backlight off at startup
                _backlightInitialized = true;
            } else {
                _state.ledsBrightness = panelLights;  // Button backlight adjustable via panel lights
            }
        } else {
            _state.cockpitLights = 1.0f;
            _state.ledsBrightness = _backlightInitialized ? 1.0f : 0.0f;
            _backlightInitialized = true;
        }
    } else {
        // MCP has no power - turn off all displays and lights
        _state.mcpBrightness = 0.0f;   // LCD off
        _state.cockpitLights = 0.0f;
        _state.ledsBrightness = 0.0f;  // Buttons off
    }

    // MD-11 doesn't have special digit flags
    _state.digitA = false;
    _state.digitB = false;

    // MD-11 MCP has NO LED annunciators - all LEDs must be disabled
    // The real MD-11 uses a different display system (not LED buttons)
    _state.led.N1      = false;
    _state.led.SPEED   = false;
    _state.led.VNAV    = false;
    _state.led.LVL_CHG = false;
    _state.led.HDG_SEL = false;
    _state.led.LNAV    = false;
    _state.led.VORLOC  = false;
    _state.led.APP     = false;
    _state.led.ALT_HLD = false;
    _state.led.V_S     = false;
    _state.led.CMD_A   = false;
    _state.led.CWS_A   = false;
    _state.led.CMD_B   = false;
    _state.led.CWS_B   = false;
    _state.led.AT_ARM  = false;
    _state.led.MA_CAPT = false;
    _state.led.MA_FO   = false;
    

    if (_cb) _cb(_state);
}

// -----------------------------------------------------------------------------
// Command helpers
// -----------------------------------------------------------------------------
void RotateMD11PAP3Profile::execOnce(XPLMCommandRef cmd) { 
    if (cmd) {
        XPLMCommandOnce(cmd);
    } else {
    }
}

void RotateMD11PAP3Profile::repeatCmd(XPLMCommandRef inc, XPLMCommandRef dec, int8_t delta, int stepPerTick) {
    if (delta == 0) return;
    const bool up  = (delta > 0);
    const int  reps = std::max(1, static_cast<int>(std::abs(static_cast<int>(delta))) * std::max(1, stepPerTick));
    XPLMCommandRef cmd = up ? inc : dec;
    if (!cmd) {
        return;
    }
    for (int i = 0; i < reps; ++i) XPLMCommandOnce(cmd);
}

void RotateMD11PAP3Profile::writeDataRef(XPLMDataRef dr, int value) {
    if (!dr) {
        return;
    }
    const XPLMDataTypeID ty = XPLMGetDataRefTypes(dr);
    if (ty & xplmType_Int) {
        XPLMSetDatai(dr, value);
    } else if (ty & xplmType_Float) {
        XPLMSetDataf(dr, static_cast<float>(value));
    } else if (ty & xplmType_Double) {
        XPLMSetDatad(dr, static_cast<double>(value));
    }
}

bool RotateMD11PAP3Profile::debounce(float& lastTs, float minDeltaSec) const {
    const float now = XPLMGetElapsedTime();
    if (now - lastTs < minDeltaSec) return false;
    lastTs = now;
    return true;
}

// -----------------------------------------------------------------------------
// Input hooks
// -----------------------------------------------------------------------------
void RotateMD11PAP3Profile::onButton(std::uint8_t off, std::uint8_t mask, bool pressed) {
    
    // MD-11 uses dataref writes for all buttons (momentary: press=1, release=0)
    for (const auto& b : _btnDataRefs) {
        if (b.off == off && b.mask == mask) {
            if (b.dataref) {
                writeDataRef(b.dataref, pressed ? b.pressValue : b.releaseValue);
            }
            return;
        }
    }
    
}

void RotateMD11PAP3Profile::onEncoderDelta(std::uint8_t posOff, std::int8_t delta) {
    
    for (const auto& e : _encs) {
        if (e.posOff == posOff) {
            repeatCmd(e.inc, e.dec, delta, e.stepPerTick);
            return;
        }
    }
    
}

// -----------------------------------------------------------------------------
// Power gating
// -----------------------------------------------------------------------------
uint8_t RotateMD11PAP3Profile::mcpPowerMask() const {
    uint8_t mask = 0;
    // MCP has power if battery switch is on OR ground power is connected
    bool battSwOn = dr_is_on(_drBattPowered);
    bool grdPwrOn = dr_is_on(_drBatteryGrd);
    bool hasPower = battSwOn || grdPwrOn;
    
    if (hasPower) mask |= 0x01; // MCP has power
    return mask;
}

bool RotateMD11PAP3Profile::mcpHasPower() const { 
    bool hasPower = (mcpPowerMask() & 0x01) != 0;
    return hasPower; 
}

// -----------------------------------------------------------------------------
// Boot syncs
// -----------------------------------------------------------------------------
void RotateMD11PAP3Profile::syncSimToHardware() {
    // MD-11 doesn't have maintained switches like F/D or A/T ARM
    // Nothing to sync from hardware to sim
}

void RotateMD11PAP3Profile::syncSimToHardwareFromRaw(const std::uint8_t* r, int len) {
    // MD-11 doesn't have maintained switches
    // Nothing to sync
}

} // namespace pap3::aircraft

