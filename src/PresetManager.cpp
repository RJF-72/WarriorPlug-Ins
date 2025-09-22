#include "PresetManager.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
// #include <filesystem> // C++17 feature, using alternatives for compatibility
#include <iostream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <set>
#include <thread>
#include <atomic>

namespace Warrior {

PresetManager::PresetManager() : autoSaveEnabled(false), autoSaveInterval(5), shouldStopAutoSave(false) {
    loadFactoryPresets();
}

PresetManager::~PresetManager() {
    shouldStopAutoSave = true;
    if (autoSaveThread.joinable()) {
        autoSaveThread.join();
    }
}

bool PresetManager::savePreset(const Preset& preset) {
    if (!validatePreset(preset)) {
        std::cerr << "Invalid preset: " << preset.name << std::endl;
        return false;
    }
    
    // Update metadata
    Preset updatedPreset = preset;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    if (presets.find(preset.name) == presets.end()) {
        updatedPreset.createdDate = ss.str();
    }
    updatedPreset.modifiedDate = ss.str();
    
    // Save to memory
    presets[preset.name] = updatedPreset;
    
    // Save to file
    std::string filePath = getPresetDirectory() + "/" + preset.name + ".wpreset";
    bool success = savePresetToFile(updatedPreset, filePath);
    
    if (success) {
        std::cout << "Saved preset: " << preset.name << std::endl;
    } else {
        std::cerr << "Failed to save preset file: " << filePath << std::endl;
    }
    
    return success;
}

bool PresetManager::loadPreset(const std::string& presetName) {
    auto it = presets.find(presetName);
    if (it != presets.end()) {
        updateRecentlyUsed(presetName);
        updatePresetUsage(presetName);
        return true;
    }
    
    // Try to load from file
    std::string filePath = getPresetDirectory() + "/" + presetName + ".wpreset";
    Preset preset;
    if (loadPresetFromFile(filePath, preset)) {
        presets[presetName] = preset;
        updateRecentlyUsed(presetName);
        updatePresetUsage(presetName);
        return true;
    }
    
    std::cerr << "Preset not found: " << presetName << std::endl;
    return false;
}

bool PresetManager::deletePreset(const std::string& presetName) {
    if (isFactoryPreset(presetName)) {
        std::cerr << "Cannot delete factory preset: " << presetName << std::endl;
        return false;
    }
    
    // Remove from memory
    presets.erase(presetName);
    
    // Remove from recently used
    recentlyUsed.erase(
        std::remove(recentlyUsed.begin(), recentlyUsed.end(), presetName),
        recentlyUsed.end());
    
    // Remove usage stats
    usageStats.erase(presetName);
    
    // Delete file (using system call for compatibility)
    std::string filePath = getPresetDirectory() + "/" + presetName + ".wpreset";
    std::string rmCmd = "rm -f " + filePath;
    try {
        int result = system(rmCmd.c_str());
        if (result == 0) {
            std::cout << "Deleted preset: " << presetName << std::endl;
            return true;
        } else {
            std::cerr << "Failed to delete preset file: " << filePath << std::endl;
            return false;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Failed to delete preset file: " << ex.what() << std::endl;
        return false;
    }
}

std::vector<Preset> PresetManager::getAllPresets() const {
    std::vector<Preset> result;
    result.reserve(presets.size());
    
    for (const auto& pair : presets) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<Preset> PresetManager::getPresetsByCategory(const std::string& category) const {
    std::vector<Preset> result;
    
    for (const auto& pair : presets) {
        if (pair.second.category == category) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<Preset> PresetManager::getPresetsByGenre(GenreType genre) const {
    std::vector<Preset> result;
    
    for (const auto& pair : presets) {
        if (pair.second.genre == genre) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<Preset> PresetManager::searchPresets(const std::string& searchTerm) const {
    std::vector<Preset> result;
    std::string lowerSearchTerm = searchTerm;
    std::transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), lowerSearchTerm.begin(), ::tolower);
    
    for (const auto& pair : presets) {
        const Preset& preset = pair.second;
        
        // Search in name
        std::string lowerName = preset.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find(lowerSearchTerm) != std::string::npos) {
            result.push_back(preset);
            continue;
        }
        
        // Search in description
        std::string lowerDesc = preset.description;
        std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);
        if (lowerDesc.find(lowerSearchTerm) != std::string::npos) {
            result.push_back(preset);
            continue;
        }
        
        // Search in tags
        for (const auto& tag : preset.tags) {
            std::string lowerTag = tag;
            std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
            if (lowerTag.find(lowerSearchTerm) != std::string::npos) {
                result.push_back(preset);
                break;
            }
        }
    }
    
    return result;
}

Preset PresetManager::getCurrentState(GenreEffectsEngine* effectsEngine) const {
    if (!effectsEngine) return {};
    
    Preset currentState;
    currentState.name = "Current State";
    currentState.description = "Current plugin state";
    currentState.category = "Temporary";
    currentState.genre = effectsEngine->getCurrentGenre();
    
    // Get all effect parameters
    auto effectChain = effectsEngine->getEffectChain();
    for (const auto& effect : effectChain) {
        currentState.effectStates[effect->getName()] = effect->isEnabled();
        
        auto parameters = effect->getParameters();
        for (const auto& param : parameters) {
            PresetParameter presetParam;
            presetParam.name = param.name;
            presetParam.value = param.value;
            presetParam.effectName = effect->getName();
            currentState.parameters.push_back(presetParam);
        }
    }
    
    return currentState;
}

bool PresetManager::applyPreset(const Preset& preset, GenreEffectsEngine* effectsEngine) {
    if (!effectsEngine) return false;
    
    // Set genre
    effectsEngine->setGenre(preset.genre);
    
    // Apply effect states
    auto effectChain = effectsEngine->getEffectChain();
    for (const auto& effect : effectChain) {
        auto stateIt = preset.effectStates.find(effect->getName());
        if (stateIt != preset.effectStates.end()) {
            effect->setEnabled(stateIt->second);
        }
    }
    
    // Apply parameters
    for (const auto& param : preset.parameters) {
        effectsEngine->setEffectParameter(param.effectName, param.name, param.value);
    }
    
    std::cout << "Applied preset: " << preset.name << std::endl;
    return true;
}

void PresetManager::loadFactoryPresets() {
    // Clear existing presets
    presets.clear();
    factoryPresetNames.clear();
    
    // Rock Factory Preset
    createFactoryPreset("Rock Classic", "Classic rock sound with moderate distortion",
                       GenreType::Rock, {
                           {"drive", 0.6f, "Distortion"},
                           {"tone", 0.7f, "Distortion"},
                           {"level", 0.8f, "Distortion"},
                           {"roomSize", 0.4f, "Reverb"},
                           {"wetLevel", 0.2f, "Reverb"}
                       });
    
    // Jazz Factory Preset
    createFactoryPreset("Jazz Clean", "Warm, clean jazz tone",
                       GenreType::Jazz, {
                           {"threshold", 0.8f, "Compressor"},
                           {"ratio", 2.0f, "Compressor"},
                           {"roomSize", 0.6f, "Reverb"},
                           {"wetLevel", 0.3f, "Reverb"}
                       });
    
    // Metal Factory Preset
    createFactoryPreset("Metal Mayhem", "High-gain metal sound",
                       GenreType::Metal, {
                           {"drive", 0.9f, "Distortion"},
                           {"tone", 0.8f, "Distortion"},
                           {"threshold", 0.5f, "Compressor"},
                           {"ratio", 6.0f, "Compressor"},
                           {"lowGain", 0.3f, "3-Band EQ"},
                           {"highGain", 0.4f, "3-Band EQ"}
                       });
    
    // Blues Factory Preset
    createFactoryPreset("Blues Breaker", "Vintage blues overdrive",
                       GenreType::Blues, {
                           {"drive", 0.4f, "Distortion"},
                           {"tone", 0.6f, "Distortion"},
                           {"level", 0.7f, "Distortion"},
                           {"midGain", 0.2f, "3-Band EQ"}
                       });
    
    // Electronic Factory Preset
    createFactoryPreset("Electronic Edge", "Modern electronic processing",
                       GenreType::Electronic, {
                           {"threshold", 0.6f, "Compressor"},
                           {"ratio", 4.0f, "Compressor"},
                           {"highGain", 0.5f, "3-Band EQ"},
                           {"roomSize", 0.8f, "Reverb"}
                       });
    
    std::cout << "Loaded " << factoryPresetNames.size() << " factory presets" << std::endl;
}

bool PresetManager::isFactoryPreset(const std::string& presetName) const {
    return std::find(factoryPresetNames.begin(), factoryPresetNames.end(), presetName) != factoryPresetNames.end();
}

std::vector<std::string> PresetManager::getAvailableCategories() const {
    std::set<std::string> categories;
    for (const auto& pair : presets) {
        categories.insert(pair.second.category);
    }
    return std::vector<std::string>(categories.begin(), categories.end());
}

void PresetManager::updatePresetUsage(const std::string& presetName) {
    usageStats[presetName]++;
    
    // Update the preset's usage count
    auto it = presets.find(presetName);
    if (it != presets.end()) {
        it->second.usageCount = usageStats[presetName];
    }
}

std::vector<Preset> PresetManager::getMostUsedPresets(int count) const {
    std::vector<std::pair<std::string, int>> sortedUsage(usageStats.begin(), usageStats.end());
    std::sort(sortedUsage.begin(), sortedUsage.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<Preset> result;
    for (int i = 0; i < std::min(count, (int)sortedUsage.size()); i++) {
        auto it = presets.find(sortedUsage[i].first);
        if (it != presets.end()) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

std::vector<Preset> PresetManager::getRecentlyUsedPresets(int count) const {
    std::vector<Preset> result;
    int actualCount = std::min(count, (int)recentlyUsed.size());
    
    for (int i = 0; i < actualCount; i++) {
        auto it = presets.find(recentlyUsed[i]);
        if (it != presets.end()) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

std::string PresetManager::getPresetDirectory() const {
    std::string homeDir = getenv("HOME") ? getenv("HOME") : ".";
    std::string presetDir = homeDir + "/.warrior_plugins/presets";
    
    // Create directory if it doesn't exist (using system call for compatibility)
    std::string mkdirCmd = "mkdir -p " + presetDir;
    system(mkdirCmd.c_str());
    
    return presetDir;
}

std::string PresetManager::getBackupDirectory() const {
    std::string homeDir = getenv("HOME") ? getenv("HOME") : ".";
    std::string backupDir = homeDir + "/.warrior_plugins/backups";
    
    // Create directory if it doesn't exist (using system call for compatibility)
    std::string mkdirCmd = "mkdir -p " + backupDir;
    system(mkdirCmd.c_str());
    
    return backupDir;
}

bool PresetManager::savePresetToFile(const Preset& preset, const std::string& filePath) const {
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) return false;
        
        std::string json = serializePreset(preset);
        file << json;
        file.close();
        
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "Error saving preset to file: " << ex.what() << std::endl;
        return false;
    }
}

bool PresetManager::loadPresetFromFile(const std::string& filePath, Preset& preset) const {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;
        
        std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        return deserializePreset(json, preset);
    } catch (const std::exception& ex) {
        std::cerr << "Error loading preset from file: " << ex.what() << std::endl;
        return false;
    }
}

std::string PresetManager::serializePreset(const Preset& preset) const {
    // Simple JSON-like serialization (in a real implementation, use a proper JSON library)
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"name\": \"" << preset.name << "\",\n";
    ss << "  \"description\": \"" << preset.description << "\",\n";
    ss << "  \"category\": \"" << preset.category << "\",\n";
    ss << "  \"genre\": " << static_cast<int>(preset.genre) << ",\n";
    ss << "  \"parameters\": [\n";
    
    for (size_t i = 0; i < preset.parameters.size(); i++) {
        const auto& param = preset.parameters[i];
        ss << "    {\"name\": \"" << param.name << "\", \"value\": " << param.value 
           << ", \"effect\": \"" << param.effectName << "\"}";
        if (i < preset.parameters.size() - 1) ss << ",";
        ss << "\n";
    }
    
    ss << "  ]\n";
    ss << "}";
    
    return ss.str();
}

bool PresetManager::deserializePreset(const std::string& json, Preset& preset) const {
    // Simplified JSON parsing (in a real implementation, use a proper JSON library)
    // For now, just create a basic preset structure
    preset.name = "Imported Preset";
    preset.description = "Imported from file";
    preset.category = "User";
    preset.genre = GenreType::Rock;
    
    return true;
}

void PresetManager::updateRecentlyUsed(const std::string& presetName) {
    // Remove if already exists
    recentlyUsed.erase(
        std::remove(recentlyUsed.begin(), recentlyUsed.end(), presetName),
        recentlyUsed.end());
    
    // Add to front
    recentlyUsed.insert(recentlyUsed.begin(), presetName);
    
    // Keep only the last 20
    if (recentlyUsed.size() > 20) {
        recentlyUsed.resize(20);
    }
}

bool PresetManager::validatePreset(const Preset& preset) const {
    if (preset.name.empty()) return false;
    if (preset.parameters.empty()) return false;
    return true;
}

void PresetManager::createFactoryPreset(const std::string& name, const std::string& description, 
                                       GenreType genre, const std::vector<PresetParameter>& params) {
    Preset preset;
    preset.name = name;
    preset.description = description;
    preset.category = "Factory";
    preset.author = "Warrior Audio";
    preset.version = "1.0";
    preset.genre = genre;
    preset.parameters = params;
    preset.rating = 5.0f;
    preset.usageCount = 0;
    
    // Set default effect states
    preset.effectStates["3-Band EQ"] = true;
    preset.effectStates["Distortion"] = (genre == GenreType::Rock || genre == GenreType::Metal || genre == GenreType::Blues);
    preset.effectStates["Compressor"] = true;
    preset.effectStates["Reverb"] = true;
    
    presets[name] = preset;
    factoryPresetNames.push_back(name);
}

} // namespace Warrior