#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <catch2/catch_all.hpp>

class Helpers {
public:
    static std::unique_ptr<juce::AudioBuffer<float>> generateNoise(int channels = 2, int samples = 4096);
    static std::unique_ptr<juce::AudioBuffer<float>> generateIncreasingAudioSampleBuffer(int channels = 2, int samples = 4096);
    static std::unique_ptr<juce::AudioBuffer<float>> generateSineSweep (int channels, int samples, float sampleRate);
};
