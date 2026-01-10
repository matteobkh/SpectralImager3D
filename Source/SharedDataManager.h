/*
  ==============================================================================
    SharedDataManager.h - Shared data for multi-instance communication
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <mutex>

constexpr size_t kMaxTracks = 16;
constexpr int kFFTOrder = 12;  // 4096-point FFT for better bass resolution (~10Hz per bin at 44.1kHz)
constexpr int kFFTSize = 1 << kFFTOrder;
constexpr int kNumBins = kFFTSize / 2;
constexpr size_t kMaxBands = 64;  // Maximum supported bands

// Per-band data: stores L/R levels for Ozone-style visualization
struct BandInfo
{
    std::atomic<float> leftLevel{ 0.0f };   // Left channel level (0-1)
    std::atomic<float> rightLevel{ 0.0f };  // Right channel level (0-1)
};

struct TrackData
{
    std::array<BandInfo, kMaxBands> bands;
    std::atomic<uint32_t> colorARGB{ 0xFF00FFFF };
    std::atomic<bool> isActive{ false };
    std::atomic<uint64_t> lastUpdate{ 0 };
    std::atomic<uint64_t> instanceId{ 0 };
    std::atomic<int> numBands{ 24 };  // Current number of active bands
    
    void getBand(size_t i, float& left, float& right) const
    {
        if (i < kMaxBands)
        {
            left = bands[i].leftLevel.load(std::memory_order_relaxed);
            right = bands[i].rightLevel.load(std::memory_order_relaxed);
        }
    }
    
    void setBand(size_t i, float left, float right)
    {
        if (i < kMaxBands)
        {
            bands[i].leftLevel.store(left, std::memory_order_relaxed);
            bands[i].rightLevel.store(right, std::memory_order_relaxed);
        }
    }
    
    juce::Colour getColor() const { return juce::Colour(colorARGB.load(std::memory_order_relaxed)); }
    void setColor(juce::Colour c) { colorARGB.store(c.getARGB(), std::memory_order_relaxed); }
};

class SharedDataManager
{
public:
    int registerSender(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (size_t i = 0; i < kMaxTracks; ++i)
            if (tracks[i].instanceId.load() == id) return static_cast<int>(i);
        for (size_t i = 0; i < kMaxTracks; ++i)
        {
            if (!tracks[i].isActive.load())
            {
                tracks[i].instanceId.store(id);
                tracks[i].isActive.store(true);
                tracks[i].lastUpdate.store(static_cast<uint64_t>(juce::Time::currentTimeMillis()));
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    void unregisterSender(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (size_t i = 0; i < kMaxTracks; ++i)
        {
            if (tracks[i].instanceId.load() == id)
            {
                tracks[i].isActive.store(false);
                tracks[i].instanceId.store(0);
                break;
            }
        }
    }
    
    TrackData& getTrack(int i)
    {
        size_t idx = static_cast<size_t>(juce::jlimit(0, static_cast<int>(kMaxTracks) - 1, i));
        return tracks[idx];
    }
    
    const TrackData& getTrack(int i) const
    {
        size_t idx = static_cast<size_t>(juce::jlimit(0, static_cast<int>(kMaxTracks) - 1, i));
        return tracks[idx];
    }
    
    int getActiveCount() const
    {
        int n = 0;
        for (size_t i = 0; i < kMaxTracks; ++i)
            if (tracks[i].isActive.load()) ++n;
        return n;
    }
    
    void updateTimestamp(int slot)
    {
        if (slot >= 0 && slot < static_cast<int>(kMaxTracks))
            tracks[static_cast<size_t>(slot)].lastUpdate.store(static_cast<uint64_t>(juce::Time::currentTimeMillis()));
    }
    
    void cleanupStale(uint64_t timeout = 1000)
    {
        uint64_t now = static_cast<uint64_t>(juce::Time::currentTimeMillis());
        for (size_t i = 0; i < kMaxTracks; ++i)
        {
            if (tracks[i].isActive.load() && now - tracks[i].lastUpdate.load() > timeout)
            {
                tracks[i].isActive.store(false);
                tracks[i].instanceId.store(0);
            }
        }
    }
    
private:
    std::array<TrackData, kMaxTracks> tracks;
    std::mutex mutex;
};
