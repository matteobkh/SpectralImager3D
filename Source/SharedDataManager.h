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
constexpr int kFFTOrder = 12;  // 4096-point FFT
constexpr int kFFTSize = 1 << kFFTOrder;
constexpr int kNumBins = kFFTSize / 2;
constexpr size_t kMaxBands = 64;

// Per-band data
struct BandInfo
{
    std::atomic<float> leftLevel{ 0.0f };
    std::atomic<float> rightLevel{ 0.0f };
};

struct TrackData
{
    std::array<BandInfo, kMaxBands> bands;
    std::atomic<uint32_t> colorARGB{ 0xFF00FFFF };
    std::atomic<bool> isActive{ false };
    std::atomic<uint64_t> lastUpdate{ 0 };
    std::atomic<uint64_t> instanceId{ 0 };
    std::atomic<int> numBands{ 24 };
    
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

// Abstract interface for data providers
class ITrackDataProvider
{
public:
    virtual ~ITrackDataProvider() = default;
    virtual TrackData& getTrack(int i) = 0;
    virtual const TrackData& getTrack(int i) const = 0;
    virtual int getActiveCount() const = 0;
    virtual void updateTimestamp(int slot) = 0;
    virtual void cleanupStale(uint64_t timeout) = 0;
};

// Standard implementation with mutexes and registration logic
class SharedDataManager : public ITrackDataProvider
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
    
    TrackData& getTrack(int i) override
    {
        size_t idx = static_cast<size_t>(juce::jlimit(0, static_cast<int>(kMaxTracks) - 1, i));
        return tracks[idx];
    }
    
    const TrackData& getTrack(int i) const override
    {
        size_t idx = static_cast<size_t>(juce::jlimit(0, static_cast<int>(kMaxTracks) - 1, i));
        return tracks[idx];
    }
    
    int getActiveCount() const override
    {
        int n = 0;
        for (size_t i = 0; i < kMaxTracks; ++i)
            if (tracks[i].isActive.load()) ++n;
        return n;
    }
    
    void updateTimestamp(int slot) override
    {
        if (slot >= 0 && slot < static_cast<int>(kMaxTracks))
        {
            auto& t = tracks[static_cast<size_t>(slot)];
            t.lastUpdate.store(static_cast<uint64_t>(juce::Time::currentTimeMillis()));
            if (!t.isActive.load(std::memory_order_relaxed))
                t.isActive.store(true, std::memory_order_relaxed);
        }
    }
    
    void cleanupStale(uint64_t timeout) override
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

// Local implementation for Unified mode - no locking, no cleanup
class LocalDataManager : public ITrackDataProvider
{
public:
    LocalDataManager()
    {
        // Always 8 active tracks for Unified mode
        for(size_t i = 0; i < 8; ++i)
            tracks[i].isActive.store(true);
    }

    TrackData& getTrack(int i) override
    {
        size_t idx = static_cast<size_t>(juce::jlimit(0, static_cast<int>(kMaxTracks) - 1, i));
        return tracks[idx];
    }
    
    const TrackData& getTrack(int i) const override
    {
        size_t idx = static_cast<size_t>(juce::jlimit(0, static_cast<int>(kMaxTracks) - 1, i));
        return tracks[idx];
    }
    
    int getActiveCount() const override
    {
        // In unified mode, we technically have 8 "available", but maybe we only want to count active?
        // Actually, we just return the hardcoded count or scan. Scanning is safer.
        int n = 0;
        for (size_t i = 0; i < kMaxTracks; ++i)
            if (tracks[i].isActive.load()) ++n;
        return n;
    }
    
    void updateTimestamp(int) override
    {
        // No-op in local mode, tracks never expire
    }
    
    void cleanupStale(uint64_t) override
    {
        // No-op, tracks persist indefinitely
    }

private:
    std::array<TrackData, kMaxTracks> tracks;
};
