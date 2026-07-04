#ifndef KINGAIR350_FCU_EFIS_PROFILE_H
#define KINGAIR350_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <map>
#include <string>
#include <unordered_map>

class KingAir350FCUEfisProfile : public FCUEfisAircraftProfile {
    public:
        KingAir350FCUEfisProfile(ProductFCUEfis *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, FCUEfisButtonDef> &buttonDefs() const override;
        void updateDisplayData(FCUDisplayData &data) override;

        bool hasEfisLeft() const override {
            return true;
        }

        bool hasEfisRight() const override {
            return false;
        }

        void buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) override;
};

#endif
