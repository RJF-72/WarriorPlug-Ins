#include "public.sdk/source/main/pluginfactory.h"
#include "WarriorUSBRecorderProcessor.h"
#include "WarriorUSBRecorderController.h"
#include "pluginterfaces/vst/vsttypes.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace Warrior;

// Plugin Factory
static const FUID ProcessorUID(0x12345678, 0x12345678, 0x12345678, 0x12345678);
static const FUID ControllerUID(0x87654321, 0x87654321, 0x87654321, 0x87654321);

BEGIN_FACTORY_DEF("Warrior Audio", "https://www.warrior-audio.com", "warrior@warrior-audio.com")
    DEF_CLASS2(INLINE_UID_FROM_FUID(ProcessorUID),
               PClassInfo::kManyInstances,
               kVstAudioEffectClass,
               "Warrior USB Recorder",
               Vst::kDistributable,
               Vst::PlugType::kInstrument,
               "1.0.0",
               kVstVersionString,
               WarriorUSBRecorderProcessor::createInstance)

    DEF_CLASS2(INLINE_UID_FROM_FUID(ControllerUID),
               PClassInfo::kManyInstances,
               kVstComponentControllerClass,
               "Warrior USB Recorder Controller",
               0,
               "",
               "1.0.0",
               kVstVersionString,
               WarriorUSBRecorderController::createInstance)
END_FACTORY