#pragma once

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace Warrior {

class RingBuffer {
public:
    RingBuffer(size_t size);
    ~RingBuffer();
    
    size_t write(const float* data, size_t samples);
    size_t read(float* data, size_t samples);
    size_t available() const;
    size_t space() const;
    void reset();

private:
    std::unique_ptr<float[]> buffer;
    size_t size;
    std::atomic<size_t> writePos;
    std::atomic<size_t> readPos;
};

class LowLatencyProcessor {
public:
    LowLatencyProcessor();
    ~LowLatencyProcessor();
    
    // Configuration
    bool initialize(int sampleRate, int bufferSize, int numChannels);
    void shutdown();
    
    // Processing
    void processAudio(const float* input, float* output, int numSamples);
    
    // Latency optimization
    void setBufferSize(int bufferSize);
    int getBufferSize() const { return bufferSize; }
    void setOptimizationLevel(int level); // 0=balanced, 1=low-latency, 2=ultra-low-latency
    int getOptimizationLevel() const { return optimizationLevel; }
    
    // Real-time processing
    void enableRealTimeProcessing(bool enable);
    bool isRealTimeProcessingEnabled() const { return realTimeEnabled; }
    
    // Monitoring and statistics
    float getCurrentLatency() const; // in milliseconds
    float getAverageLatency() const;
    float getCPUUsage() const;
    int getXrunCount() const { return xrunCount; }
    void resetStatistics();
    
    // Thread priority and affinity
    void setThreadPriority(int priority);
    void setThreadAffinity(const std::vector<int>& cpuCores);

private:
    // Configuration
    int sampleRate;
    int bufferSize;
    int numChannels;
    int optimizationLevel;
    bool initialized;
    bool realTimeEnabled;
    
    // Buffers
    std::unique_ptr<RingBuffer> inputBuffer;
    std::unique_ptr<RingBuffer> outputBuffer;
    std::unique_ptr<float[]> processingBuffer;
    
    // Statistics
    mutable std::mutex statsMutex;
    std::vector<float> latencyHistory;
    std::atomic<int> xrunCount;
    std::chrono::high_resolution_clock::time_point lastProcessTime;
    
    // Threading
    std::thread processingThread;
    std::atomic<bool> shouldStop;
    std::condition_variable processSignal;
    std::mutex processSignalMutex;
    
    // Internal methods
    void processingThreadFunction();
    void optimizeForLatency();
    void measureLatency();
    float calculateCPUUsage() const;
    
    // Platform-specific optimizations
    void setPlatformOptimizations();
    void setThreadRealtimePriority();
};

} // namespace Warrior