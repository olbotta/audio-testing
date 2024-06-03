#include "Settings.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <utility>

//TODO: write it using JUCE guide
template<typename Type>
juce::Array<Type> getBinEnergies (const juce::dsp::FFT* forwardFFT, juce::AudioBuffer<Type> in)
{
    auto* inRead = in.getArrayOfReadPointers();

    std::array<Type, fft_size> inFifo;
    std::array<Type, fft_size * 2> inFftData;
    juce::Array<Type> binEnergies;
    unsigned long fifoIndex = 0;

    for (int sam = 0; sam < in.getNumSamples(); ++sam)
    {
        for (int ch = 0; ch < in.getNumChannels(); ++ch)
        {
            if (fifoIndex == fft_size)//end of the bin(?)
            {
                std::ranges::fill (inFftData.begin(), inFftData.end(), 0.0f);//superfluo? (viene sovrascritto subito dopo)
                std::ranges::copy (inFifo.begin(), inFifo.end(), inFftData.begin());
                forwardFFT->performFrequencyOnlyForwardTransform (inFftData.data(), true);

                for (unsigned long fftIndex = 0; fftIndex < inFftData.size() / 2; fftIndex++)
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

template<typename Type>
struct AudioBufferLowerEnergyMatcher : Catch::Matchers::MatcherGenericBase
{
public:
    AudioBufferLowerEnergyMatcher (juce::AudioBuffer<Type> const& buffer) :
                                                                     forwardFFT (fft_order),
                                                                     buffer (buffer)
                                                                //window { fft_size, juce::dsp::WindowingFunction<float>::WindowingMethod::hann }
    {}

    bool match (juce::AudioBuffer<Type> const& other) const
    {
        if (buffer.getNumChannels() != other.getNumChannels() || buffer.getNumSamples() != other.getNumSamples())
            return false;

        Type bufferPower = 0;
        Type otherPower = 0;
        juce::Array<Type> bufferEnergies = getBinEnergies (&forwardFFT, buffer);
        juce::Array<Type> otherEnergies  = getBinEnergies (&forwardFFT, other);

        for (int i = 0; i < bufferEnergies.size(); i++)
        {
            bufferPower += bufferEnergies[i];
            otherPower  += otherEnergies[i];
        }

        std::cout<< "bufferPower: " + std::to_string(bufferPower) + ", otherPower: "+ std::to_string(otherPower) + "\n";

        return bufferPower < otherPower;

    }

    std::string describe() const override
    {
        return "Has lower cumulative energy than other";
    }

private:
    juce::dsp::FFT forwardFFT;
    juce::AudioBuffer<Type> buffer;
};

template<typename Type>
auto AudioBufferLowerEnergy (juce::AudioBuffer<Type> buffer) -> AudioBufferLowerEnergyMatcher<Type>{
    return AudioBufferLowerEnergyMatcher<Type>{buffer};
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
