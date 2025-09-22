#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include "GenreEffectsEngine.h"

namespace Warrior {

struct PresetParameter {
    std::string name;
    float value;
    std::string effectName;
};

struct Preset {
    std::string name;
    std::string description;
    std::string category;
    std::string author;
    std::string version;
    GenreType genre;
    std::vector<PresetParameter> parameters;
    std::map<std::string, bool> effectStates; // effect name -> enabled state
    
    // Metadata
    std::string createdDate;
    std::string modifiedDate;
    std::vector<std::string> tags;
    float rating;
    int usageCount;
};

class PresetManager {
public:
    PresetManager();
    ~PresetManager();
    
    // Preset management
    bool savePreset(const Preset& preset);
    bool loadPreset(const std::string& presetName);
    bool deletePreset(const std::string& presetName);
    bool renamePreset(const std::string& oldName, const std::string& newName);
    
    // Preset discovery
    std::vector<Preset> getAllPresets() const;
    std::vector<Preset> getPresetsByCategory(const std::string& category) const;
    std::vector<Preset> getPresetsByGenre(GenreType genre) const;
    std::vector<Preset> searchPresets(const std::string& searchTerm) const;
    
    // Current state management
    Preset getCurrentState(GenreEffectsEngine* effectsEngine) const;
    bool applyPreset(const Preset& preset, GenreEffectsEngine* effectsEngine);
    
    // Factory presets
    void loadFactoryPresets();
    bool isFactoryPreset(const std::string& presetName) const;
    
    // Import/Export
    bool exportPreset(const std::string& presetName, const std::string& filePath) const;
    bool importPreset(const std::string& filePath);
    bool exportPresetPack(const std::vector<std::string>& presetNames, const std::string& filePath) const;
    bool importPresetPack(const std::string& filePath);
    
    // Categories
    std::vector<std::string> getAvailableCategories() const;
    void addCategory(const std::string& category);
    
    // Auto-save and backup
    void enableAutoSave(bool enable, int intervalMinutes = 5);
    void createBackup();
    std::vector<std::string> getAvailableBackups() const;
    bool restoreFromBackup(const std::string& backupName);
    
    // Statistics
    void updatePresetUsage(const std::string& presetName);
    std::vector<Preset> getMostUsedPresets(int count = 10) const;
    std::vector<Preset> getRecentlyUsedPresets(int count = 10) const;

private:
    std::map<std::string, Preset> presets;
    std::vector<std::string> factoryPresetNames;
    std::vector<std::string> recentlyUsed;
    std::map<std::string, int> usageStats;
    
    // Auto-save
    bool autoSaveEnabled;
    int autoSaveInterval;
    std::thread autoSaveThread;
    std::atomic<bool> shouldStopAutoSave;
    
    // File operations
    std::string getPresetDirectory() const;
    std::string getBackupDirectory() const;
    bool savePresetToFile(const Preset& preset, const std::string& filePath) const;
    bool loadPresetFromFile(const std::string& filePath, Preset& preset) const;
    
    // JSON serialization
    std::string serializePreset(const Preset& preset) const;
    bool deserializePreset(const std::string& json, Preset& preset) const;
    
    // Internal helpers
    void autoSaveThreadFunction();
    void updateRecentlyUsed(const std::string& presetName);
    bool validatePreset(const Preset& preset) const;
    std::string generateUniquePresetName(const std::string& baseName) const;
    
    // Factory preset creation
    void createFactoryPreset(const std::string& name, const std::string& description, 
                            GenreType genre, const std::vector<PresetParameter>& params);
};

} // namespace Warrior