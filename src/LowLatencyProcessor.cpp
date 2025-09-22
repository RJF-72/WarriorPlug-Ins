#include "LowLatencyProcessor.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <cstring>
#include <numeric>
#include <cstdlib>

// Platform-specific headers with guards
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#elif defined(__linux__)
#include <sched.h>
#include <pthread.h>
#ifdef HAVE_MLOCK
#include <sys/mlock.h>
#endif
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#endif

namespace Warrior {

// RingBuffer Implementation
RingBuffer::RingBuffer(size_t size) : size(size), writePos(0), readPos(0) {
    buffer = std::make_unique<float[]>(size);
    reset();
}

RingBuffer::~RingBuffer() = default;

size_t RingBuffer::write(const float* data, size_t samples) {
    const size_t currentWritePos = writePos.load();
    const size_t currentReadPos = readPos.load();
    
    size_t availableSpace = (currentReadPos - currentWritePos - 1 + size) % size;
    size_t samplesToWrite = std::min(samples, availableSpace);
    
    if (samplesToWrite == 0) return 0;
    
    // Handle wrap-around
    size_t firstChunkSize = std::min(samplesToWrite, size - currentWritePos);
    memcpy(buffer.get() + currentWritePos, data, firstChunkSize * sizeof(float));
    
    if (samplesToWrite > firstChunkSize) {
        memcpy(buffer.get(), data + firstChunkSize, (samplesToWrite - firstChunkSize) * sizeof(float));
    }
    
    writePos.store((currentWritePos + samplesToWrite) % size);
    return samplesToWrite;
}

size_t RingBuffer::read(float* data, size_t samples) {
    const size_t currentWritePos = writePos.load();
    const size_t currentReadPos = readPos.load();
    
    size_t availableData = (currentWritePos - currentReadPos + size) % size;
    size_t samplesToRead = std::min(samples, availableData);
    
    if (samplesToRead == 0) return 0;
    
    // Handle wrap-around
    size_t firstChunkSize = std::min(samplesToRead, size - currentReadPos);
    memcpy(data, buffer.get() + currentReadPos, firstChunkSize * sizeof(float));
    
    if (samplesToRead > firstChunkSize) {
        memcpy(data + firstChunkSize, buffer.get(), (samplesToRead - firstChunkSize) * sizeof(float));
    }
    
    readPos.store((currentReadPos + samplesToRead) % size);
    return samplesToRead;
}

size_t RingBuffer::available() const {
    const size_t currentWritePos = writePos.load();
    const size_t currentReadPos = readPos.load();
    return (currentWritePos - currentReadPos + size) % size;
}

size_t RingBuffer::space() const {
    return size - available() - 1;
}

void RingBuffer::reset() {
    writePos.store(0);
    readPos.store(0);
    memset(buffer.get(), 0, size * sizeof(float));
}

// LowLatencyProcessor Implementation
LowLatencyProcessor::LowLatencyProcessor() 
    : sampleRate(44100), bufferSize(128), numChannels(2), optimizationLevel(1),
      initialized(false), realTimeEnabled(true), xrunCount(0), shouldStop(false) {
}

LowLatencyProcessor::~LowLatencyProcessor() {
    shutdown();
}

bool LowLatencyProcessor::initialize(int sampleRate, int bufferSize, int numChannels) {
    if (initialized) {
        shutdown();
    }
    
    this->sampleRate = sampleRate;
    this->bufferSize = bufferSize;
    this->numChannels = numChannels;
    
    // Create ring buffers with 4x buffer size for safety
    size_t ringBufferSize = bufferSize * numChannels * 4;
    inputBuffer = std::make_unique<RingBuffer>(ringBufferSize);
    outputBuffer = std::make_unique<RingBuffer>(ringBufferSize);
    
    // Allocate processing buffer
    processingBuffer = std::make_unique<float[]>(bufferSize * numChannels);
    
    // Initialize latency measurement
    latencyHistory.reserve(1000);
    
    // Apply platform-specific optimizations
    setPlatformOptimizations();
    
    // Start processing thread
    shouldStop = false;
    processingThread = std::thread(&LowLatencyProcessor::processingThreadFunction, this);
    
    initialized = true;
    
    std::cout << "Low-latency processor initialized: " << sampleRate << "Hz, " 
              << bufferSize << " samples, " << numChannels << " channels" << std::endl;
    
    return true;
}

void LowLatencyProcessor::shutdown() {
    if (!initialized) return;
    
    shouldStop = true;
    processSignal.notify_all();
    
    if (processingThread.joinable()) {
        processingThread.join();
    }
    
    inputBuffer.reset();
    outputBuffer.reset();
    processingBuffer.reset();
    
    initialized = false;
}

void LowLatencyProcessor::processAudio(const float* input, float* output, int numSamples) {
    if (!initialized) {
        memset(output, 0, numSamples * numChannels * sizeof(float));
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Write input to ring buffer
    size_t samplesWritten = inputBuffer->write(input, numSamples * numChannels);
    if (samplesWritten < numSamples * numChannels) {
        xrunCount++;
        std::cerr << "Input buffer overrun detected!" << std::endl;
    }
    
    // Trigger processing
    {
        std::lock_guard<std::mutex> lock(processSignalMutex);
        processSignal.notify_one();
    }
    
    // Read processed output
    size_t samplesRead = outputBuffer->read(output, numSamples * numChannels);
    if (samplesRead < numSamples * numChannels) {
        // Fill remaining with silence
        memset(output + samplesRead, 0, (numSamples * numChannels - samplesRead) * sizeof(float));
        xrunCount++;
        std::cerr << "Output buffer underrun detected!" << std::endl;
    }
    
    // Measure latency
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    std::lock_guard<std::mutex> lock(statsMutex);
    float latencyMs = duration.count() / 1000.0f;
    latencyHistory.push_back(latencyMs);
    if (latencyHistory.size() > 1000) {
        latencyHistory.erase(latencyHistory.begin());
    }
}

void LowLatencyProcessor::setBufferSize(int bufferSize) {
    if (this->bufferSize != bufferSize) {
        this->bufferSize = bufferSize;
        if (initialized) {
            // Reinitialize with new buffer size
            int oldSampleRate = sampleRate;
            int oldNumChannels = numChannels;
            shutdown();
            initialize(oldSampleRate, bufferSize, oldNumChannels);
        }
    }
}

void LowLatencyProcessor::setOptimizationLevel(int level) {
    optimizationLevel = std::clamp(level, 0, 2);
    optimizeForLatency();
}

void LowLatencyProcessor::enableRealTimeProcessing(bool enable) {
    realTimeEnabled = enable;
    if (enable) {
        setThreadRealtimePriority();
    }
}

float LowLatencyProcessor::getCurrentLatency() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (latencyHistory.empty()) return 0.0f;
    return latencyHistory.back();
}

float LowLatencyProcessor::getAverageLatency() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (latencyHistory.empty()) return 0.0f;
    
    float sum = std::accumulate(latencyHistory.begin(), latencyHistory.end(), 0.0f);
    return sum / latencyHistory.size();
}

float LowLatencyProcessor::getCPUUsage() const {
    return calculateCPUUsage();
}

void LowLatencyProcessor::resetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex);
    latencyHistory.clear();
    xrunCount = 0;
}

void LowLatencyProcessor::processingThreadFunction() {
    setThreadRealtimePriority();
    
    while (!shouldStop) {
        std::unique_lock<std::mutex> lock(processSignalMutex);
        processSignal.wait(lock, [this] { return shouldStop || inputBuffer->available() >= bufferSize * numChannels; });
        
        if (shouldStop) break;
        
        // Process available audio
        while (inputBuffer->available() >= bufferSize * numChannels && !shouldStop) {
            size_t samplesRead = inputBuffer->read(processingBuffer.get(), bufferSize * numChannels);
            
            if (samplesRead == bufferSize * numChannels) {
                // Perform actual audio processing here
                // For now, just pass through
                outputBuffer->write(processingBuffer.get(), bufferSize * numChannels);
            }
        }
    }
}

void LowLatencyProcessor::optimizeForLatency() {
    switch (optimizationLevel) {
        case 0: // Balanced
            setBufferSize(256);
            break;
        case 1: // Low-latency
            setBufferSize(128);
            break;
        case 2: // Ultra-low-latency
            setBufferSize(64);
            break;
    }
}

void LowLatencyProcessor::setPlatformOptimizations() {
#ifdef _WIN32
    // Windows-specific optimizations
    timeBeginPeriod(1); // Set timer resolution to 1ms
#elif defined(__linux__) && defined(HAVE_MLOCK)
    // Linux-specific optimizations
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::cerr << "Warning: Could not lock memory pages" << std::endl;
    }
#endif
}

void LowLatencyProcessor::setThreadRealtimePriority() {
    if (!realTimeEnabled) return;
    
#ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#elif defined(__linux__)
    struct sched_param param;
    param.sched_priority = 80; // High priority
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#elif defined(__APPLE__)
    thread_time_constraint_policy_data_t policy;
    policy.period = (mach_timebase_info_data_t){1, 1000000}; // 1ms
    policy.computation = policy.period / 3;
    policy.constraint = policy.period;
    policy.preemptible = 1;
    
    thread_policy_set(mach_thread_self(), THREAD_TIME_CONSTRAINT_POLICY,
                     (thread_policy_t)&policy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
#endif
}

float LowLatencyProcessor::calculateCPUUsage() const {
    // Simplified CPU usage calculation
    // In a real implementation, this would measure actual CPU time
    return std::min(100.0f, (float)xrunCount.load() * 10.0f);
}

} // namespace Warrior