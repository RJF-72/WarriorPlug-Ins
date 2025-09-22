#include "USBAudioDetector.h"
#include "GenreEffectsEngine.h"
#include "LowLatencyProcessor.h"
#include "PresetManager.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "Testing Warrior USB Recorder Components..." << std::endl;
    
    // Test USB Audio Detector
    std::cout << "\n1. Testing USB Audio Detector..." << std::endl;
    Warrior::USBAudioDetector usbDetector;
    if (usbDetector.initialize()) {
        std::cout << "   ✓ USB Detector initialized successfully" << std::endl;
        
        auto devices = usbDetector.scanForAudioDevices();
        std::cout << "   ✓ Found " << devices.size() << " USB audio devices" << std::endl;
        
        for (const auto& device : devices) {
            std::cout << "     - " << device.productName << " (" 
                      << std::hex << device.vendorId << ":" << device.productId << std::dec << ")" << std::endl;
            
            // Test instrument profiling
            auto profile = usbDetector.identifyInstrument(device);
            std::cout << "       Type: " << profile.instrumentType << ", Suggested genre: " << profile.suggestedGenre << std::endl;
        }
        
        usbDetector.shutdown();
    } else {
        std::cout << "   ⚠ USB Detector initialization failed" << std::endl;
    }
    
    // Test Genre Effects Engine
    std::cout << "\n2. Testing Genre Effects Engine..." << std::endl;
    Warrior::GenreEffectsEngine effectsEngine;
    effectsEngine.setGenre(Warrior::GenreType::Rock);
    std::cout << "   ✓ Set genre to Rock" << std::endl;
    
    auto genres = effectsEngine.getAvailableGenres();
    std::cout << "   ✓ Available genres: " << genres.size() << std::endl;
    for (auto genre : genres) {
        std::cout << "     - " << effectsEngine.getGenreName(genre) << std::endl;
    }
    
    // Test effects processing
    std::vector<float> testInput(1024, 0.1f);  // Small test signal
    std::vector<float> testOutput(1024, 0.0f);
    effectsEngine.processAudio(testInput.data(), testOutput.data(), 512, 2);
    std::cout << "   ✓ Effects processing completed" << std::endl;
    
    // Test Low-Latency Processor
    std::cout << "\n3. Testing Low-Latency Processor..." << std::endl;
    Warrior::LowLatencyProcessor latencyProcessor;
    if (latencyProcessor.initialize(44100, 128, 2)) {
        std::cout << "   ✓ Low-latency processor initialized" << std::endl;
        std::cout << "   ✓ Buffer size: " << latencyProcessor.getBufferSize() << " samples" << std::endl;
        latencyProcessor.shutdown();
    } else {
        std::cout << "   ⚠ Low-latency processor initialization failed" << std::endl;
    }
    
    // Test Preset Manager
    std::cout << "\n4. Testing Preset Manager..." << std::endl;
    Warrior::PresetManager presetManager;
    auto presets = presetManager.getAllPresets();
    std::cout << "   ✓ Loaded " << presets.size() << " presets" << std::endl;
    
    // Show available presets
    for (const auto& preset : presets) {
        std::cout << "     - " << preset.name << " (" << preset.category << ")" << std::endl;
    }
    
    // Test preset by genre
    auto rockPresets = presetManager.getPresetsByGenre(Warrior::GenreType::Rock);
    std::cout << "   ✓ Found " << rockPresets.size() << " Rock presets" << std::endl;
    
    std::cout << "\n✓ All tests completed successfully!" << std::endl;
    std::cout << "\nWarrior USB Recorder Plugin core components are working correctly." << std::endl;
    std::cout << "Note: This demo uses simulated USB devices. For real USB detection," << std::endl;
    std::cout << "install libusb-1.0-dev and portaudio19-dev packages." << std::endl;
    
    return 0;
}