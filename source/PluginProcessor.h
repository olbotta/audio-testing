#pragma once
#include "DryWet.h"
#include "Parameters.h"
#include <juce_dsp/juce_dsp.h>
//#include <juce_audio_processors/juce_audio_processors.h>
//#include <juce_data_structures/juce_data_structures.h>


class FilterAudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    FilterAudioProcessor();
    ~FilterAudioProcessor() override  = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor * createEditor() override {return nullptr;}

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState* getParameters();

private:
    void parameterChanged(const juce::String& paramID, float newValue) override;

    juce::AudioProcessorValueTreeState parameters;
    DryWet drywetter;
    float cutoff;
    juce::dsp::StateVariableTPTFilter<float> tptFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::StateVariableTPTFilter<float>, juce::dsp::IIR::Coefficients <float>> filter;
    double fs;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterAudioProcessor)
};
