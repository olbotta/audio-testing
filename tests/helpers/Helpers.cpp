#include "Helpers.h"
#include "sine sweep/SineSweep.h"

std::unique_ptr<juce::AudioBuffer<float>> Helpers::generateNoise(int channels, int samples)
{
    auto buffer = std::make_unique<juce::AudioBuffer<float>>(channels, samples);

    //Fill with random values ranging from -1 to 1
    for (int i = 0; i < channels; i++) {
        for (int j = 0; j < samples; j++) {
            buffer->setSample(i, j, -1.0f + juce::Random::getSystemRandom().nextFloat() * 2.0f);
        }
    }

    return buffer;
}

std::unique_ptr<juce::AudioBuffer<float>>  Helpers::generateIncreasingAudioSampleBuffer(int channels, int samples)
{
    auto buffer = std::make_unique<juce::AudioBuffer<float>>(channels, samples);

    //Fill with increasing values ranging from -1 to 1
    for (int i = 0; i < channels; i++) {
        for (int j = 0; j < samples; j++) {
            buffer->setSample(i, j, juce::jmap((float)(j % 1001), 0.0f, 1000.0f, -1.0f, 1.0f));
        }
    }

    return buffer;
}

//TODO: test this
std::unique_ptr<juce::AudioBuffer<float>>  Helpers::generateSineSweep(int channels, int samples, float sampleRate)
{
    auto buffer = std::make_unique<juce::AudioBuffer<float>>(channels, samples);
    SineSweep sineSweep(sampleRate);
    sineSweep.setSweepRange(20.0f, 20000.0f);
    sineSweep.setSweepDuration(5.0f);
    sineSweep.reset();

    for (int sample = 0; sample < buffer->getNumSamples(); ++sample)
    {
        float sweepSample = sineSweep.getNextSample();
        for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
        {
            buffer->setSample(channel, sample, sweepSample);
        }
    }

    return buffer;
}
