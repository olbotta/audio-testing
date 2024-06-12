#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "Parameters.h"
#include <cmath>

class DryWet
{
public:
    DryWet() = default;
    ~DryWet() = default;

    void prepareToPlay (double sr, int maxNumSamples)
    {
        drySignal.setSize (2, maxNumSamples);

        smoothedDryLevel.reset (sr, SMOOTHING_TIME);
        smoothedWetLevel.reset (sr, SMOOTHING_TIME);
        setDryWetRatio (dryWetRatio);
    }

    void releaseResources()
    {
        drySignal.setSize (0, 0);
    }

    void setDry (const juce::AudioBuffer<float>& buffer)
    {
        //questo ciclo strano è per chiamare getnumchannels solo una volta, e non ad ogni confronto del for
        // quindi bisogna fare quella magia con il --ch
        for (int ch = buffer.getNumChannels(); --ch >= 0;)
            drySignal.copyFrom (ch, 0, buffer, ch, 0, buffer.getNumSamples());
    }

    void merge (juce::AudioBuffer<float>& wetBuffer)
    {
        auto numSamples = wetBuffer.getNumSamples();

        smoothedWetLevel.applyGain (wetBuffer, numSamples); //v3
        smoothedDryLevel.applyGain (drySignal, numSamples); //v3 oss: scrive numSamples! non drySignal.getNumSamples()

        for (int ch = wetBuffer.getNumChannels(); --ch >= 0;) //ciclo strano di presti che scorre i canali al contrario
        {//siamo sicuri che al massimo abbassi l'energia?
            wetBuffer.addFrom (ch, 0, drySignal, ch, 0, numSamples);
        }
    }

    void setDryWetRatio (const float newValue)
    {
        dryWetRatio = newValue;
        //smoothedWetLevel.setCurrentAndTargetValue (sqrt (1 - dryWetRatio));
        //smoothedDryLevel.setCurrentAndTargetValue (sqrt (dryWetRatio));
        smoothedWetLevel.setCurrentAndTargetValue (1 - dryWetRatio);
        smoothedDryLevel.setCurrentAndTargetValue (dryWetRatio);

        //smoothedWetLevel.setTargetValue (1 - dryWetRatio);
        //smoothedDryLevel.setTargetValue (dryWetRatio);
        //smoothedWetLevel.setTargetValue (sqrt (1 - dryWetRatio));
        //smoothedDryLevel.setTargetValue (sqrt (dryWetRatio));
    }

private:
    float dryWetRatio = 0.5; // 1 = completely dry, 0 completely wet
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDryLevel;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedWetLevel;

    juce::AudioBuffer<float> drySignal;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DryWet)
};
