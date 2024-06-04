#include "../Settings.h"
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_templated.hpp"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"
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
    explicit AudioBufferMatcher (juce::AudioBuffer<Type> const& buffer) : buffer (buffer) {}

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
        return "AudioBuffers should have same length and internal values";
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
    explicit AudioBufferLowerEnergyMatcher (juce::AudioBuffer<Type> const& buffer) :
                                                                     forwardFFT (fft_order),
                                                                     buffer (buffer)
    {}

    bool match (juce::AudioBuffer<Type> const& other) const
    { // ⚠️ @code other refers to the first argument of CHECK_THAT / REQUIRES_THAT
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

        return otherPower < bufferPower;

    }

    std::string describe() const override
    {
        return "Buffer should have lower cumulative energy than other";
    }

private:
    juce::dsp::FFT forwardFFT;
    juce::AudioBuffer<Type> buffer;
};

template<typename Type>
auto HasLowerCumulativeEnergyThan (juce::AudioBuffer<Type> buffer) -> AudioBufferLowerEnergyMatcher<Type>{
    return AudioBufferLowerEnergyMatcher<Type>{buffer};
}

template<typename Type>
struct AudioBufferLowerEnergyFftBinsMatcher : public Catch::Matchers::MatcherGenericBase
{
public:
    explicit AudioBufferLowerEnergyFftBinsMatcher (juce::Array<Type> fftBinsValues) :
                                                                    fftBinsValues (fftBinsValues),
                                                                    forwardFFT (fft_order)
    {}

    bool match (juce::AudioBuffer<Type> const& other) const
    {
        juce::Array<Type> bufferBinEnergies = getBinEnergies (&forwardFFT, other);

        for (int i = 0; i < fftBinsValues.size(); i++)
        {
            if (fftBinsValues[i] == -1)//flagged as don't control TODO: refactor to more clear function
                continue;

            if (bufferBinEnergies[i] < fftBinsValues[i])
                return false;
        }
        return true;
    }

    std::string describe() const override { return "Buffer should have lower energies in the selected bins";}

private:
    juce::Array<Type> fftBinsValues;
    juce::dsp::FFT forwardFFT;
};

template<typename Type>
auto HasLowerFftBinEnergyThan (juce::Array<Type> fftBinsValues) -> AudioBufferLowerEnergyFftBinsMatcher<Type>{
    return AudioBufferLowerEnergyFftBinsMatcher<Type>{fftBinsValues};
}
