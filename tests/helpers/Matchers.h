#include "../Settings.h"
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_templated.hpp"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"
#include <utility>


template<typename Type>
juce::Array<Type> getBinEnergies (juce::AudioBuffer<Type> in)
{
    juce::dsp::FFT fft = juce::dsp::FFT(FFT_ORDER);
    auto* inRead = in.getArrayOfReadPointers();

    std::array<Type, FFT_SIZE> inFifo; // incoming audio samples
    std::array<Type, FFT_SIZE * 2> inFftData;
    juce::Array<Type> binEnergies; //of lenght FFT_SIZE
    unsigned long fifoIndex = 0;

    for (int sam = 0; sam < in.getNumSamples(); ++sam)
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
            inFifo[fifoIndex] = inRead[0][sam];
            fifoIndex++;

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
        {
            INFO ("buffers have different channel size (" << buffer.getNumChannels() << ", " << other.getNumChannels() << ")");
            REQUIRE (buffer.getNumChannels() == other.getNumChannels());
        }
        {
            INFO ("buffers have different sample size (" << buffer.getNumSamples() << ", " << other.getNumSamples() << ")");
            REQUIRE (buffer.getNumSamples() == other.getNumSamples());
        }
        auto* inRead  = buffer.getArrayOfReadPointers();
        auto* othRead = other.getArrayOfReadPointers();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int sam = 0; sam < buffer.getNumSamples(); ++sam)
            {
                if (std::fabs (inRead[ch][sam] - othRead[ch][sam]) > EQUALITY_TOLERANCE)
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
struct AudioBufferLowerCumulativeEnergyMatcher : Catch::Matchers::MatcherGenericBase
{
public:
    explicit AudioBufferLowerCumulativeEnergyMatcher (juce::AudioBuffer<Type> const& buffer) : buffer (buffer)
    {}

    bool match (juce::AudioBuffer<Type> const& other) const
    { // ⚠️ @code other refers to the first argument of CHECK_THAT / REQUIRES_THAT

        juce::Array<Type> bufferEnergies = getBinEnergies<Type> (buffer);
        juce::Array<Type> otherEnergies = getBinEnergies<Type> (other);

        Type bufferPower = std::accumulate (bufferEnergies.begin(), bufferEnergies.end(), 0.0f);
        Type otherPower = std::accumulate (otherEnergies.begin(), otherEnergies.end(), 0.0f);

        UNSCOPED_INFO ("bufferPower (" + std::to_string (bufferPower) + ") should be lower than otherPower (" + std::to_string (otherPower) << ")");
        return bufferPower - otherPower > std::numeric_limits<Type>::epsilon();
    }

    std::string describe() const override
    {
        return "Buffer should have lower cumulative energy than other";
    }

private:
    juce::AudioBuffer<Type> buffer;
};

template<typename Type>
auto hasLowerCumulativeEnergyThan (juce::AudioBuffer<Type> buffer) -> AudioBufferLowerCumulativeEnergyMatcher<Type>{
    return AudioBufferLowerCumulativeEnergyMatcher<Type>{buffer};
}

template<typename Type>
struct AudioBufferLowerEnergyFreqBinsMatcher : public Catch::Matchers::MatcherGenericBase
{
public:
    explicit AudioBufferLowerEnergyFreqBinsMatcher (juce::AudioBuffer<Type> buffer,  juce::IteratorPair<int,int> range ) :
                                                                                                               buffer(buffer),
                                                                                                               range(range)
    {}

    bool match (juce::AudioBuffer<Type> const& other) const
    {
        juce::Array<Type> bufferBinEnergies = getBinEnergies<Type> (buffer);
        juce::Array<Type> otherBinEnergies = getBinEnergies<Type> (other);//effected

        {
            INFO ("endBin (" << range.end() << ") should be <= than number of bins calculated by fft(" << bufferBinEnergies.size() << ")");
            REQUIRE (range.end() <= bufferBinEnergies.size());
        }

        for (int bin = range.begin(); bin <= range.end(); bin++)
        {
                UNSCOPED_INFO ("Buffer should have lower energy than other in each selected bin, got "
                               <<": "<<bufferBinEnergies[bin]<< "-"<<otherBinEnergies[bin]<<"= "<<bufferBinEnergies[bin] - otherBinEnergies[bin]<<" in bin "<<bin);
                //REQUIRE(bufferBinEnergies[bin] - otherBinEnergies[bin] > - FILTER_TOLERANCE);

                if (bufferBinEnergies[bin] - otherBinEnergies[bin] <= -FILTER_TOLERANCE)
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
auto hasLowerFftBinEnergyThan (juce::AudioBuffer<Type> buffer, juce::IteratorPair<int,int> range = juce::makeRange(0,FFT_SIZE/2)) -> AudioBufferLowerEnergyFreqBinsMatcher<Type>{
    return AudioBufferLowerEnergyFreqBinsMatcher<Type>{buffer,range};
}

template<typename Type>
struct AudioBufferFrequenciesMatcher : public Catch::Matchers::MatcherGenericBase
{
public:
    explicit AudioBufferFrequenciesMatcher (juce::AudioBuffer<Type> buffer,  juce::IteratorPair<int,int> range ) :
                                                                                                                  buffer(buffer),
                                                                                                                  range(range)
    {}

    bool match (juce::AudioBuffer<Type> const& other) const
    {
        juce::Array<Type> bufferBinEnergies = getBinEnergies<Type> (buffer);
        juce::Array<Type> otherBinEnergies = getBinEnergies<Type> (other);//effected

        {
            INFO ("endBin (" << range.end() << ") should be <= than number of bins calculated by fft(" << bufferBinEnergies.size() << ")");
            REQUIRE (range.end() <= bufferBinEnergies.size());
        }

        for (int bin = range.begin(); bin <= range.end(); bin++)
        {
            UNSCOPED_INFO ("Buffer should have same magnitude in the selected frequency bins, got "
                           <<": abs("<<bufferBinEnergies[bin]<< "-"<<otherBinEnergies[bin]<<")= "<<abs(bufferBinEnergies[bin] - otherBinEnergies[bin])<<" in bin "<<bin);

            if (std::fabs(bufferBinEnergies[bin] - otherBinEnergies[bin]) > FILTER_TOLERANCE*5)
                return false;

        }
        return true;
    }

    std::string describe() const override { return "Buffer should have same magnitude in the selected frequency bins";}

private:
    juce::AudioBuffer<Type> buffer;
    juce::IteratorPair<int, int> range;
};

template<typename Type>
auto matchesFrequencyBinsOf (juce::AudioBuffer<Type> buffer, juce::IteratorPair<int,int> range = juce::makeRange(0,FFT_SIZE/2)) -> AudioBufferFrequenciesMatcher<Type>{
    return AudioBufferFrequenciesMatcher<Type>{buffer,range};
}