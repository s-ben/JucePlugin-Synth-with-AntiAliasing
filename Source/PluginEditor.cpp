/*
  ==============================================================================

    This file was auto-generated by the Jucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "SinewaveSynth.h"

// OSCOPE :::
class waveOutputUI : public Component,
                    public Timer
{
    
    Image historyImage;
    ScopedPointer<Graphics> historyGraphic;
    ScopedPointer<TextButton> viewWave;
    ScopedPointer<TextButton> viewMinBlep;
    ScopedPointer<Component> fftGraph;
    WaveGenPluginAudioProcessor* myWaveGen;
    
public:
    
    waveOutputUI (WaveGenPluginAudioProcessor* waveGen) {
        
        myWaveGen = waveGen;
        
        historyImage = Image(juce::Image::ARGB, 1000, 300, true);
        historyGraphic = new Graphics(historyImage);
        
        setOpaque(true);
        startTimer(2000);
        
        addAndMakeVisible( fftGraph = myWaveGen->createFFTGraph() );
        
        //FFTGraphView* fftGraphClass = dynamic_cast<FFTGraphView*>(fftGraph.get());
        //fftGraphClass->setFadeMultiple(0);
        
    }
    
    void timerCallback() override {
        
        repaint();
    }
    
    void paint(Graphics& g) override {
        
        
        g.fillAll(Colours::black);
        
        Path p;
        Range<float> range;
        
        // if (mySynth != nullptr) // sometimes we have a placeholder, before any object is focused ...
        {
            
            // VIEW OUTPUT
            const float* current =   myWaveGen->currentOutput.getReadPointer(0);
            int numSamples = myWaveGen->currentOutput.getNumSamples();
            
            bool sign = bool(current[0] > 0);
            int crossings = 0;
            for (int i=0; i < numSamples; i++)
            {
                bool currentSign = bool(current[i] > 0);
                
                if (currentSign != sign)
                {
                    sign = currentSign;
                    crossings++;
                    
                    if (crossings > 10)
                    {
                        numSamples = i;
                        break;
                    }
                }
            }
            
            
            // VIEW BLEP :::
            //            SubSynthVoice* v = (SubSynthVoice*)mySynth->getLastVoiceTriggered();
            //            Array<float> blep = v->getWaveGen(0)->getBlepGenerator()->getMinBlepArray();
            //            int numSamples = blep.size();
            //            float* current = blep.getRawDataPointer();
            
            // VIEW BLEP DER :::
            //            SubSynthVoice* v = (SubSynthVoice*)mySynth->getLastVoiceTriggered();
            //            Array<float> blep = v->getWaveGen(0)->getBlepGenerator()->getMinBlepDerivArray();
            //            int numSamples = blep.size();
            //            float* current = blep.getRawDataPointer();
            
            //range = FloatVectorOperations::findMinAndMax(blep.getRawDataPointer(), numSamples);
            
            for (int i=0; i < getWidth(); i++)
            {
                int scaledIndex = numSamples*float(i)/float(getWidth());
                float val = current[scaledIndex]; // invert here ... since coord are 0 at the top ...
                
                jassert(val == val);
                if (val != val) return;
                
                if (i == 0)
                {
                    p.startNewSubPath(0, 0.2);
                    p.startNewSubPath(0, -0.2);
                    p.startNewSubPath(0, val);
                }
                else p.lineTo(i, val);
            }
            
            // NOTE the maximum
            // max = FloatVectorOperations::findMaximum(history.getRawDataPointer(), history.size());
            
        }
        
        // Nan, etc ...
        if (p.isEmpty()) return;
        if (p.getBounds().getWidth() < .01) return;
        if (p.getBounds().getWidth() != p.getBounds().getWidth()) return;
        
        
        
        // RESCALE ::::: to fill the space ...
        p.scaleToFit(5, 5, historyImage.getWidth()-10, historyImage.getHeight()/2-10, false); // this .. effectively .. traslates too
        
        historyImage.clear(historyImage.getBounds(), Colours::black);
        
        
        // GRADIENT ::::
        Colour c1 = Colours::grey;
        Colour c2 = Colours::lightgrey;
        
        ColourGradient grad(c1, historyImage.getWidth()/2, historyImage.getHeight()/2,
                            c2, 0, historyImage.getHeight()/2, true
                            );
        
        historyGraphic->setGradientFill( grad );
        historyGraphic->strokePath(p, PathStrokeType(1.1));
        
        
        // SCALE p to the historyImage
        g.drawImage(historyImage, 1, 1, getWidth()-2, getHeight()-2, 0, 0, historyImage.getWidth(), historyImage.getHeight());
        
        
        
    }
    
    void resized() override {
        
        fftGraph->setBounds(0, getHeight()/2, getWidth(), getHeight()/2);
        
    }
};



//==============================================================================
// This is a handy slider subclass that controls an AudioProcessorParameter
// (may move this class into the library itself at some point in the future..)
class WaveGenPluginAudioProcessorEditor::ParameterSlider   : public Slider,
                                                              private Timer
{
public:
    ParameterSlider (AudioProcessorParameter& p)
        : Slider (p.getName (256)), param (p)
    {
        setRange (0.0, 1.0, 0.0);
        startTimerHz (30);
        updateSliderPos();
    }

    void valueChanged() override
    {
        if (isMouseButtonDown())
            param.setValueNotifyingHost ((float) Slider::getValue());
        else
            param.setValue ((float) Slider::getValue());
    }

    void timerCallback() override       { updateSliderPos(); }

    void startedDragging() override     { param.beginChangeGesture(); }
    void stoppedDragging() override     { param.endChangeGesture();   }

    double getValueFromText (const String& text) override   { return param.getValueForText (text); }
    String getTextFromValue (double value) override         { return param.getText ((float) value, 1024); }

    void updateSliderPos()
    {
        const float newValue = param.getValue();

        if (newValue != (float) Slider::getValue() && ! isMouseButtonDown())
            Slider::setValue (newValue);
    }

    AudioProcessorParameter& param;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterSlider)
};

//==============================================================================
WaveGenPluginAudioProcessorEditor::WaveGenPluginAudioProcessorEditor (WaveGenPluginAudioProcessor& owner)
    : AudioProcessorEditor (owner),
      midiKeyboard (owner.keyboardState, MidiKeyboardComponent::horizontalKeyboard),
      timecodeDisplayLabel (String()),
      gainLabel (String(), "Throughput level:"),
      toneLabel (String(), "Tone"),
      depthLabel (String(), "Overtones"),
      PWMLabel (String(), "Shape"),
      delayLabel (String(), "Delay:")
{

    // add some sliders..
    addAndMakeVisible (gainSlider = new ParameterSlider (*owner.gainParam));
    gainSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    
    addAndMakeVisible (delaySlider = new ParameterSlider (*owner.delayParam));
    delaySlider->setSliderStyle (Slider::Rotary);

    // add some labels for the sliders..
    gainLabel.attachToComponent (gainSlider, false);
    gainLabel.setFont (Font (11.0f));
    
    delayLabel.attachToComponent (delaySlider, false);
    delayLabel.setFont (Font (11.0f));

    
    addAndMakeVisible (waveType = new ComboBox ("wave type"));
    waveType->setJustificationType (Justification::centredLeft);
    waveType->setTextWhenNothingSelected ("- wave type -");
    waveType->addItem("sine", 1);
    waveType->addItem("square", 2);
    waveType->addItem("saw rise", 3);
    waveType->addItem("saw fall", 4);
    waveType->addListener(this);
    waveType->setSelectedId(2);
    
    addAndMakeVisible (tone = new Slider ("tone"));
    tone->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    tone->addListener(this);
    tone->setRange(0, 24);
    
    addAndMakeVisible (pwm = new Slider ("PWM"));
    pwm->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    pwm->addListener(this);
    pwm->setRange(-100, 100);
    
    addAndMakeVisible (overtoneDepth = new Slider ("overtones"));
    overtoneDepth->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    overtoneDepth->addListener(this);
    overtoneDepth->setRange(4, 128);
    
    PWMLabel.attachToComponent (pwm, false);
    PWMLabel.setFont (Font (11.0f));
    
    toneLabel.attachToComponent (tone, false);
    toneLabel.setFont (Font (11.0f));
    
    depthLabel.attachToComponent (overtoneDepth, false);
    depthLabel.setFont (Font (11.0f));
    
    
    // add the midi keyboard component..
    addAndMakeVisible (midiKeyboard);
    
    // the output waveform and FFT
    addAndMakeVisible( outputUI = new waveOutputUI(&owner) );
    
    // add a label that will display the current timecode and status..
    addAndMakeVisible (timecodeDisplayLabel);
    timecodeDisplayLabel.setColour (Label::textColourId, Colours::blue);
    timecodeDisplayLabel.setFont (Font (Font::getDefaultMonospacedFontName(), 15.0f, Font::plain));

    // set resize limits for this plug-in
    setResizeLimits (400, 200, 800, 600);

    // set our component's initial size to be the last one that was stored in the filter's settings
    setSize (600, 600);

    // start a timer which will keep our timecode display updated
    startTimerHz (30);
}

WaveGenPluginAudioProcessorEditor::~WaveGenPluginAudioProcessorEditor()
{
}

//==============================================================================
void WaveGenPluginAudioProcessorEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Colours::white, 0, 0,
                                       Colours::lightgrey, 0, (float) getHeight(), false));
    g.fillAll();
}

void WaveGenPluginAudioProcessorEditor::resized()
{
    // This lays out our child components...

    Rectangle<int> r (getLocalBounds().reduced (8));

    timecodeDisplayLabel.setBounds (r.removeFromTop (26));
    midiKeyboard.setBounds (r.removeFromBottom (70));

    r.removeFromTop (30);
    Rectangle<int> sliderArea (r.removeFromTop (50));
    gainSlider->setBounds (sliderArea.removeFromLeft (jmin (180, sliderArea.getWidth() / 2)));
    
    // DELAY is disabled ....
    // delaySlider->setBounds (sliderArea.removeFromLeft (jmin (180, sliderArea.getWidth())));
    delaySlider->setVisible(false);
    
    // sliders ...
    waveType->setBounds(gainSlider->getRight() + 4, 35, 100, 20);
    tone->setBounds(gainSlider->getRight() + 4, waveType->getBottom() + 20, 80, 30);
    pwm->setBounds(tone->getRight() + 4, waveType->getBottom() + 20, 80, 30);
    overtoneDepth->setBounds(pwm->getRight() + 4, waveType->getBottom() + 20, 80, 30);
    
    
    float y = gainSlider->getBottom() + 4;
    outputUI->setBounds(4, y, getWidth()-8, getHeight() - y - 78);
    
    getProcessor().lastUIWidth = getWidth();
    getProcessor().lastUIHeight = getHeight();
    
}

//==============================================================================
void WaveGenPluginAudioProcessorEditor::timerCallback()
{
    updateTimecodeDisplay (getProcessor().lastPosInfo);
}


void WaveGenPluginAudioProcessorEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    
    if (comboBoxThatHasChanged == waveType)
    {
        WaveGenPluginAudioProcessor& waveGenProc = static_cast<WaveGenPluginAudioProcessor&> (processor);
        Synthesiser* synth =  waveGenProc.getSynth();
        
        WaveGenerator::WaveType newtype = WaveGenerator::sine;
        int id = waveType->getSelectedId();
        
        if (id == 1) newtype = WaveGenerator::sine;
        else if (id == 2) newtype = WaveGenerator::square;
        else if (id == 3) newtype = WaveGenerator::sawRise;
        else if (id == 4) newtype = WaveGenerator::sawFall;
        else jassertfalse;
        
        for (int v=0; v < synth->getNumVoices(); v++)
        {
            SineWaveVoice* sineVoice = static_cast<SineWaveVoice*>(synth->getVoice(v));
            WaveGenerator* waveGen = sineVoice->getWaveGenerator();
            waveGen->setWaveType(newtype);
        }
        
    }
    
    
}

void WaveGenPluginAudioProcessorEditor::sliderValueChanged(Slider* sliderThatWasMoved) {
    
    //
//    if (sliderThatWasMoved == tone)
//    {
//        double newTone = tone->getValue();
//        voiceWaveform->setToneOffset(newTone);
//    }
//    else if (sliderThatWasMoved == oscOvertones)
//    {
//        double newNumberOfOvertones = oscOvertones->getValue();
//        mySynth->Parameters[AddictionSynthAP::V1_OVERTONE_DEPTH + selectedVoice]->setParamValue(newNumberOfOvertones);
//        
//    }
//    else if (sliderThatWasMoved == AAFreqSlider)
//    {
//        double val = AAFreqSlider->getValue();
//        //mySynth->setVoiceAAMultiple(selectedVoice, val);
//        
//        SubSynthVoice* v = mySynth->getLastVoiceTriggered();
//        
//        if (v)
//        {
//            v->getWaveGen(0)->setTest(val);
//        }
//    }
    
    
    if (sliderThatWasMoved == tone)
    {
        WaveGenPluginAudioProcessor& waveGenProc = static_cast<WaveGenPluginAudioProcessor&> (processor);
        Synthesiser* synth =  waveGenProc.getSynth();

        int toneOffsetSemis = tone->getValue();
        
        for (int v=0; v < synth->getNumVoices(); v++)
        {
            SineWaveVoice* sineVoice = static_cast<SineWaveVoice*>(synth->getVoice(v));
            WaveGenerator* waveGen = sineVoice->getWaveGenerator();
            waveGen->setToneOffset(toneOffsetSemis);
        }
    }
    else if (sliderThatWasMoved == pwm)
    {
        WaveGenPluginAudioProcessor& waveGenProc = static_cast<WaveGenPluginAudioProcessor&> (processor);
        Synthesiser* synth =  waveGenProc.getSynth();
        
        float skewPercentage = (float)pwm->getValue()/100.f;
        
        for (int v=0; v < synth->getNumVoices(); v++)
        {
            SineWaveVoice* sineVoice = static_cast<SineWaveVoice*>(synth->getVoice(v));
            WaveGenerator* waveGen = sineVoice->getWaveGenerator();
            waveGen->setSkew(skewPercentage);
        }
    }
    else if (sliderThatWasMoved == overtoneDepth)
    {
        
        WaveGenPluginAudioProcessor& waveGenProc = static_cast<WaveGenPluginAudioProcessor&> (processor);
        Synthesiser* synth =  waveGenProc.getSynth();
        
        float multiple = (float)overtoneDepth->getValue();
        
        for (int v=0; v < synth->getNumVoices(); v++)
        {
            // Get the synth voice ... it should be a square wave voice ....
            SineWaveVoice* sineVoice = static_cast<SineWaveVoice*>(synth->getVoice(v));
            WaveGenerator* waveGen = sineVoice->getWaveGenerator();
            waveGen->setBlepOvertoneDepth(multiple);
        }
    }
    
}


//==============================================================================
// quick-and-dirty function to format a timecode string
static String timeToTimecodeString (double seconds)
{
    const int millisecs = roundToInt (seconds * 1000.0);
    const int absMillisecs = std::abs (millisecs);

    return String::formatted ("%02d:%02d:%02d.%03d",
                              millisecs / 360000,
                              (absMillisecs / 60000) % 60,
                              (absMillisecs / 1000) % 60,
                              absMillisecs % 1000);
}

// quick-and-dirty function to format a bars/beats string
static String quarterNotePositionToBarsBeatsString (double quarterNotes, int numerator, int denominator)
{
    if (numerator == 0 || denominator == 0)
        return "1|1|000";

    const int quarterNotesPerBar = (numerator * 4 / denominator);
    const double beats  = (fmod (quarterNotes, quarterNotesPerBar) / quarterNotesPerBar) * numerator;

    const int bar    = ((int) quarterNotes) / quarterNotesPerBar + 1;
    const int beat   = ((int) beats) + 1;
    const int ticks  = ((int) (fmod (beats, 1.0) * 960.0 + 0.5));

    return String::formatted ("%d|%d|%03d", bar, beat, ticks);
}

// Updates the text in our position label.
void WaveGenPluginAudioProcessorEditor::updateTimecodeDisplay (AudioPlayHead::CurrentPositionInfo pos)
{
    MemoryOutputStream displayText;

    displayText << "[" << SystemStats::getJUCEVersion() << "]   "
                << String (pos.bpm, 2) << " bpm, "
                << pos.timeSigNumerator << '/' << pos.timeSigDenominator
                << "  -  " << timeToTimecodeString (pos.timeInSeconds)
                << "  -  " << quarterNotePositionToBarsBeatsString (pos.ppqPosition,
                                                                    pos.timeSigNumerator,
                                                                    pos.timeSigDenominator);

    if (pos.isRecording)
        displayText << "  (recording)";
    else if (pos.isPlaying)
        displayText << "  (playing)";

    timecodeDisplayLabel.setText (displayText.toString(), dontSendNotification);
}
