#include "ff777_profile.h"
#include "../../device/pap3_device.h"

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
static inline bool bit(const std::uint8_t* r, int len, std::uint8_t off, std::uint8_t mask) {
    return (r && len > 0 && off < (std::uint8_t)len) && ((r[off] & mask) != 0);
}

// -----------------------------------------------------------------------------
// Ctor / Dtor
// -----------------------------------------------------------------------------
FF777PAP3Profile::FF777PAP3Profile() {
    // Check for FlightFactor 777 presence - using a known FF777 dataref
    _drFF777Check = XPLMFindDataRef("1-sim/output/mcp/spd");
    
    // Course - using standard X-Plane datarefs (always available)
    _drCrsCapt = XPLMFindDataRef("sim/cockpit/radios/nav1_obs_degm");
    _drCrsFo   = XPLMFindDataRef("sim/cockpit/radios/nav2_obs_degm");

    // Power - using standard X-Plane datarefs (always available)
    _drHasApPower = XPLMFindDataRef("sim/cockpit2/autopilot/autopilot_has_power");
    _drDcBus1     = XPLMFindDataRef("sim/cockpit2/electrical/bus_volts");
    _drDcBus2     = XPLMFindDataRef("sim/cockpit2/electrical/bus_volts");

    // Note: All FF777-specific datarefs, commands, buttons and encoders 
    // will be initialized lazily in initializeCommandsAndEncoders()
    // when FF777 becomes fully available
}

// -----------------------------------------------------------------------------
// Lazy initialization - called when FF777 commands become available
// -----------------------------------------------------------------------------
void FF777PAP3Profile::initializeCommandsAndEncoders() {
    _btns.clear();
    _encs.clear();
    
    // MCP values - FlightFactor 777 uses "1-sim" prefix
    _drSpd     = XPLMFindDataRef("1-sim/output/mcp/spd");
    _drHdg     = XPLMFindDataRef("1-sim/output/mcp/hdg");
    _drAlt     = XPLMFindDataRef("1-sim/output/mcp/alt");
    _drVvi     = XPLMFindDataRef("1-sim/output/mcp/vs");
    
    // MCP visibility/state flags
    _drIsMach  = XPLMFindDataRef("1-sim/output/mcp/isMachTrg");
    _drSpdOpen = XPLMFindDataRef("1-sim/output/mcp/isSpdOpen");
    _drHdgTrg  = XPLMFindDataRef("1-sim/output/mcp/isHdgTrg");
    _drVsOpen  = XPLMFindDataRef("1-sim/output/mcp/isVsOpen");
    _drVsTrg   = XPLMFindDataRef("1-sim/output/mcp/isVsTrg");
    _drMcpOk   = XPLMFindDataRef("1-sim/output/mcp/ok");
    
    // Brightness - using FF777 glareshield light
    _drMcpBrightnessArr = XPLMFindDataRef("1-sim/ckpt/lights/glareshield");
    _drCockpitLightsArr = XPLMFindDataRef("1-sim/ckpt/lights/glareshield");

    // LEDs - FlightFactor 777 lamp glow datarefs
    _drLedCaptAP  = XPLMFindDataRef("1-sim/ckpt/lampsGlow/mcpCaptAP");
    _drLedAt      = XPLMFindDataRef("1-sim/ckpt/lampsGlow/mcpAT");
    _drLedLnav    = XPLMFindDataRef("1-sim/ckpt/lampsGlow/mcpLNAV");
    _drLedVnav    = XPLMFindDataRef("1-sim/ckpt/lampsGlow/mcpVNAV");
    _drLedFlch    = XPLMFindDataRef("1-sim/ckpt/lampsGlow/mcpFLCH");
    _drLedVs      = XPLMFindDataRef("1-sim/ckpt/lampsGlow/mcpVS");
    _drLedAltHold = XPLMFindDataRef("1-sim/ckpt/lampsGlow/mcpAltHOLD");
    _drLedApp     = XPLMFindDataRef("1-sim/ckpt/lampsGlow/mcpAPP");
    _drLedLoc     = XPLMFindDataRef("1-sim/ckpt/lampsGlow/mcpLOC");

    // Maintained positions (0/1) - switch animation datarefs
    _drFDLeftPos    = XPLMFindDataRef("1-sim/ckpt/mcpFdLSwitch/anim");
    _drFDRightPos   = XPLMFindDataRef("1-sim/ckpt/mcpFdRSwitch/anim");
    _drATArmPosLeft = XPLMFindDataRef("1-sim/ckpt/mcpAtSwitchL/anim");
    _drATArmPos     = XPLMFindDataRef("1-sim/ckpt/mcpAtSwitchR/anim");
    _drApDiscPos    = XPLMFindDataRef("1-sim/ckpt/mcpApDiscSwitch/anim");

    // Toggle commands
    _cmdFDLeftToggle    = Cmd("1-sim/command/mcpFdLSwitch_trigger");
    _cmdFDRightToggle   = Cmd("1-sim/command/mcpFdRSwitch_trigger");
    _cmdATArmToggleLeft = Cmd("1-sim/command/mcpAtSwitchL_trigger");
    _cmdATArmToggle     = Cmd("1-sim/command/mcpAtSwitchR_trigger");
    _cmdApDiscToggle    = Cmd("1-sim/command/mcpApDiscSwitch_trigger");

    // Buttons (momentary) - mapping PAP3 buttons to FF777 commands
    auto B = [&](uint8_t off, uint8_t mask, const char* press, const char* release=nullptr){
        _btns.push_back({off, mask, Cmd(press), release ? Cmd(release) : nullptr});
    };

    // 0x01 - Main autopilot mode buttons
    B(0x01, 0x01, "1-sim/command/mcpClbButton_button");      // CLB (THR REF) / N1 equivalent
    B(0x01, 0x02, "1-sim/command/mcpAtButton_button");       // SPEED
    B(0x01, 0x04, "1-sim/command/mcpVnavButton_button");     // VNAV
    B(0x01, 0x08, "1-sim/command/mcpFlchButton_button");     // FLCH (LVL CHG)
    B(0x01, 0x10, "1-sim/command/mcpHdgCelButton_button");   // HDG SEL
    B(0x01, 0x20, "1-sim/command/mcpLnavButton_button");     // LNAV
    B(0x01, 0x40, "1-sim/command/mcpLocButton_button");      // LOC (VORLOC)
    B(0x01, 0x80, "1-sim/command/mcpAppButton_button");      // APP

    // 0x02 - Additional autopilot buttons
    B(0x02, 0x01, "1-sim/command/mcpAltHoldButton_button");  // ALT HOLD
    B(0x02, 0x02, "1-sim/command/mcpVsButton_button");       // V/S
    B(0x02, 0x04, "1-sim/command/mcpApLButton_button");      // CMD A (AP engage left)
    // Note: FF777 doesn't have separate CWS buttons like Zibo, using AP engage commands
    B(0x02, 0x08, "1-sim/command/mcpApLButton_button");      // CWS A (map to CMD A)
    B(0x02, 0x10, "1-sim/command/mcpApRButton_button");      // CMD B (AP engage right)
    B(0x02, 0x20, "1-sim/command/mcpApRButton_button");      // CWS B (map to CMD B)
    B(0x02, 0x40, "1-sim/command/mcpIasMachButton_button");  // CHANGE OVER (IAS/MACH toggle)
    B(0x02, 0x80, "1-sim/command/mcpSpdRotary_push");        // SPD INTERV (SPD push)

    // 0x03
    B(0x03, 0x01, "1-sim/command/mcpAltRotary_push");        // ALT INTERV (ALT push)

    // Encoders - mapping PAP3 encoder positions to FF777 rotary commands
    auto E = [&](uint8_t posOff, const char* inc, const char* dec, int step=1){
        _encs.push_back({posOff, Cmd(inc), Cmd(dec), step});
    };
    
    // Note: 777 doesn't have course controls on MCP (they're on EFIS panel)
    // E(0x15, ...); // CRS CAPT - not available on 777 MCP
    E(0x17, "1-sim/command/mcpSpdRotary_rotary+", "1-sim/command/mcpSpdRotary_rotary-"); // SPD
    E(0x19, "1-sim/command/mcpHdgRotary_rotary+", "1-sim/command/mcpHdgRotary_rotary-"); // HDG
    E(0x1B, "1-sim/command/mcpAltRotary_rotary+", "1-sim/command/mcpAltRotary_rotary-"); // ALT
    E(0x1D, "1-sim/command/mcpVsRotary_rotary+",  "1-sim/command/mcpVsRotary_rotary-");  // V/S
    // E(0x1F, ...); // CRS FO - not available on 777 MCP
    
    _commandsInitialized = true;
}

FF777PAP3Profile::~FF777PAP3Profile() { stop(); }

// -----------------------------------------------------------------------------
// Interface
// -----------------------------------------------------------------------------
bool FF777PAP3Profile::isEligible() const { 
    return (_drFF777Check != nullptr);
}

void FF777PAP3Profile::start(StateCallback onChanged) {
    if (_running) return;
    _cb = std::move(onChanged);
    _running = true;
}

void FF777PAP3Profile::stop() {
    if (!_running) return;
    _running = false;
}

pap3::aircraft::State FF777PAP3Profile::current() const { return _state; }

void FF777PAP3Profile::tick() {
    if (!_running) return;
    
    // Lazy initialization: Wait for FF777 commands and datarefs to become available
    if (!_commandsInitialized) {
        // Test if FF777 commands and datarefs are now available
        XPLMCommandRef testCmd = XPLMFindCommand("1-sim/command/mcpSpdRotary_rotary+");
        XPLMDataRef testHdg = XPLMFindDataRef("1-sim/output/mcp/hdg");
        XPLMDataRef testAlt = XPLMFindDataRef("1-sim/output/mcp/alt");
        if (testCmd != nullptr && testHdg != nullptr && testAlt != nullptr) {
            initializeCommandsAndEncoders();
        }
    }
    
    if (_pendingIntentApply && _haveIntent) {
        applyIntent();

        const bool fdLeftMatch  = (dr_get_pos(_drFDLeftPos)  == (_intent.fd_left_on  ? FD_ON_POS : FD_OFF_POS));
        const bool fdRightMatch = (dr_get_pos(_drFDRightPos) == (_intent.fd_right_on ? FD_ON_POS : FD_OFF_POS));

        if (fdLeftMatch && fdRightMatch) {
            _pendingIntentApply = false;
        }
    }

    poll(); // Read sim -> State -> callback device
}

// -----------------------------------------------------------------------------
// Poll sim -> state
// -----------------------------------------------------------------------------
void FF777PAP3Profile::poll() {
    // Read MCP values with type-safe access
    if (_drSpd) _state.spd = XPLMGetDataf(_drSpd);
    if (_drSpdOpen) _state.spdVisible = (XPLMGetDatai(_drSpdOpen) != 0); // visible when non-zero (int)
    
    if (_drHdg) {
        XPLMDataTypeID hdgType = XPLMGetDataRefTypes(_drHdg);
        if (hdgType & xplmType_Int) {
            _state.hdg = XPLMGetDatai(_drHdg);
        } else if (hdgType & xplmType_Float) {
            _state.hdg = static_cast<int>(XPLMGetDataf(_drHdg));
        }
    }
    
    if (_drAlt) {
        XPLMDataTypeID altType = XPLMGetDataRefTypes(_drAlt);
        if (altType & xplmType_Int) {
            _state.alt = XPLMGetDatai(_drAlt);
        } else if (altType & xplmType_Float) {
            _state.alt = static_cast<int>(XPLMGetDataf(_drAlt));
        }
    }
    
    if (_drVvi) _state.vvi = XPLMGetDataf(_drVvi);
    if (_drVsOpen) _state.vviVisible = (XPLMGetDatai(_drVsOpen) != 0); // visible when non-zero (int)
    
    // Note: 777 doesn't have course on MCP, course is on EFIS panel - hide course displays
    _state.crsVisible = false;  // 777 doesn't show course on MCP
    if (_drCrsCapt) _state.crsCapt = XPLMGetDatai(_drCrsCapt);
    if (_drCrsFo)   _state.crsFo   = XPLMGetDatai(_drCrsFo);
    
    // 777 has both left and right A/T switches - consider armed if either is on
    if (_drATArmPosLeft && _drATArmPos) {
        const bool atLeftOn = (dr_get_pos(_drATArmPosLeft) == AT_ON_POS);
        const bool atRightOn = (dr_get_pos(_drATArmPos) == AT_ON_POS);
        _state.atArmOn = (atLeftOn || atRightOn);
    } else if (_drATArmPos) {
        _state.atArmOn = (dr_get_pos(_drATArmPos) == AT_ON_POS);
    }

    // Brightness from FF777 glareshield light (single float value, not array)
    if (_drMcpBrightnessArr) {
        float v = XPLMGetDataf(_drMcpBrightnessArr);
        _state.mcpBrightness = std::clamp(v, 0.0f, 1.0f);
        _state.cockpitLights = std::clamp(v, 0.0f, 1.0f);
        _state.ledsBrightness = std::max(v, 0.60f);
    }

    _state.digitA = false;  // FF777 doesn't have special digit flags
    _state.digitB = false;

    // Map FF777 LEDs to PAP3 state
    // FF777 has fewer LEDs than a full 737 MCP, so we map what's available
    _state.led.N1      = false; // Use CLB button status if available
    _state.led.SPEED   = false; // No direct LED for SPEED mode
    _state.led.VNAV    = dr_is_on(_drLedVnav);
    _state.led.LVL_CHG = dr_is_on(_drLedFlch); // FLCH = LVL_CHG
    _state.led.HDG_SEL = false; // No direct HDG SEL LED in FF777
    _state.led.LNAV    = dr_is_on(_drLedLnav);
    _state.led.VORLOC  = dr_is_on(_drLedLoc);
    _state.led.APP     = dr_is_on(_drLedApp);
    _state.led.ALT_HLD = dr_is_on(_drLedAltHold);
    _state.led.V_S     = dr_is_on(_drLedVs);
    _state.led.CMD_A   = dr_is_on(_drLedCaptAP);
    _state.led.CWS_A   = false; // FF777 doesn't have CWS LEDs
    _state.led.CMD_B   = false; // FF777 has single AP engage
    _state.led.CWS_B   = false;
    _state.led.AT_ARM  = dr_is_on(_drLedAt);
    _state.led.MA_CAPT = false; // FF777 doesn't have separate Master lights
    _state.led.MA_FO   = false; // FF777 doesn't have separate Master lights

    if (_cb) _cb(_state);
}

// -----------------------------------------------------------------------------
// Command helpers
// -----------------------------------------------------------------------------
void FF777PAP3Profile::execOnce(XPLMCommandRef cmd) { 
    if (cmd) XPLMCommandOnce(cmd); 
}

void FF777PAP3Profile::repeatCmd(XPLMCommandRef inc, XPLMCommandRef dec, int8_t delta, int stepPerTick) {
    if (delta == 0) return;
    const bool up  = (delta > 0);
    const int  reps = std::max(1, static_cast<int>(std::abs(static_cast<int>(delta))) * std::max(1, stepPerTick));
    XPLMCommandRef cmd = up ? inc : dec;
    if (!cmd) return;
    for (int i = 0; i < reps; ++i) XPLMCommandOnce(cmd);
}

bool FF777PAP3Profile::debounce(float& lastTs, float minDeltaSec) const {
    const float now = XPLMGetElapsedTime();
    if (now - lastTs < minDeltaSec) return false;
    lastTs = now;
    return true;
}

// ---------- Toggle helper ----------
void FF777PAP3Profile::toggleToPosIfNeeded(XPLMDataRef posRef,
                                           int desiredPos,
                                           XPLMCommandRef toggleCmd,
                                           float& guardTs,
                                           int& lastCommandedPos) {
    if (!posRef || !toggleCmd) return;
    const int simPos = dr_get_pos(posRef);

    if (simPos == desiredPos) {
        guardTs = 0.f;
        lastCommandedPos = desiredPos;
        return;
    }

    if (desiredPos != lastCommandedPos) {
        guardTs = 0.f;
    }

    const float now = XPLMGetElapsedTime();
    if (guardTs > 0.f && (now - guardTs) < 0.20f) return;

    guardTs = now;
    lastCommandedPos = desiredPos;
    XPLMCommandOnce(toggleCmd);
}

// -----------------------------------------------------------------------------
// Input hooks -> update Intent -> applyIntent()
// -----------------------------------------------------------------------------
void FF777PAP3Profile::onButton(std::uint8_t off, std::uint8_t mask, bool pressed) {
    // -------- F/D LEFT ON (0x04/0x08) --------
    if (off == 0x04 && mask == 0x08) {
        if (!pressed) return;
        _intent.fd_left_on = true;
        _haveIntent = true;
        toggleToPosIfNeeded(_drFDLeftPos, FD_ON_POS, _cmdFDLeftToggle, _guardFdLeftTs, _lastFdLeftCmdPos);
        return;
    }

    // -------- F/D LEFT OFF (0x04/0x10) --------
    if (off == 0x04 && mask == 0x10) {
        if (!pressed) return;
        _intent.fd_left_on = false;
        _haveIntent = true;
        toggleToPosIfNeeded(_drFDLeftPos, FD_OFF_POS, _cmdFDLeftToggle, _guardFdLeftTs, _lastFdLeftCmdPos);
        return;
    }

    // -------- F/D RIGHT ON (0x04/0x20) --------
    if (off == 0x04 && mask == 0x20) {
        if (!pressed) return;
        _intent.fd_right_on = true;
        _haveIntent = true;
        toggleToPosIfNeeded(_drFDRightPos, FD_ON_POS, _cmdFDRightToggle, _guardFdRightTs, _lastFdRightCmdPos);
        return;
    }

    // -------- F/D RIGHT OFF (0x04/0x40) --------
    if (off == 0x04 && mask == 0x40) {
        if (!pressed) return;
        _intent.fd_right_on = false;
        _haveIntent = true;
        toggleToPosIfNeeded(_drFDRightPos, FD_OFF_POS, _cmdFDRightToggle, _guardFdRightTs, _lastFdRightCmdPos);
        return;
    }

    // -------- A/T ARM ON (0x06/0x01) --------
    if (off == 0x06 && mask == 0x01) {
        if (!pressed) return;
        _intent.at_arm_on = true;
        _haveIntent = true;
        if (_device) {
            _device->setAtHardwareIntent(true);
        }
        const int desiredPos = AT_ON_POS;
        if (desiredPos != _lastAtCmdPos) {
            _lastAtToggleTime = 0.0f;
        }
        if (debounce(_lastAtToggleTime, 0.15f)) {
            // Toggle both left and right A/T switches for 777
            toggleToPosIfNeeded(_drATArmPosLeft, desiredPos, _cmdATArmToggleLeft, _guardAtLeftTs, _lastAtLeftCmdPos);
            toggleToPosIfNeeded(_drATArmPos, desiredPos, _cmdATArmToggle, _guardAtTs, _lastAtCmdPos);
        }
        return;
    }

    // -------- A/T ARM OFF (0x06/0x02) --------
    if (off == 0x06 && mask == 0x02) {
        if (!pressed) return;
        _intent.at_arm_on = false;
        _haveIntent = true;
        if (_device) {
            _device->setAtHardwareIntent(false);
        }
        const int desiredPos = AT_OFF_POS;
        if (desiredPos != _lastAtCmdPos) {
            _lastAtToggleTime = 0.0f;
        }
        if (debounce(_lastAtToggleTime, 0.15f)) {
            // Toggle both left and right A/T switches for 777
            toggleToPosIfNeeded(_drATArmPosLeft, desiredPos, _cmdATArmToggleLeft, _guardAtLeftTs, _lastAtLeftCmdPos);
            toggleToPosIfNeeded(_drATArmPos, desiredPos, _cmdATArmToggle, _guardAtTs, _lastAtCmdPos);
        }
        return;
    }

    // -------- A/P DISENGAGE ON (0x04/0x80) --------
    if (off == 0x04 && mask == 0x80) {
        if (!pressed) return;
        _intent.ap_engaged = false;  // Reversed: disengage ON means AP should be OFF
        _haveIntent = true;
        const int desiredPos = AP_OFF_POS;  // Reversed
        if (desiredPos != _lastApDiscCmdPos) {
            _lastApDiscToggleTime = 0.0f;
        }
        if (debounce(_lastApDiscToggleTime, 0.15f)) {
            toggleToPosIfNeeded(_drApDiscPos, desiredPos, _cmdApDiscToggle, _guardApDiscTs, _lastApDiscCmdPos);
        }
        return;
    }

    // -------- A/P DISENGAGE OFF (0x05/0x01) --------
    if (off == 0x05 && mask == 0x01) {
        if (!pressed) return;
        _intent.ap_engaged = true;  // Reversed: disengage OFF means AP should be ON
        _haveIntent = true;
        const int desiredPos = AP_ON_POS;  // Reversed
        if (desiredPos != _lastApDiscCmdPos) {
            _lastApDiscToggleTime = 0.0f;
        }
        if (debounce(_lastApDiscToggleTime, 0.15f)) {
            toggleToPosIfNeeded(_drApDiscPos, desiredPos, _cmdApDiscToggle, _guardApDiscTs, _lastApDiscCmdPos);
        }
        return;
    }

    // -------- Momentary buttons --------
    for (const auto& b : _btns) {
        if (b.off == off && b.mask == mask) {
            execOnce(pressed ? b.press : b.release);
            return;
        }
    }
}

void FF777PAP3Profile::onEncoderDelta(std::uint8_t posOff, std::int8_t delta) {
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
uint8_t FF777PAP3Profile::mcpPowerMask() const {
    uint8_t mask = 0;
    if (dr_is_on(_drHasApPower)) mask |= 0x01; // AP has power
    if (dr_is_on(_drDcBus1))     mask |= 0x02; // DC bus 1
    if (dr_is_on(_drDcBus2))     mask |= 0x04; // DC bus 2
    return mask;
}

bool FF777PAP3Profile::mcpHasPower() const { 
    return (mcpPowerMask() & 0x01) != 0;
}

// -----------------------------------------------------------------------------
// Boot syncs
// -----------------------------------------------------------------------------
void FF777PAP3Profile::seedFromSim() {
    _intent.fd_left_on  = (dr_get_pos(_drFDLeftPos)  == FD_ON_POS);
    _intent.fd_right_on = (dr_get_pos(_drFDRightPos) == FD_ON_POS);
    
    // 777 has both left and right A/T switches - consider armed if either is on
    const bool atLeftOn = (dr_get_pos(_drATArmPosLeft) == AT_ON_POS);
    const bool atRightOn = (dr_get_pos(_drATArmPos) == AT_ON_POS);
    _intent.at_arm_on = (atLeftOn || atRightOn);
    _state.atArmOn = _intent.at_arm_on;
    
    _intent.ap_engaged = (dr_get_pos(_drApDiscPos) == AP_OFF_POS);  // Reversed logic
    
    _lastFdLeftCmdPos  = dr_get_pos(_drFDLeftPos);
    _lastFdRightCmdPos = dr_get_pos(_drFDRightPos);
    _lastAtLeftCmdPos  = dr_get_pos(_drATArmPosLeft);
    _lastAtCmdPos      = dr_get_pos(_drATArmPos);
    _lastApDiscCmdPos  = dr_get_pos(_drApDiscPos);
    
    _haveIntent = true;
    _pendingIntentApply = true;
    
    if (_device) {
        _device->setAtHardwareIntent(_intent.at_arm_on);
    }
}

void FF777PAP3Profile::seedFromRaw(const std::uint8_t* r, int len) {
    // F/D LEFT flags
    const bool fdLeftOn  = bit(r, len, 0x04, 0x08);
    const bool fdLeftOff = bit(r, len, 0x04, 0x10);
    if      (fdLeftOn)  _intent.fd_left_on = true;
    else if (fdLeftOff) _intent.fd_left_on = false;
    else                _intent.fd_left_on = (dr_get_pos(_drFDLeftPos) == FD_ON_POS);

    // F/D RIGHT flags
    const bool fdRightOn  = bit(r, len, 0x04, 0x20);
    const bool fdRightOff = bit(r, len, 0x04, 0x40);
    if      (fdRightOn)  _intent.fd_right_on = true;
    else if (fdRightOff) _intent.fd_right_on = false;
    else                 _intent.fd_right_on = (dr_get_pos(_drFDRightPos) == FD_ON_POS);

    // A/T ARM
    const bool atOn  = bit(r, len, 0x06, 0x01);
    const bool atOff = bit(r, len, 0x06, 0x02);
    if      (atOn)  _intent.at_arm_on = true;
    else if (atOff) _intent.at_arm_on = false;
    else {
        // 777 has both left and right A/T switches - consider armed if either is on
        const bool atLeftOn = (dr_get_pos(_drATArmPosLeft) == AT_ON_POS);
        const bool atRightOn = (dr_get_pos(_drATArmPos) == AT_ON_POS);
        _intent.at_arm_on = (atLeftOn || atRightOn);
    }

    // A/P DISENGAGE
    const bool apDiscOn  = bit(r, len, 0x04, 0x80);
    const bool apDiscOff = bit(r, len, 0x05, 0x01);
    if      (apDiscOn)  _intent.ap_engaged = false;  // Reversed logic
    else if (apDiscOff) _intent.ap_engaged = true;   // Reversed logic
    else                _intent.ap_engaged = (dr_get_pos(_drApDiscPos) == AP_OFF_POS);  // Reversed logic

    _lastFdLeftCmdPos  = dr_get_pos(_drFDLeftPos);
    _lastFdRightCmdPos = dr_get_pos(_drFDRightPos);
    _lastAtLeftCmdPos  = dr_get_pos(_drATArmPosLeft);
    _lastAtCmdPos      = dr_get_pos(_drATArmPos);
    _lastApDiscCmdPos  = dr_get_pos(_drApDiscPos);
    
    // Update state - armed if either switch is on
    const bool atLeftOn = (dr_get_pos(_drATArmPosLeft) == AT_ON_POS);
    const bool atRightOn = (dr_get_pos(_drATArmPos) == AT_ON_POS);
    _state.atArmOn = (atLeftOn || atRightOn);

    _haveIntent = true;
    _pendingIntentApply = true;
    
    if (_device) {
        _device->setAtHardwareIntent(_intent.at_arm_on);
    }
}

void FF777PAP3Profile::applyIntent() {
    if (!_haveIntent) return;

    // F/D
    toggleToPosIfNeeded(_drFDLeftPos,  _intent.fd_left_on  ? FD_ON_POS : FD_OFF_POS, _cmdFDLeftToggle,  _guardFdLeftTs,  _lastFdLeftCmdPos);
    toggleToPosIfNeeded(_drFDRightPos, _intent.fd_right_on ? FD_ON_POS : FD_OFF_POS, _cmdFDRightToggle, _guardFdRightTs, _lastFdRightCmdPos);

    // A/T - toggle both left and right for 777
    const int atDesiredPos = _intent.at_arm_on ? AT_ON_POS : AT_OFF_POS;
    toggleToPosIfNeeded(_drATArmPosLeft, atDesiredPos, _cmdATArmToggleLeft, _guardAtLeftTs, _lastAtLeftCmdPos);
    toggleToPosIfNeeded(_drATArmPos, atDesiredPos, _cmdATArmToggle, _guardAtTs, _lastAtCmdPos);
    if (_device) {
        _device->setAtHardwareIntent(_intent.at_arm_on);
    }
    
    // A/P DISENGAGE
    const int apDiscDesiredPos = _intent.ap_engaged ? AP_ON_POS : AP_OFF_POS;
    toggleToPosIfNeeded(_drApDiscPos, apDiscDesiredPos, _cmdApDiscToggle, _guardApDiscTs, _lastApDiscCmdPos);
}

void FF777PAP3Profile::syncSimToHardware() {
    seedFromSim();
    applyIntent();
}

void FF777PAP3Profile::syncSimToHardwareFromRaw(const std::uint8_t* r, int len) {
    if (r && len > 0) seedFromRaw(r, len);
    else              seedFromSim();
    applyIntent();

    _lastAtToggleTime = 0.0f;
    _lastApDiscToggleTime = 0.0f;
}

} // namespace pap3::aircraft

