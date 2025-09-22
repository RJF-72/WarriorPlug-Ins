#include "USBAudioDetector.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

namespace Warrior {

USBAudioDetector::USBAudioDetector() 
    : usbContext(nullptr), initialized(false), monitoring(false), shouldStopMonitoring(false) {
}

USBAudioDetector::~USBAudioDetector() {
    shutdown();
}

bool USBAudioDetector::initialize() {
    if (initialized) return true;
    
    usbContext = nullptr;
    std::cout << "USB Audio Detector initialized (simulation mode)" << std::endl;
    
    initialized = true;
    loadInstrumentProfiles();
    
    return true;
}

void USBAudioDetector::shutdown() {
    if (!initialized) return;
    
    stopDeviceMonitoring();
    initialized = false;
}

std::vector<USBDevice> USBAudioDetector::scanForAudioDevices() {
    std::vector<USBDevice> devices;
    
    if (!initialized) return devices;
    
    // Create a dummy device for testing
    USBDevice dummyDevice = {};
    dummyDevice.vendorId = 0x1234;
    dummyDevice.productId = 0x5678;
    dummyDevice.manufacturerName = "Virtual Audio";
    dummyDevice.productName = "Test USB Interface";
    dummyDevice.serialNumber = "TEST001";
    dummyDevice.isAudioDevice = true;
    dummyDevice.audioChannels = 2;
    dummyDevice.sampleRate = 44100;
    devices.push_back(dummyDevice);
    
    connectedDevices = devices;
    return devices;
}

bool USBAudioDetector::isDeviceConnected(const USBDevice& device) {
    auto devices = scanForAudioDevices();
    return std::find_if(devices.begin(), devices.end(), 
        [&device](const USBDevice& d) {
            return d.vendorId == device.vendorId && 
                   d.productId == device.productId &&
                   d.serialNumber == device.serialNumber;
        }) != devices.end();
}

void USBAudioDetector::setDeviceConnectedCallback(DeviceConnectedCallback callback) {
    onDeviceConnected = callback;
}

void USBAudioDetector::setDeviceDisconnectedCallback(DeviceDisconnectedCallback callback) {
    onDeviceDisconnected = callback;
}

InstrumentProfile USBAudioDetector::identifyInstrument(const USBDevice& device) {
    // Search through known instrument profiles
    for (const auto& profile : instrumentProfiles) {
        auto vendorMatch = std::find(profile.knownVendorIds.begin(), profile.knownVendorIds.end(), device.vendorId);
        auto productMatch = std::find(profile.knownProductIds.begin(), profile.knownProductIds.end(), device.productId);
        
        if (vendorMatch != profile.knownVendorIds.end() || productMatch != profile.knownProductIds.end()) {
            return profile;
        }
    }
    
    // Default profile for unknown devices
    InstrumentProfile defaultProfile;
    defaultProfile.name = "Unknown Instrument";
    defaultProfile.instrumentType = "generic";
    defaultProfile.preferredSampleRates = {44100, 48000, 96000};
    defaultProfile.preferredChannels = device.audioChannels;
    defaultProfile.suggestedGain = 0.5f;
    defaultProfile.suggestedGenre = "rock";
    
    return defaultProfile;
}

void USBAudioDetector::loadInstrumentProfiles() {
    instrumentProfiles.clear();
    
    // Add some common instrument profiles
    InstrumentProfile guitarProfile;
    guitarProfile.name = "Electric Guitar Interface";
    guitarProfile.knownVendorIds = {0x041e, 0x0763, 0x0582}; // Creative, M-Audio, Roland
    guitarProfile.knownProductIds = {0x3f02, 0x2080, 0x012a}; 
    guitarProfile.instrumentType = "guitar";
    guitarProfile.preferredSampleRates = {44100, 48000};
    guitarProfile.preferredChannels = 1;
    guitarProfile.suggestedGain = 0.7f;
    guitarProfile.suggestedGenre = "rock";
    instrumentProfiles.push_back(guitarProfile);
    
    InstrumentProfile micProfile;
    micProfile.name = "USB Microphone";
    micProfile.knownVendorIds = {0x0b05, 0x17cc, 0x046d}; // Blue, Audio-Technica, Logitech
    micProfile.instrumentType = "microphone";
    micProfile.preferredSampleRates = {44100, 48000, 96000};
    micProfile.preferredChannels = 1;
    micProfile.suggestedGain = 0.6f;
    micProfile.suggestedGenre = "vocal";
    instrumentProfiles.push_back(micProfile);
    
    InstrumentProfile keyboardProfile;
    keyboardProfile.name = "MIDI Keyboard";
    keyboardProfile.knownVendorIds = {0x09e8, 0x0944, 0x15ca}; // AKAI, Korg, M-Audio
    keyboardProfile.instrumentType = "keyboard";
    keyboardProfile.preferredSampleRates = {44100, 48000};
    keyboardProfile.preferredChannels = 2;
    keyboardProfile.suggestedGain = 0.8f;
    keyboardProfile.suggestedGenre = "electronic";
    instrumentProfiles.push_back(keyboardProfile);
}

void USBAudioDetector::addCustomInstrumentProfile(const InstrumentProfile& profile) {
    instrumentProfiles.push_back(profile);
}

void USBAudioDetector::startDeviceMonitoring() {
    if (monitoring) return;
    
    monitoring = true;
    shouldStopMonitoring = false;
    
    monitorThread = std::thread([this]() {
        pollForDeviceChanges();
    });
}

void USBAudioDetector::stopDeviceMonitoring() {
    if (!monitoring) return;
    
    shouldStopMonitoring = true;
    
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
    
    monitoring = false;
}

void USBAudioDetector::pollForDeviceChanges() {
    auto previousDevices = connectedDevices;
    
    while (!shouldStopMonitoring) {
        auto currentDevices = scanForAudioDevices();
        
        // Check for newly connected devices
        for (const auto& device : currentDevices) {
            auto found = std::find_if(previousDevices.begin(), previousDevices.end(),
                [&device](const USBDevice& d) {
                    return d.vendorId == device.vendorId && 
                           d.productId == device.productId &&
                           d.serialNumber == device.serialNumber;
                });
            
            if (found == previousDevices.end() && onDeviceConnected) {
                onDeviceConnected(device);
            }
        }
        
        // Check for disconnected devices
        for (const auto& device : previousDevices) {
            auto found = std::find_if(currentDevices.begin(), currentDevices.end(),
                [&device](const USBDevice& d) {
                    return d.vendorId == device.vendorId && 
                           d.productId == device.productId &&
                           d.serialNumber == device.serialNumber;
                });
            
            if (found == currentDevices.end() && onDeviceDisconnected) {
                onDeviceDisconnected(device);
            }
        }
        
        previousDevices = currentDevices;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

} // namespace Warrior