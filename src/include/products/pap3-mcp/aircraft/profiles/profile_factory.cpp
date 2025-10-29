#include "profile_factory.h"
#include "zibo_profile.h"
#include "ff777_profile.h"
#include "rotatemd11_profile.h"

namespace pap3::aircraft {

std::unique_ptr<PAP3AircraftProfile> ProfileFactory::detect() {
    // Try FlightFactor 777 first
    {
        auto ff777 = std::make_unique<FF777PAP3Profile>();
        if (ff777->isEligible()) {
            return ff777;
        }
    }
    
    // Try Rotate MD-11
    {
        auto rotatemd11 = std::make_unique<RotateMD11PAP3Profile>();
        if (rotatemd11->isEligible()) {
            return rotatemd11;
        }
    }
    
    // Try Zibo
    {
        auto zibo = std::make_unique<ZiboPAP3Profile>();
        if (zibo->isEligible()) {
            return zibo;
        }
    }
    return nullptr;
}

} // namespace pap3::aircraft