/*
  ==============================================================================
    SpectralAnalyzer.h - Multi-band L/R level analyzer with proper bass response
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SharedDataManager.h"
#include <array>
#include <cmath>

struct BandResult
{
    float leftLevel = 0.0f;
    float rightLevel = 0.0f;
};

class SpectralAnalyzer
{
public:
    SpectralAnalyzer()
        : fft(kFFTOrder),
          window(static_cast<size_t>(kFFTSize), juce::dsp::WindowingFunction<float>::hann)
    {
        leftBuf.resize(static_cast<size_t>(kFFTSize), 0.0f);
        rightBuf.resize(static_cast<size_t>(kFFTSize), 0.0f);
        leftFFT.resize(static_cast<size_t>(kFFTSize * 2), 0.0f);
        rightFFT.resize(static_cast<size_t>(kFFTSize * 2), 0.0f);
        calcBands();
    }
    
    void prepare(double sr, int)
    {
        sampleRate = sr;
        calcBands();
        clear();
    }
    
    void setNumBands(int n)
    {
        activeBands = juce::jlimit(12, static_cast<int>(kMaxBands), n);
        calcBands();
    }
    
    int getNumBands() const { return activeBands; }
    
    void clear()
    {
        std::fill(leftBuf.begin(), leftBuf.end(), 0.0f);
        std::fill(rightBuf.begin(), rightBuf.end(), 0.0f);
        writePos = 0;
        sampleCount = 0;
        for (auto& b : results) b = BandResult{};
    }
    
    bool process(const float* L, const float* R, int numSamples)
    {
        bool ready = false;
        for (int i = 0; i < numSamples; ++i)
        {
            leftBuf[static_cast<size_t>(writePos)] = L[i];
            rightBuf[static_cast<size_t>(writePos)] = R[i];
            writePos = (writePos + 1) % kFFTSize;
            if (++sampleCount >= kFFTSize / 4)
            {
                analyze();
                sampleCount = 0;
                ready = true;
            }
        }
        return ready;
    }
    
    const std::array<BandResult, kMaxBands>& getResults() const { return results; }
    
private:
    void calcBands()
    {
        // Logarithmic frequency bands from 20Hz to 20kHz
        const float minF = 20.0f, maxF = 20000.0f;
        const float logMin = std::log10(minF), logMax = std::log10(maxF);
        
        for (int i = 0; i <= activeBands; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(activeBands);
            float freq = std::pow(10.0f, logMin + t * (logMax - logMin));
            
            // Store exact fractional bin for interpolation
            float binFloat = freq * static_cast<float>(kFFTSize) / static_cast<float>(sampleRate);
            bandBinsFloat[static_cast<size_t>(i)] = binFloat;
            bandFreqs[static_cast<size_t>(i)] = freq;
        }
    }
    
    void analyze()
    {
        // Copy and window L/R channels separately
        for (int i = 0; i < kFFTSize; ++i)
        {
            int idx = (writePos + i) % kFFTSize;
            leftFFT[static_cast<size_t>(i)] = leftBuf[static_cast<size_t>(idx)];
            rightFFT[static_cast<size_t>(i)] = rightBuf[static_cast<size_t>(idx)];
        }
        std::fill(leftFFT.begin() + kFFTSize, leftFFT.end(), 0.0f);
        std::fill(rightFFT.begin() + kFFTSize, rightFFT.end(), 0.0f);
        
        window.multiplyWithWindowingTable(leftFFT.data(), static_cast<size_t>(kFFTSize));
        window.multiplyWithWindowingTable(rightFFT.data(), static_cast<size_t>(kFFTSize));
        fft.performFrequencyOnlyForwardTransform(leftFFT.data());
        fft.performFrequencyOnlyForwardTransform(rightFFT.data());
        
        // Normalization: 2/N for FFT, ~2 for Hann window correction
        constexpr float fftNorm = 4.0f / static_cast<float>(kFFTSize);
        constexpr float smooth = 0.88f;
        
        for (int band = 0; band < activeBands; ++band)
        {
            size_t bandIdx = static_cast<size_t>(band);
            float startBinF = bandBinsFloat[bandIdx];
            float endBinF = bandBinsFloat[bandIdx + 1];
            
            // Use interpolation to get unique values even when bins overlap
            float leftEnergy = 0.0f, rightEnergy = 0.0f;
            float totalWeight = 0.0f;
            
            int startBin = std::max(1, static_cast<int>(std::floor(startBinF)));
            int endBin = std::min(kNumBins - 1, static_cast<int>(std::ceil(endBinF)));
            
            for (int bin = startBin; bin <= endBin; ++bin)
            {
                // Calculate how much this bin contributes to this band
                float binStart = static_cast<float>(bin) - 0.5f;
                float binEnd = static_cast<float>(bin) + 0.5f;
                
                float overlapStart = std::max(binStart, startBinF);
                float overlapEnd = std::min(binEnd, endBinF);
                float weight = std::max(0.0f, overlapEnd - overlapStart);
                
                if (weight > 0.0f)
                {
                    size_t b = static_cast<size_t>(bin);
                    float lMag = leftFFT[b] * fftNorm;
                    float rMag = rightFFT[b] * fftNorm;
                    leftEnergy += lMag * lMag * weight;
                    rightEnergy += rMag * rMag * weight;
                    totalWeight += weight;
                }
            }
            
            // Normalize by weight
            if (totalWeight > 0.0f)
            {
                leftEnergy /= totalWeight;
                rightEnergy /= totalWeight;
            }
            
            float leftRMS = std::sqrt(leftEnergy);
            float rightRMS = std::sqrt(rightEnergy);
            
            // Apply pink noise compensation for perceptually flat response
            // Bass frequencies need a boost since they have less energy per band
            float centerFreq = (bandFreqs[bandIdx] + bandFreqs[bandIdx + 1]) * 0.5f;
            float pinkComp = std::sqrt(centerFreq / 1000.0f);  // +3dB/octave from 1kHz
            pinkComp = juce::jlimit(0.3f, 3.0f, pinkComp);
            
            leftRMS *= pinkComp;
            rightRMS *= pinkComp;
            
            // Smooth
            results[bandIdx].leftLevel = results[bandIdx].leftLevel * smooth + leftRMS * (1.0f - smooth);
            results[bandIdx].rightLevel = results[bandIdx].rightLevel * smooth + rightRMS * (1.0f - smooth);
        }
    }
    
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> leftBuf, rightBuf, leftFFT, rightFFT;
    std::array<float, kMaxBands + 1> bandBinsFloat{};
    std::array<float, kMaxBands + 1> bandFreqs{};
    std::array<BandResult, kMaxBands> results{};
    int writePos = 0, sampleCount = 0;
    int activeBands = 24;
    double sampleRate = 44100.0;
};
