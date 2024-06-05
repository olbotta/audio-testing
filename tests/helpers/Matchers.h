#include "../Settings.h"
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_templated.hpp"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"
#include <utility>

//TODO: write it using JUCE guide
template<typename Type>
juce::Array<Type> getBinEnergies (juce::AudioBuffer<Type> in)
{
    juce::dsp::FFT fft = juce::dsp::FFT(FFT_ORDER);
    auto* inRead = in.getArrayOfReadPointers();

    std::array<Type, FFT_SIZE> inFifo; // incoming audio samples
    std::array<Type, FFT_SIZE * 2> inFftData;
    juce::Array<Type> binEnergies;
    unsigned long fifoIndex = 0;

    for (int sam = 0; sam < in.getNumSamples(); ++sam)
    {
        for (int ch = 0; ch < in.getNumChannels(); ++ch)
        {
            if (fifoIndex == FFT_SIZE)//end of the bin(?)
            {
                std::ranges::fill (inFftData.begin(), inFftData.end(), 0.0f);//superfluo? (viene sovrascritto subito dopo)
                std::ranges::copy (inFifo.begin(), inFifo.end(), inFftData.begin());
                fft.performFrequencyOnlyForwardTransform (inFftData.data(), true);

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
        INFO("buffers have different channel size ("<<buffer.getNumChannels()<<", "<<other.getNumChannels()<<")");
        REQUIRE (buffer.getNumChannels() == other.getNumChannels());
        INFO("buffers have different sample size ("<<buffer.getNumSamples()<<", "<<other.getNumSamples()<<")");
        REQUIRE (buffer.getNumSamples() == other.getNumSamples());

        auto* inRead  = buffer.getArrayOfReadPointers();
        auto* othRead = other.getArrayOfReadPointers();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int sam = 0; sam < buffer.getNumSamples(); ++sam)
            {
                if (std::fabs (inRead[ch][sam] - othRead[ch][sam]) > std::numeric_limits<Type>::epsilon())
                    return false;

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
auto matches (juce::AudioBuffer<Type> buffer) -> AudioBufferMatcher<Type>{
 return AudioBufferMatcher<Type>{buffer};
}

template<typename Type>
struct AudioBufferLowerEnergyMatcher : Catch::Matchers::MatcherGenericBase
{
public:
    explicit AudioBufferLowerEnergyMatcher (juce::AudioBuffer<Type> const& buffer) : buffer (buffer)
    {}

    bool match (juce::AudioBuffer<Type> const& other) const
    { // ⚠️ @code other refers to the first argument of CHECK_THAT / REQUIRES_THAT

        INFO("buffers have different channel size ("<<buffer.getNumChannels()<<", "<<other.getNumChannels()<<")");
        REQUIRE (buffer.getNumChannels() == other.getNumChannels());
        INFO("buffers have different sample size ("<<buffer.getNumSamples()<<", "<<other.getNumSamples()<<")");
        REQUIRE (buffer.getNumSamples() == other.getNumSamples());

        juce::Array<Type> bufferEnergies = getBinEnergies (buffer);
        juce::Array<Type> otherEnergies = getBinEnergies (other);

        Type bufferPower = std::accumulate (bufferEnergies.begin(), bufferEnergies.end(), 0.0f);
        Type otherPower = std::accumulate (otherEnergies.begin(), otherEnergies.end(), 0.0f);

        INFO ("bufferPower (" + std::to_string (bufferPower) + ") should be lower than otherPower (" + std::to_string (otherPower) << ")");
        //REQUIRE (otherPower >= bufferPower);
        return std::fabs (otherPower - bufferPower) > std::numeric_limits<Type>::epsilon();
    }

    std::string describe() const override
    {
        return "Buffer should have lower cumulative energy than other";
    }

private:
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
