#include "WarriorUSBRecorderProcessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include <iostream>
#include <cstring>

namespace Warrior {

WarriorUSBRecorderProcessor::WarriorUSBRecorderProcessor()
    : sampleRate(44100.0), maxSamplesPerBlock(512), isRecording(false) {
    // Initialize parameter values
    paramValues[kInputGain] = 0.7f;
    paramValues[kOutputGain] = 0.8f;
    paramValues[kGenreSelect] = 0.0f; // Rock
    paramValues[kEffectMix] = 0.5f;
    paramValues[kUSBAutoDetect] = 1.0f;
    paramValues[kLowLatencyMode] = 1.0f;
    paramValues[kRecordEnable] = 0.0f;
}

WarriorUSBRecorderProcessor::~WarriorUSBRecorderProcessor() {
    terminate();
}

Steinberg::tresult WarriorUSBRecorderProcessor::initialize(Steinberg::FUnknown* context) {
    Steinberg::tresult result = AudioEffect::initialize(context);
    if (result != Steinberg::kResultOk) {
        return result;
    }
    
    // Initialize core components
    usbDetector = std::make_unique<USBAudioDetector>();
    effectsEngine = std::make_unique<GenreEffectsEngine>();
    latencyProcessor = std::make_unique<LowLatencyProcessor>();
    presetManager = std::make_unique<PresetManager>();
    
    // Initialize USB detector
    if (!usbDetector->initialize()) {
        std::cerr << "Failed to initialize USB detector" << std::endl;
    }
    
    // Set up device callbacks
    usbDetector->setDeviceConnectedCallback([this](const USBDevice& device) {
        std::cout << "USB device connected: " << device.productName << std::endl;
        
        // Auto-detect instrument and apply appropriate genre/settings
        auto profile = usbDetector->identifyInstrument(device);
        std::cout << "Detected instrument: " << profile.name << " (" << profile.instrumentType << ")" << std::endl;
        
        // Set genre based on instrument
        if (profile.suggestedGenre == "rock") {
            effectsEngine->setGenre(GenreType::Rock);
            paramValues[kGenreSelect] = 0.0f;
        } else if (profile.suggestedGenre == "jazz") {
            effectsEngine->setGenre(GenreType::Jazz);
            paramValues[kGenreSelect] = 0.1f;
        } else if (profile.suggestedGenre == "electronic") {
            effectsEngine->setGenre(GenreType::Electronic);
            paramValues[kGenreSelect] = 0.3f;
        }
        
        // Apply suggested gain
        paramValues[kInputGain] = profile.suggestedGain;
    });
    
    usbDetector->setDeviceDisconnectedCallback([this](const USBDevice& device) {
        std::cout << "USB device disconnected: " << device.productName << std::endl;
    });
    
    // Start device monitoring if auto-detect is enabled
    if (paramValues[kUSBAutoDetect] > 0.5f) {
        usbDetector->startDeviceMonitoring();
    }
    
    std::cout << "Warrior USB Recorder initialized successfully" << std::endl;
    return Steinberg::kResultOk;
}

Steinberg::tresult WarriorUSBRecorderProcessor::terminate() {
    if (usbDetector) {
        usbDetector->stopDeviceMonitoring();
        usbDetector->shutdown();
    }
    
    if (latencyProcessor) {
        latencyProcessor->shutdown();
    }
    
    usbDetector.reset();
    effectsEngine.reset();
    latencyProcessor.reset();
    presetManager.reset();
    
    return AudioEffect::terminate();
}

Steinberg::tresult WarriorUSBRecorderProcessor::setActive(Steinberg::TBool state) {
    if (state) {
        // Initialize low-latency processor
        if (latencyProcessor && !latencyProcessor->initialize(static_cast<int>(sampleRate), 128, 2)) {
            std::cerr << "Failed to initialize low-latency processor" << std::endl;
        }
        
        // Start device scanning
        if (usbDetector) {
            auto devices = usbDetector->scanForAudioDevices();
            std::cout << "Found " << devices.size() << " USB audio devices" << std::endl;
            
            for (const auto& device : devices) {
                std::cout << "  - " << device.productName << " (" 
                          << device.vendorId << ":" << device.productId << ")" << std::endl;
            }
        }
    } else {
        if (latencyProcessor) {
            latencyProcessor->shutdown();
        }
    }
    
    return AudioEffect::setActive(state);
}

Steinberg::tresult WarriorUSBRecorderProcessor::setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) {
    sampleRate = newSetup.sampleRate;
    maxSamplesPerBlock = newSetup.maxSamplesPerBlock;
    
    // Update low-latency processor
    if (latencyProcessor) {
        latencyProcessor->shutdown();
        latencyProcessor->initialize(static_cast<int>(sampleRate), 
                                   static_cast<int>(maxSamplesPerBlock), 2);
    }
    
    return AudioEffect::setupProcessing(newSetup);
}

Steinberg::tresult WarriorUSBRecorderProcessor::process(Steinberg::Vst::ProcessData& data) {
    // Process parameter changes
    if (data.inputParameterChanges) {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numParamsChanged; i++) {
            auto* paramQueue = data.inputParameterChanges->getParameterData(i);
            if (!paramQueue) continue;
            
            Steinberg::Vst::ParamValue value;
            int32 sampleOffset;
            if (paramQueue->getPoint(paramQueue->getPointCount() - 1, sampleOffset, value) == Steinberg::kResultOk) {
                setParamNormalized(paramQueue->getParameterId(), value);
            }
        }
    }
    
    // Process audio
    if (data.numSamples > 0 && data.inputs && data.outputs) {
        float** inputs = data.inputs[0].channelBuffers32;
        float** outputs = data.outputs[0].channelBuffers32;
        int32 numChannels = data.inputs[0].numChannels;
        int32 numSamples = data.numSamples;
        
        // Apply input gain
        float inputGain = paramValues[kInputGain];
        for (int32 ch = 0; ch < numChannels; ch++) {
            for (int32 s = 0; s < numSamples; s++) {
                inputs[ch][s] *= inputGain;
            }
        }
        
        // Process through effects engine if we have input
        if (effectsEngine && numChannels >= 1) {
            // For stereo processing, interleave the samples
            std::vector<float> interleavedInput(numSamples * 2);
            std::vector<float> interleavedOutput(numSamples * 2);
            
            for (int32 s = 0; s < numSamples; s++) {
                interleavedInput[s * 2] = inputs[0][s];
                interleavedInput[s * 2 + 1] = numChannels > 1 ? inputs[1][s] : inputs[0][s];
            }
            
            // Process through effects
            effectsEngine->processAudio(interleavedInput.data(), interleavedOutput.data(), numSamples, 2);
            
            // Apply effect mix
            float effectMix = paramValues[kEffectMix];
            
            // Deinterleave and apply to outputs
            for (int32 s = 0; s < numSamples; s++) {
                float dryL = interleavedInput[s * 2];
                float dryR = interleavedInput[s * 2 + 1];
                float wetL = interleavedOutput[s * 2];
                float wetR = interleavedOutput[s * 2 + 1];
                
                outputs[0][s] = dryL * (1.0f - effectMix) + wetL * effectMix;
                if (numChannels > 1) {
                    outputs[1][s] = dryR * (1.0f - effectMix) + wetR * effectMix;
                }
            }
        } else {
            // Just pass through
            for (int32 ch = 0; ch < numChannels; ch++) {
                memcpy(outputs[ch], inputs[ch], numSamples * sizeof(float));
            }
        }
        
        // Apply output gain
        float outputGain = paramValues[kOutputGain];
        for (int32 ch = 0; ch < numChannels; ch++) {
            for (int32 s = 0; s < numSamples; s++) {
                outputs[ch][s] *= outputGain;
            }
        }
        
        // Process through low-latency processor for monitoring
        if (latencyProcessor && paramValues[kLowLatencyMode] > 0.5f) {
            // This would typically handle real-time monitoring and latency compensation
        }
    }
    
    return Steinberg::kResultOk;
}

Steinberg::uint32 WarriorUSBRecorderProcessor::getLatencySamples() {
    if (latencyProcessor && paramValues[kLowLatencyMode] > 0.5f) {
        return latencyProcessor->getBufferSize();
    }
    return 0; // No additional latency
}

Steinberg::tresult WarriorUSBRecorderProcessor::getParameterInfo(Steinberg::int32 paramIndex, Steinberg::Vst::ParameterInfo& info) {
    if (paramIndex < 0 || paramIndex >= kNumParams) {
        return Steinberg::kResultFalse;
    }
    
    info.id = paramIndex;
    info.flags = Steinberg::Vst::ParameterInfo::kCanAutomate;
    info.unitId = Steinberg::Vst::kRootUnitId;
    info.stepCount = 0;
    info.defaultNormalizedValue = 0.0;
    
    switch (paramIndex) {
        case kInputGain:
            strcpy(info.title, "Input Gain");
            strcpy(info.shortTitle, "In Gain");
            strcpy(info.units, "dB");
            info.defaultNormalizedValue = 0.7;
            break;
        case kOutputGain:
            strcpy(info.title, "Output Gain");
            strcpy(info.shortTitle, "Out Gain");
            strcpy(info.units, "dB");
            info.defaultNormalizedValue = 0.8;
            break;
        case kGenreSelect:
            strcpy(info.title, "Genre");
            strcpy(info.shortTitle, "Genre");
            strcpy(info.units, "");
            info.stepCount = 11; // 12 genres (0-11)
            info.flags |= Steinberg::Vst::ParameterInfo::kIsList;
            info.defaultNormalizedValue = 0.0;
            break;
        case kEffectMix:
            strcpy(info.title, "Effect Mix");
            strcpy(info.shortTitle, "Mix");
            strcpy(info.units, "%");
            info.defaultNormalizedValue = 0.5;
            break;
        case kUSBAutoDetect:
            strcpy(info.title, "USB Auto-Detect");
            strcpy(info.shortTitle, "Auto USB");
            strcpy(info.units, "");
            info.stepCount = 1; // On/Off
            info.flags |= Steinberg::Vst::ParameterInfo::kIsBypass;
            info.defaultNormalizedValue = 1.0;
            break;
        case kLowLatencyMode:
            strcpy(info.title, "Low-Latency Mode");
            strcpy(info.shortTitle, "Low Lat");
            strcpy(info.units, "");
            info.stepCount = 1; // On/Off
            info.flags |= Steinberg::Vst::ParameterInfo::kIsBypass;
            info.defaultNormalizedValue = 1.0;
            break;
        case kRecordEnable:
            strcpy(info.title, "Record Enable");
            strcpy(info.shortTitle, "Record");
            strcpy(info.units, "");
            info.stepCount = 1; // On/Off
            info.flags |= Steinberg::Vst::ParameterInfo::kIsBypass;
            info.defaultNormalizedValue = 0.0;
            break;
    }
    
    return Steinberg::kResultOk;
}

Steinberg::tresult WarriorUSBRecorderProcessor::setParamNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) {
    if (id < 0 || id >= kNumParams) {
        return Steinberg::kResultFalse;
    }
    
    paramValues[id] = static_cast<float>(value);
    
    // Handle parameter-specific logic
    switch (id) {
        case kGenreSelect:
            if (effectsEngine) {
                int genreIndex = static_cast<int>(value * 11.0 + 0.5);
                GenreType genre = static_cast<GenreType>(genreIndex);
                effectsEngine->setGenre(genre);
                std::cout << "Genre changed to: " << effectsEngine->getGenreName(genre) << std::endl;
            }
            break;
            
        case kEffectMix:
            if (effectsEngine) {
                effectsEngine->setDryWetMix(static_cast<float>(value));
            }
            break;
            
        case kUSBAutoDetect:
            if (usbDetector) {
                if (value > 0.5) {
                    usbDetector->startDeviceMonitoring();
                } else {
                    usbDetector->stopDeviceMonitoring();
                }
            }
            break;
            
        case kLowLatencyMode:
            if (latencyProcessor) {
                latencyProcessor->enableRealTimeProcessing(value > 0.5);
            }
            break;
            
        case kRecordEnable:
            isRecording = (value > 0.5);
            std::cout << "Recording " << (isRecording ? "enabled" : "disabled") << std::endl;
            break;
    }
    
    return Steinberg::kResultOk;
}

Steinberg::Vst::ParamValue WarriorUSBRecorderProcessor::getParamNormalized(Steinberg::Vst::ParamID id) {
    if (id < 0 || id >= kNumParams) {
        return 0.0;
    }
    
    return paramValues[id];
}

} // namespace Warrior