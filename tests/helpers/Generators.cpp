#include "Generators.h"
#include "sine sweep/SineSweep.h"

std::unique_ptr<juce::AudioBuffer<float>> Generators::generateNoise(int channels, int samples)
{
    juce::Random randomGenerator = juce::Random(SEED);

    auto buffer = std::make_unique<juce::AudioBuffer<float>>(channels, samples);

        for (int s = 0; s < samples; s++)
        {
            auto sampleValue = -1.0f + randomGenerator.nextFloat() * 2.0f;
            for (int c = 0; c < channels; ++c)
            {
                buffer->setSample (c, s, sampleValue);
            }
        }

    return buffer;
}

std::unique_ptr<juce::AudioBuffer<float>> Generators::generateIncreasingAudioSampleBuffer(int channels, int samples)
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
std::unique_ptr<juce::AudioBuffer<float>> Generators::generateSineSweep(int channels, int samples, float sampleRate)
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
