#ifndef PRODUCT_FMC_H
#define PRODUCT_FMC_H

#include "fmc-aircraft-profile.h"
#include "usbdevice.h"

#include <chrono>
#include <map>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>

class ProductFMC : public USBDevice {
    private:
        // Worker queue command structure (needs to be declared first)
        struct IoCmd {
            enum Type : uint8_t {
                DrawPage,
                WriteData
            } type;
            std::vector<uint8_t> data;
            std::vector<std::vector<char>> pageData;
        };

        FMCAircraftProfile *profile;
        std::vector<std::vector<char>> page;
        int lastUpdateCycle;
        int displayUpdateFrameCounter = 0;
        std::set<int> pressedButtonIndices;
        uint64_t lastButtonStateLo;
        uint32_t lastButtonStateHi;

        // I/O worker thread
        std::thread              _ioThread;
        std::atomic<bool>        _ioRunning{false};
        std::mutex               _ioMx;
        std::condition_variable  _ioCv;
        std::deque<IoCmd>        _ioQueue;

        // Coalescing state for last sent page
        std::vector<std::vector<char>> _sentPage;
        
        // Drawing rate-limit (similar to PAP3 LCD rate-limit)
        float                    _minDrawPeriod = 1.f / 25.f; // ~25 Hz

        void updatePage();
        void draw(const std::vector<std::vector<char>> *pagePtr = nullptr);
        std::pair<uint8_t, uint8_t> dataFromColFont(char color, bool fontSmall = false);

        void setProfileForCurrentAircraft();

        // Worker thread main loop
        void ioThreadMain();

        inline void qEnqueue(IoCmd&& c) {
            { std::lock_guard<std::mutex> lk(_ioMx); _ioQueue.emplace_back(std::move(c)); }
            _ioCv.notify_one();
        }
        inline void qDrawPage(const std::vector<std::vector<char>>& page) {
            IoCmd c; c.type = IoCmd::DrawPage; c.pageData = page; qEnqueue(std::move(c));
        }
        inline void qWriteData(const std::vector<uint8_t>& data) {
            IoCmd c; c.type = IoCmd::WriteData; c.data = data; qEnqueue(std::move(c));
        }

    public:
        ProductFMC(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, FMCHardwareType hardwareType, FMCDeviceVariant variant, unsigned char identifierByte);
        ~ProductFMC();

        static constexpr unsigned int PageLines = 14; // Header + 6 * label + 6 * cont + textbox
        static constexpr unsigned int PageCharsPerLine = 24;
        static constexpr unsigned int PageBytesPerChar = 3;
        static constexpr unsigned int PageBytesPerLine = PageCharsPerLine * PageBytesPerChar;
        FMCHardwareType hardwareType;
        const unsigned char identifierByte;
        const FMCDeviceVariant deviceVariant;
        bool fontUpdatingEnabled;

        const char *classIdentifier() override;
        bool connect() override;
        void disconnect() override;
        void unloadProfile();
        void update() override;
        void didReceiveData(int reportId, uint8_t *report, int reportLength) override;
        void didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count = 1) override;

        void writeLineToPage(std::vector<std::vector<char>> &page, int line, int pos, const std::string &text, char color, bool fontSmall = false);
        void setFont(std::vector<std::vector<unsigned char>> font);

        void setAllLedsEnabled(bool enable);
        void setLedBrightness(FMCLed led, uint8_t brightness);

        void clearDisplay();
        void showBackground(FMCBackgroundVariant variant);
    
        void setDeviceVariant(FMCDeviceVariant variant);
};

#endif
