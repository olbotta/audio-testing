#include "Settings.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>

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

class AudioBufferMatcher : public Catch::Matchers::MatcherBase<juce::AudioBuffer<float>>
{
    juce::AudioBuffer<float> otherBuffer;

public:
    AudioBufferMatcher (juce::AudioBuffer<float> const& other) : otherBuffer (other) {}

    bool match (juce::AudioBuffer<float> const& in) const override
    {
        //return in >= m_begin && in <= m_end;
        auto* inRead = in.getArrayOfReadPointers();
        auto* othRead = otherBuffer.getArrayOfReadPointers();

        if (in.getNumChannels() != otherBuffer.getNumChannels() || in.getNumSamples() != otherBuffer.getNumSamples())
        {
            return false;
        }

        for (int ch = 0; ch < in.getNumChannels(); ++ch)
        {
            for (int sam = 0; sam < in.getNumSamples(); ++sam)
            {
                if (std::fabs (inRead[ch][sam] - othRead[ch][sam]) > std::numeric_limits<float>::epsilon())
                {
                    return false;
                }
            }
        }

        return true;
    }

    std::string describe() const override
    {
        std::ostringstream ss;
        ss << "AudioBuffers are the same ";
        return ss.str();
    }
};

AudioBufferMatcher AudioBuffersMatch (juce::AudioBuffer<float> other)
{
    return { other };
}

class AudioBufferEnergyMatcher : public Catch::Matchers::MatcherBase<juce::AudioBuffer<float>>
{
    juce::AudioBuffer<float> otherBuffer;

public:
    AudioBufferEnergyMatcher (juce::AudioBuffer<float> other) : otherBuffer (other), forwardFFT (fft_order), window { fft_size, juce::dsp::WindowingFunction<float>::WindowingMethod::hann } {}

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

        if (inPower < othPower)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    std::string describe() const override
    {
        std::ostringstream ss;
        ss << "Other buffer has higher total energy";
        return ss.str();
    }

private:
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
};

AudioBufferEnergyMatcher AudioBufferHigherEnergy (juce::AudioBuffer<float> other)
{
    return { other };
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
