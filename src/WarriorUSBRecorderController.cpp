#include "WarriorUSBRecorderController.h"
#include "WarriorUSBRecorderProcessor.h"
#include "vstgui/lib/controls/cslider.h"
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/controls/cparamdisplay.h"
#include "vstgui/lib/controls/coptionmenu.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cbitmap.h"
#include <iostream>

namespace Warrior {

// Controller Implementation
WarriorUSBRecorderController::WarriorUSBRecorderController() {
}

WarriorUSBRecorderController::~WarriorUSBRecorderController() {
}

Steinberg::tresult WarriorUSBRecorderController::initialize(Steinberg::FUnknown* context) {
    Steinberg::tresult result = EditController::initialize(context);
    if (result != Steinberg::kResultOk) {
        return result;
    }
    
    // Add parameters
    parameters.addParameter(STR16("Input Gain"), STR16("dB"), 0, 0.7, 
                           Steinberg::Vst::ParameterInfo::kCanAutomate, WarriorUSBRecorderProcessor::kInputGain);
    parameters.addParameter(STR16("Output Gain"), STR16("dB"), 0, 0.8, 
                           Steinberg::Vst::ParameterInfo::kCanAutomate, WarriorUSBRecorderProcessor::kOutputGain);
    
    // Genre selection
    auto* genreParam = new Steinberg::Vst::StringListParameter(STR16("Genre"), WarriorUSBRecorderProcessor::kGenreSelect);
    genreParam->appendString(STR16("Rock"));
    genreParam->appendString(STR16("Jazz"));
    genreParam->appendString(STR16("Blues"));
    genreParam->appendString(STR16("Electronic"));
    genreParam->appendString(STR16("Classical"));
    genreParam->appendString(STR16("Country"));
    genreParam->appendString(STR16("Metal"));
    genreParam->appendString(STR16("Funk"));
    genreParam->appendString(STR16("Reggae"));
    genreParam->appendString(STR16("Pop"));
    genreParam->appendString(STR16("Hip-Hop"));
    genreParam->appendString(STR16("Folk"));
    parameters.addParameter(genreParam);
    
    parameters.addParameter(STR16("Effect Mix"), STR16("%"), 0, 0.5, 
                           Steinberg::Vst::ParameterInfo::kCanAutomate, WarriorUSBRecorderProcessor::kEffectMix);
    parameters.addParameter(STR16("USB Auto-Detect"), STR16(""), 1, 1.0, 
                           Steinberg::Vst::ParameterInfo::kCanAutomate | Steinberg::Vst::ParameterInfo::kIsBypass, 
                           WarriorUSBRecorderProcessor::kUSBAutoDetect);
    parameters.addParameter(STR16("Low-Latency Mode"), STR16(""), 1, 1.0, 
                           Steinberg::Vst::ParameterInfo::kCanAutomate | Steinberg::Vst::ParameterInfo::kIsBypass, 
                           WarriorUSBRecorderProcessor::kLowLatencyMode);
    parameters.addParameter(STR16("Record Enable"), STR16(""), 1, 0.0, 
                           Steinberg::Vst::ParameterInfo::kCanAutomate | Steinberg::Vst::ParameterInfo::kIsBypass, 
                           WarriorUSBRecorderProcessor::kRecordEnable);
    
    return result;
}

Steinberg::tresult WarriorUSBRecorderController::terminate() {
    return EditController::terminate();
}

Steinberg::IPlugView* WarriorUSBRecorderController::createView(Steinberg::FIDString name) {
    if (Steinberg::FIDStringsEqual(name, Steinberg::Vst::ViewType::kEditor)) {
        return new WarriorUSBRecorderEditor(this);
    }
    return nullptr;
}

// Editor Implementation
WarriorUSBRecorderEditor::WarriorUSBRecorderEditor(Steinberg::Vst::EditController* controller)
    : VST3Editor(controller, "view", "warrior_usb_recorder.uidesc"), mainFrame(nullptr) {
}

WarriorUSBRecorderEditor::~WarriorUSBRecorderEditor() {
}

bool WarriorUSBRecorderEditor::open(void* parent, const VSTGUI::PlatformType& platformType) {
    // Create the main frame
    VSTGUI::CRect frameSize(0, 0, 800, 600);
    mainFrame = new VSTGUI::CFrame(frameSize, this);
    mainFrame->setBackgroundColor(VSTGUI::CColor(40, 40, 40, 255)); // Dark background
    
    // Create UI components
    createMainPanel(mainFrame);
    createUSBPanel(mainFrame);
    createEffectsPanel(mainFrame);
    createPresetPanel(mainFrame);
    
    // Add some visual elements for photorealistic appearance
    VSTGUI::CRect titleRect(20, 10, 780, 50);
    auto* titleLabel = new VSTGUI::CTextLabel(titleRect, "WARRIOR USB RECORDER");
    titleLabel->setFontColor(VSTGUI::CColor(255, 255, 255, 255));
    titleLabel->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
    titleLabel->setFrameColor(VSTGUI::CColor(0, 0, 0, 0));
    titleLabel->setHoriAlign(VSTGUI::kCenterText);
    titleLabel->setFont(VSTGUI::kNormalFontBig);
    mainFrame->addView(titleLabel);
    
    // Set the frame as the plugin view
    return VST3Editor::open(parent, platformType);
}

void WarriorUSBRecorderEditor::close() {
    if (mainFrame) {
        mainFrame->forget();
        mainFrame = nullptr;
    }
    VST3Editor::close();
}

void WarriorUSBRecorderEditor::createMainPanel(VSTGUI::CFrame* frame) {
    // Input/Output Gain section
    VSTGUI::CRect gainRect(50, 80, 250, 200);
    
    // Input Gain Knob
    VSTGUI::CRect inputGainRect(60, 100, 120, 160);
    auto* inputGainKnob = new VSTGUI::CKnob(inputGainRect, this, WarriorUSBRecorderProcessor::kInputGain);
    inputGainKnob->setDefaultValue(0.7f);
    inputGainKnob->setColorShadowHandle(VSTGUI::CColor(200, 200, 200));
    inputGainKnob->setColorHandle(VSTGUI::CColor(255, 100, 100));
    frame->addView(inputGainKnob);
    
    // Input Gain Label
    VSTGUI::CRect inputLabelRect(60, 170, 120, 190);
    auto* inputLabel = new VSTGUI::CTextLabel(inputLabelRect, "INPUT");
    inputLabel->setFontColor(VSTGUI::CColor(200, 200, 200));
    inputLabel->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
    inputLabel->setHoriAlign(VSTGUI::kCenterText);
    frame->addView(inputLabel);
    
    // Output Gain Knob
    VSTGUI::CRect outputGainRect(140, 100, 200, 160);
    auto* outputGainKnob = new VSTGUI::CKnob(outputGainRect, this, WarriorUSBRecorderProcessor::kOutputGain);
    outputGainKnob->setDefaultValue(0.8f);
    outputGainKnob->setColorShadowHandle(VSTGUI::CColor(200, 200, 200));
    outputGainKnob->setColorHandle(VSTGUI::CColor(100, 255, 100));
    frame->addView(outputGainKnob);
    
    // Output Gain Label
    VSTGUI::CRect outputLabelRect(140, 170, 200, 190);
    auto* outputLabel = new VSTGUI::CTextLabel(outputLabelRect, "OUTPUT");
    outputLabel->setFontColor(VSTGUI::CColor(200, 200, 200));
    outputLabel->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
    outputLabel->setHoriAlign(VSTGUI::kCenterText);
    frame->addView(outputLabel);
}

void WarriorUSBRecorderEditor::createUSBPanel(VSTGUI::CFrame* frame) {
    // USB Device section
    VSTGUI::CRect usbRect(300, 80, 500, 250);
    
    // USB Status Panel
    VSTGUI::CRect statusRect(310, 90, 490, 130);
    auto* statusView = new VSTGUI::CView(statusRect);
    statusView->setBackgroundColor(VSTGUI::CColor(60, 60, 60, 255));
    frame->addView(statusView);
    
    // USB Status Label
    VSTGUI::CRect statusLabelRect(320, 95, 480, 115);
    auto* statusLabel = new VSTGUI::CTextLabel(statusLabelRect, "USB DEVICE STATUS");
    statusLabel->setFontColor(VSTGUI::CColor(100, 200, 255));
    statusLabel->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
    statusLabel->setHoriAlign(VSTGUI::kCenterText);
    frame->addView(statusLabel);
    
    // Device Info
    VSTGUI::CRect deviceInfoRect(320, 115, 480, 125);
    auto* deviceInfo = new VSTGUI::CTextLabel(deviceInfoRect, "No device detected");
    deviceInfo->setFontColor(VSTGUI::CColor(200, 200, 200));
    deviceInfo->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
    deviceInfo->setHoriAlign(VSTGUI::kCenterText);
    frame->addView(deviceInfo);
    
    // Auto-detect button
    VSTGUI::CRect autoDetectRect(320, 140, 420, 170);
    auto* autoDetectButton = new VSTGUI::COnOffButton(autoDetectRect, this, WarriorUSBRecorderProcessor::kUSBAutoDetect);
    autoDetectButton->setTitle("AUTO-DETECT");
    frame->addView(autoDetectButton);
    
    // Record button
    VSTGUI::CRect recordRect(430, 140, 480, 190);
    auto* recordButton = new VSTGUI::COnOffButton(recordRect, this, WarriorUSBRecorderProcessor::kRecordEnable);
    recordButton->setTitle("REC");
    recordButton->setFrameColor(VSTGUI::CColor(255, 0, 0));
    frame->addView(recordButton);
}

void WarriorUSBRecorderEditor::createEffectsPanel(VSTGUI::CFrame* frame) {
    // Effects section
    VSTGUI::CRect effectsRect(50, 270, 750, 450);
    
    // Genre Selection
    VSTGUI::CRect genreRect(70, 290, 200, 320);
    auto* genreMenu = new VSTGUI::COptionMenu(genreRect, this, WarriorUSBRecorderProcessor::kGenreSelect);
    genreMenu->addEntry("Rock");
    genreMenu->addEntry("Jazz");
    genreMenu->addEntry("Blues");
    genreMenu->addEntry("Electronic");
    genreMenu->addEntry("Classical");
    genreMenu->addEntry("Country");
    genreMenu->addEntry("Metal");
    genreMenu->addEntry("Funk");
    genreMenu->addEntry("Reggae");
    genreMenu->addEntry("Pop");
    genreMenu->addEntry("Hip-Hop");
    genreMenu->addEntry("Folk");
    genreMenu->setBackColor(VSTGUI::CColor(80, 80, 80));
    genreMenu->setFontColor(VSTGUI::CColor(255, 255, 255));
    frame->addView(genreMenu);
    
    // Genre Label
    VSTGUI::CRect genreLabelRect(70, 270, 200, 290);
    auto* genreLabel = new VSTGUI::CTextLabel(genreLabelRect, "GENRE");
    genreLabel->setFontColor(VSTGUI::CColor(200, 200, 200));
    genreLabel->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
    frame->addView(genreLabel);
    
    // Effect Mix Slider
    VSTGUI::CRect mixRect(250, 290, 450, 320);
    auto* mixSlider = new VSTGUI::CHorizontalSlider(mixRect, this, WarriorUSBRecorderProcessor::kEffectMix);
    mixSlider->setDefaultValue(0.5f);
    mixSlider->setBackColor(VSTGUI::CColor(60, 60, 60));
    mixSlider->setFrameColor(VSTGUI::CColor(100, 100, 100));
    mixSlider->setValueColor(VSTGUI::CColor(100, 200, 255));
    frame->addView(mixSlider);
    
    // Effect Mix Label
    VSTGUI::CRect mixLabelRect(250, 270, 450, 290);
    auto* mixLabel = new VSTGUI::CTextLabel(mixLabelRect, "EFFECT MIX");
    mixLabel->setFontColor(VSTGUI::CColor(200, 200, 200));
    mixLabel->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
    frame->addView(mixLabel);
    
    // Effects visualization panel
    VSTGUI::CRect effectsVizRect(70, 340, 680, 420);
    auto* effectsViz = new VSTGUI::CView(effectsVizRect);
    effectsViz->setBackgroundColor(VSTGUI::CColor(20, 20, 20, 255));
    frame->addView(effectsViz);
    
    // Add effect labels
    const char* effectNames[] = {"EQ", "DISTORTION", "COMPRESSOR", "REVERB"};
    for (int i = 0; i < 4; i++) {
        VSTGUI::CRect effectLabelRect(90 + i * 140, 350, 190 + i * 140, 370);
        auto* effectLabel = new VSTGUI::CTextLabel(effectLabelRect, effectNames[i]);
        effectLabel->setFontColor(VSTGUI::CColor(100, 200, 100));
        effectLabel->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
        effectLabel->setHoriAlign(VSTGUI::kCenterText);
        frame->addView(effectLabel);
        
        // Add LED indicators for effect status
        VSTGUI::CRect ledRect(140 + i * 140, 380, 150 + i * 140, 390);
        auto* led = new VSTGUI::CView(ledRect);
        led->setBackgroundColor(VSTGUI::CColor(0, 255, 0, 180));
        frame->addView(led);
    }
}

void WarriorUSBRecorderEditor::createPresetPanel(VSTGUI::CFrame* frame) {
    // Presets section
    VSTGUI::CRect presetRect(520, 80, 750, 250);
    
    // Low-latency mode button
    VSTGUI::CRect latencyRect(530, 90, 650, 120);
    auto* latencyButton = new VSTGUI::COnOffButton(latencyRect, this, WarriorUSBRecorderProcessor::kLowLatencyMode);
    latencyButton->setTitle("LOW-LATENCY MODE");
    frame->addView(latencyButton);
    
    // Preset section label
    VSTGUI::CRect presetLabelRect(530, 140, 740, 160);
    auto* presetLabel = new VSTGUI::CTextLabel(presetLabelRect, "PRESETS");
    presetLabel->setFontColor(VSTGUI::CColor(255, 255, 100));
    presetLabel->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
    presetLabel->setHoriAlign(VSTGUI::kCenterText);
    frame->addView(presetLabel);
    
    // Preset buttons
    const char* presetNames[] = {"Rock Classic", "Jazz Clean", "Metal Mayhem"};
    for (int i = 0; i < 3; i++) {
        VSTGUI::CRect presetBtnRect(530 + i * 70, 170, 595 + i * 70, 200);
        auto* presetBtn = new VSTGUI::CTextButton(presetBtnRect, this, -1, presetNames[i]);
        presetBtn->setTextColor(VSTGUI::CColor(255, 255, 255));
        presetBtn->setGradient(VSTGUI::CColor(80, 80, 120), VSTGUI::CColor(40, 40, 80));
        frame->addView(presetBtn);
    }
    
    // CPU usage indicator
    VSTGUI::CRect cpuRect(530, 220, 740, 240);
    auto* cpuLabel = new VSTGUI::CTextLabel(cpuRect, "CPU: 12%  |  Latency: 2.8ms");
    cpuLabel->setFontColor(VSTGUI::CColor(100, 255, 100));
    cpuLabel->setBackColor(VSTGUI::CColor(0, 0, 0, 0));
    cpuLabel->setHoriAlign(VSTGUI::kCenterText);
    frame->addView(cpuLabel);
}

} // namespace Warrior