#pragma once

#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"

namespace Warrior {

class WarriorUSBRecorderController : public Steinberg::Vst::EditController
{
public:
    WarriorUSBRecorderController();
    virtual ~WarriorUSBRecorderController();

    // From IEditController
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    
    // From EditController
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;
    
    // Create function
    static Steinberg::FUnknown* createInstance(void* /*context*/) {
        return (Steinberg::Vst::IEditController*)new WarriorUSBRecorderController;
    }
};

class WarriorUSBRecorderEditor : public VSTGUI::VST3Editor
{
public:
    WarriorUSBRecorderEditor(Steinberg::Vst::EditController* controller);
    virtual ~WarriorUSBRecorderEditor();

protected:
    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType = VSTGUI::kDefaultNative) SMTG_OVERRIDE;
    void PLUGIN_API close() SMTG_OVERRIDE;
    
    // Create the UI
    VSTGUI::CFrame* createUI();
    
private:
    void createMainPanel(VSTGUI::CFrame* frame);
    void createUSBPanel(VSTGUI::CFrame* frame);
    void createEffectsPanel(VSTGUI::CFrame* frame);
    void createPresetPanel(VSTGUI::CFrame* frame);
    
    VSTGUI::CFrame* mainFrame;
};

} // namespace Warrior