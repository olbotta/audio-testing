#include "Settings.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <utility>

//TODO: tipizzare con type
juce::Array<float> getBinEnergies (const juce::dsp::FFT* forwardFFT, juce::AudioBuffer<float> in)
{
    auto* inRead = in.getArrayOfReadPointers();

    std::array<float, fft_size> inFifo;
    std::array<float, fft_size * 2> inFftData;
    juce::Array<float> binEnergies;
    int fifoIndex = 0;

    for (int sam = 0; sam < in.getNumSamples(); ++sam)
    {
        for (int ch = 0; ch < in.getNumChannels(); ++ch)
        {
            if (fifoIndex == fft_size)
            {
                std::ranges::fill (inFftData.begin(), inFftData.end(), 0.0f);
                std::ranges::copy (inFifo.begin(), inFifo.end(), inFftData.begin());// questo non è un'altra inizializzazone a 0?
                forwardFFT->performFrequencyOnlyForwardTransform (inFftData.data(), true);

                for (unsigned int fftIndex = 0; fftIndex < inFftData.size() / 2; fftIndex++)
                {
                    binEnergies.set (fftIndex, binEnergies[fftIndex] + inFftData[fftIndex]);
                }

                fifoIndex = 0;
            }
            inFifo[fifoIndex] = inRead[ch][sam];
            fifoIndex++;
        }
    }

    return binEnergies;
}

template<typename Type>
struct AudioBufferMatcher : Catch::Matchers::MatcherGenericBase
{
public:
    AudioBufferMatcher (juce::AudioBuffer<Type> const& buffer) : buffer (buffer) {}

    bool match (juce::AudioBuffer<Type> const& other) const
    {
        if (buffer.getNumChannels() != other.getNumChannels() || buffer.getNumSamples() != other.getNumSamples())
            return false;

        auto* inRead  = buffer.getArrayOfReadPointers();
        auto* othRead = other.getArrayOfReadPointers();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int sam = 0; sam < buffer.getNumSamples(); ++sam)
            {
                if (std::fabs (inRead[ch][sam] - othRead[ch][sam]) > std::numeric_limits<Type>::epsilon())
                {
                    return false;
                }
            }
        }

        return true;
    }

    std::string describe() const override
    {
        return "AudioBuffers have the same values and lenght";
    }

private:
    juce::AudioBuffer<Type> buffer;
};

template<typename Type>
auto AudioBuffersMatch (juce::AudioBuffer<Type> buffer) -> AudioBufferMatcher<Type>{
 return AudioBufferMatcher<Type>{buffer};
}

class AudioBufferEnergyMatcher : public Catch::Matchers::MatcherBase<juce::AudioBuffer<float>>
{
    juce::AudioBuffer<float> otherBuffer;

public:
    AudioBufferEnergyMatcher (juce::AudioBuffer<float> other) :
                                                                         otherBuffer (std::move(other)),
                                                                         forwardFFT (fft_order)
                                                                         //window { fft_size, juce::dsp::WindowingFunction<float>::WindowingMethod::hann }
    {}

    bool match (juce::AudioBuffer<float> const& in) const override
    {
        float inPower = 0;
        float othPower = 0;

        if (in.getNumChannels() != otherBuffer.getNumChannels() || in.getNumSamples() != otherBuffer.getNumSamples())
        {
            return false;
        }

        juce::Array<float> inEnergies = getBinEnergies (&forwardFFT, in);
        juce::Array<float> othEnergies = getBinEnergies (&forwardFFT, otherBuffer);

        for (int i = 0; i < inEnergies.size(); i++)
        {
            inPower += inEnergies[i];
            othPower += othEnergies[i];
        }

        return inPower < othPower;

    }

    std::string describe() const override
    {
        return "Other buffer has higher total energy";
    }

private:
    juce::dsp::FFT forwardFFT;
    //juce::dsp::WindowingFunction<float> window;
};

AudioBufferEnergyMatcher AudioBufferHigherEnergy (juce::AudioBuffer<float> other)
{
    return  AudioBufferEnergyMatcher(other);
}

class AudioBufferMaxEnergyMatcher : public Catch::Matchers::MatcherBase<juce::AudioBuffer<float>>
{
    juce::Array<float> maxEnergies;

public:
    AudioBufferMaxEnergyMatcher (juce::Array<float> other) : maxEnergies (other), forwardFFT (fft_order), window { fft_size, juce::dsp::WindowingFunction<float>::WindowingMethod::hann } {}

    bool match (juce::AudioBuffer<float> const& in) const override
    {
        juce::Array<float> binEnergies = getBinEnergies (&forwardFFT, in);

        for (int i = 0; i < maxEnergies.size(); i++)
        {
            if (maxEnergies[i] == -1)
            {
                continue;
            }
            if (binEnergies[i] > maxEnergies[i])
            {
                return false;
            }
        }
        return true;
    }

    std::string describe() const override
    {
        std::ostringstream ss;
        ss << "Energies too high";
        return ss.str();
    }

private:
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
};

AudioBufferMaxEnergyMatcher AudioBufferCheckMaxEnergy (juce::Array<float> maxEnergiesArray)
{
    return { maxEnergiesArray };
}

class AudioBufferMinEnergyMatcher : public Catch::Matchers::MatcherBase<juce::AudioBuffer<float>>
{
    juce::Array<float> minEnergies;

public:
    AudioBufferMinEnergyMatcher (juce::Array<float> other) : minEnergies (other), forwardFFT (fft_order), window { fft_size, juce::dsp::WindowingFunction<float>::WindowingMethod::hann } {}

    bool match (juce::AudioBuffer<float> const& in) const override
    {
        juce::Array<float> binEnergies = getBinEnergies (&forwardFFT, in);

        for (int i = 0; i < minEnergies.size(); i++)
        {
            if (minEnergies[i] == -1)
            {
                continue;
            }
            if (binEnergies[i] < minEnergies[i])
            {
                return false;
            }
        }
        return true;
    }

    std::string describe() const override
    {
        std::ostringstream ss;
        ss << "Energies too low";
        return ss.str();
    }

private:
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
};

AudioBufferMinEnergyMatcher AudioBufferCheckMinEnergy (juce::Array<float> minEnergiesArray)
{
    return { minEnergiesArray };
}
