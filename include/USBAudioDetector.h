#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

// Make USB detection optional
#ifdef HAVE_LIBUSB
#include <libusb-1.0/libusb.h>
#endif

#ifdef HAVE_PORTAUDIO
#include <portaudio.h>
#endif

namespace Warrior {

struct USBDevice {
    uint16_t vendorId;
    uint16_t productId;
    std::string manufacturerName;
    std::string productName;
    std::string serialNumber;
    bool isAudioDevice;
    int audioChannels;
    int sampleRate;
    std::string devicePath;
};

struct InstrumentProfile {
    std::string name;
    std::vector<uint16_t> knownVendorIds;
    std::vector<uint16_t> knownProductIds;
    std::string instrumentType; // "guitar", "bass", "microphone", "keyboard", etc.
    std::vector<int> preferredSampleRates;
    int preferredChannels;
    float suggestedGain;
    std::string suggestedGenre;
};

class USBAudioDetector {
public:
    USBAudioDetector();
    ~USBAudioDetector();

    // Device detection
    bool initialize();
    void shutdown();
    std::vector<USBDevice> scanForAudioDevices();
    bool isDeviceConnected(const USBDevice& device);
    
    // Auto-detection callbacks
    using DeviceConnectedCallback = std::function<void(const USBDevice&)>;
    using DeviceDisconnectedCallback = std::function<void(const USBDevice&)>;
    
    void setDeviceConnectedCallback(DeviceConnectedCallback callback);
    void setDeviceDisconnectedCallback(DeviceDisconnectedCallback callback);
    
    // Instrument profiling
    InstrumentProfile identifyInstrument(const USBDevice& device);
    void loadInstrumentProfiles();
    void addCustomInstrumentProfile(const InstrumentProfile& profile);
    
    // Audio configuration
#ifdef HAVE_PORTAUDIO
    bool configureAudioDevice(const USBDevice& device, int sampleRate, int channels);
    PaStream* createAudioStream(const USBDevice& device, int sampleRate, int channels);
#endif
    
    // Monitoring
    void startDeviceMonitoring();
    void stopDeviceMonitoring();
    bool isMonitoring() const { return monitoring; }

private:
#ifdef HAVE_LIBUSB
    libusb_context* usbContext;
#else
    void* usbContext; // Placeholder when libusb is not available
#endif
    bool initialized;
    bool monitoring;
    
    std::vector<USBDevice> connectedDevices;
    std::vector<InstrumentProfile> instrumentProfiles;
    
    DeviceConnectedCallback onDeviceConnected;
    DeviceDisconnectedCallback onDeviceDisconnected;
    
    // Internal methods
#ifdef HAVE_LIBUSB
    bool isAudioInterface(libusb_device* device);
    USBDevice createUSBDeviceInfo(libusb_device* device);
    std::string getStringDescriptor(libusb_device_handle* handle, uint8_t index);
#endif
    void pollForDeviceChanges();
    
    // Threading for monitoring
    std::thread monitorThread;
    std::atomic<bool> shouldStopMonitoring;
};

} // namespace Warrior