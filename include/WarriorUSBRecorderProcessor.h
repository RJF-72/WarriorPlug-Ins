#pragma once

#include "pluginterfaces/base/fplatform.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "USBAudioDetector.h"
#include "GenreEffectsEngine.h"
#include "LowLatencyProcessor.h"
#include "PresetManager.h"

namespace Warrior {

class WarriorUSBRecorderProcessor : public Steinberg::Vst::AudioEffect
{
public:
    WarriorUSBRecorderProcessor();
    virtual ~WarriorUSBRecorderProcessor();

    // From IComponent
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;

    // From IAudioProcessor
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API getLatencySamples() SMTG_OVERRIDE;

    // From IEditController (for parameter handling)
    Steinberg::tresult PLUGIN_API getParameterInfo(Steinberg::int32 paramIndex, Steinberg::Vst::ParameterInfo& info) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
    Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized(Steinberg::Vst::ParamID id) SMTG_OVERRIDE;

    // Create function
    static Steinberg::FUnknown* createInstance(void* /*context*/) {
        return (Steinberg::Vst::IAudioProcessor*)new WarriorUSBRecorderProcessor;
    }

private:
    // Core components
    std::unique_ptr<USBAudioDetector> usbDetector;
    std::unique_ptr<GenreEffectsEngine> effectsEngine;
    std::unique_ptr<LowLatencyProcessor> latencyProcessor;
    std::unique_ptr<PresetManager> presetManager;

    // Processing parameters
    double sampleRate;
    int32 maxSamplesPerBlock;
    bool isRecording;
    
    // Plugin parameters
    enum Parameters {
        kInputGain = 0,
        kOutputGain,
        kGenreSelect,
        kEffectMix,
        kUSBAutoDetect,
        kLowLatencyMode,
        kRecordEnable,
        kNumParams
    };
    
    float paramValues[kNumParams];
};

} // namespace Warrior