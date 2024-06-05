#include "../Settings.h"
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_templated.hpp"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"
#include <utility>
#include "ImageProcessing.h"

template<typename Type, size_t fftSize>
juce::Array<Type> getBinEnergies (juce::AudioBuffer<Type> buffer)
{
    auto fft = juce::dsp::FFT (fftSize);
    std::array<Type, fftSize * 2> blockCopy;
    //std::array<Type, fftSize> binEnergies;
    juce::Array<Type> binEnergies;

        //auto inRead = buffer.getArrayOfReadPointers();
    auto bufferCopy = juce::AudioBuffer<Type>(buffer);
    int blockCount = buffer.getNumSamples() / fftSize;

    for (int blockBegin = 0; blockBegin < blockCount; blockBegin += fftSize)
    {
        std::ranges::fill (blockCopy.begin(), blockCopy.end(), 0.0f); // reset the in/out
        for (int i = blockBegin; i < fftSize; i++) //copy in buffer
            blockCopy[i] = bufferCopy.getSample(1,i);//TODO: optimize

        fft.performFrequencyOnlyForwardTransform (blockCopy.data(), true);

        for (unsigned long bin = 0; bin < blockCopy.size() / 2; bin++) // we only care about the first half ot the output
        {
            //binEnergies[bin] += blockCopy[bin];
            binEnergies.set(bin,binEnergies[bin] + blockCopy[bin]);
        }

        //std::transform(blockCopy.begin(), blockCopy.end(), binEnergies.begin(), std::plus<>());

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

        juce::Array<Type> bufferEnergies = getBinEnergies<Type,FFT_SIZE> (buffer);
        juce::Array<Type> otherEnergies = getBinEnergies<Type,FFT_SIZE> (other);

        Type bufferPower = std::accumulate (bufferEnergies.begin(), bufferEnergies.end(), 0.0f);
        Type otherPower = std::accumulate (otherEnergies.begin(), otherEnergies.end(), 0.0f);

        std::cout<<"bufferPower (" + std::to_string (bufferPower) + ") should be lower than otherPower (" + std::to_string (otherPower) << ")\n";
        UNSCOPED_INFO ("bufferPower (" + std::to_string (bufferPower) + ") should be lower than otherPower (" + std::to_string (otherPower) << ")");
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
    explicit AudioBufferLowerEnergyFftBinsMatcher (juce::AudioBuffer<Type> buffer,  juce::IteratorPair<int,int> range ) :
                                                                                                               buffer(buffer),
                                                                                                               range(range)
    {}

    bool match (juce::AudioBuffer<Type> const& other) const
    {
        juce::Array<Type> bufferBinEnergies = getBinEnergies<Type,FFT_SIZE> (buffer);
        juce::Array<Type> otherBinEnergies = getBinEnergies<Type,FFT_SIZE> (other);

        {
            INFO ("endBin (" << range.end() << ") should be <= than number of bins calculated by fft(" << bufferBinEnergies.size() << ")");
            REQUIRE (range.end() <= bufferBinEnergies.size());
        }

        for (int i = range.begin(); i <= range.end(); i++) {

            UNSCOPED_INFO("Buffer should have lower energy than other in each selected bins, got "<<bufferBinEnergies[i]<<" < "<<otherBinEnergies[i]);
            //REQUIRE(otherBinEnergies[i] < bufferBinEnergies[i]);

            if (bufferBinEnergies[i] < otherBinEnergies[i])
                return false;
        }
        return true;
    }

    std::string describe() const override { return "Buffer should have lower energies in the selected bins";}

private:
    juce::AudioBuffer<Type> buffer;
    juce::IteratorPair<int, int> range;
};

template<typename Type>
auto HasLowerFftBinEnergyThan (juce::AudioBuffer<Type> buffer, juce::IteratorPair<int,int> range = juce::makeRange(0,FFT_SIZE/2)) -> AudioBufferLowerEnergyFftBinsMatcher<Type>{
    return AudioBufferLowerEnergyFftBinsMatcher<Type>{buffer,range};
}