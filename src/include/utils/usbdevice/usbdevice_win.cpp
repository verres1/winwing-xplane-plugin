#if IBM
#include "appstate.h"
#include "config.h"
#include "usbdevice.h"

#include <chrono>
#include <hidsdi.h>
#include <iostream>
#include <setupapi.h>
#include <thread>
#include <windows.h>
#include <XPLMUtilities.h>

USBDevice::USBDevice(HIDDeviceHandle aHidDevice, uint16_t aVendorId, uint16_t aProductId, std::string aVendorName, std::string aProductName) :
    hidDevice(aHidDevice), vendorId(aVendorId), productId(aProductId), vendorName(aVendorName), productName(aProductName), connected(false) {}

USBDevice::~USBDevice() {
    disconnect();
}

bool USBDevice::connect() {
    static const size_t kInputReportSize = 65;
    if (inputBuffer) {
        delete[] inputBuffer;
        inputBuffer = nullptr;
    }
    inputBuffer = new uint8_t[kInputReportSize];

    // Start input reading thread with proper cleanup
    connected = true;
    std::thread inputThread([this]() {
        uint8_t buffer[65];
        DWORD bytesRead;
        while (connected && hidDevice != INVALID_HANDLE_VALUE) {
            BOOL result = ReadFile(hidDevice, buffer, sizeof(buffer), &bytesRead, nullptr);
            if (result && bytesRead > 0 && connected) {
                InputReportCallback(this, bytesRead, buffer);
            } else if (!result) {
                DWORD error = GetLastError();
                if (error != ERROR_DEVICE_NOT_CONNECTED) {
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    inputThread.detach();

    return true;
}

void USBDevice::InputReportCallback(void *context, DWORD bytesRead, uint8_t *report) {
    auto *self = static_cast<USBDevice *>(context);
    if (!self || !self->connected || !report || bytesRead == 0) {
        return;
    }

    if (self->hidDevice == INVALID_HANDLE_VALUE) {
        return;
    }

    try {
        InputEvent event;
        event.reportId = report[0];
        event.reportData.assign(report, report + bytesRead);
        event.reportLength = (int) bytesRead;

        self->processOnMainThread(event);
    } catch (const std::system_error &e) {
        return;
    } catch (...) {
        return;
    }
}

void USBDevice::update() {
    if (!connected) {
        return;
    }

    processQueuedEvents();
}

void USBDevice::disconnect() {
    connected = false;

    if (hidDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hidDevice);
        hidDevice = INVALID_HANDLE_VALUE;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (inputBuffer) {
        delete[] inputBuffer;
        inputBuffer = nullptr;
    }
}

void USBDevice::forceStateSync() {
    // noop, code does not use partial data
}

bool USBDevice::writeData(std::vector<uint8_t> data) {
    if (hidDevice == INVALID_HANDLE_VALUE || !connected || data.empty()) {
        return false;
    }

    if (data.size() > 1024) {
        return false;
    }

    DWORD bytesWritten;
    BOOL result = WriteFile(hidDevice, data.data(), (DWORD) data.size(), &bytesWritten, nullptr);
    if (!result || bytesWritten < data.size()) {
        DWORD error = GetLastError();
        return false;
    }
    return true;
}
#endif
