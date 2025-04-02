#pragma once
#include "input.h"
#include "track.hpp"
#include "tinyfsm/tinyfsm.hpp"

#define DEFAULT_BRIGHTNESS 1
#define DEFAULT_VOLUME 50

// Shifted button functions
#define MODBTN_FILTER     BTN_STEP_9
#define MODBTN_AMP        BTN_STEP_10

enum LEDMode {
    LEDS_OVERRIDDEN,
    LEDS_SHOW_CHANNELS,
    LEDS_SHOW_STEPS
};


namespace UI {
    void init();
    bool process(RawInput in);


    struct DrawEvent : tinyfsm::Event {};
    struct InputEvent : tinyfsm::Event {};

    class UIFSM : public tinyfsm::Fsm<UIFSM> {
    public:
        void react(tinyfsm::Event const &) {};
        virtual void react(DrawEvent const & evt) {};
        virtual void react(InputEvent const & evt);
        virtual void entry() {react(DrawEvent {});};
        virtual void exit() {};
    };

    class Screensaver : public UIFSM {
        void entry() override;
        void exit() override;
        void react(InputEvent const &) override;
        void react(DrawEvent const &) override;        
    };

    class TrackView : public UIFSM {
        void react(InputEvent const &) override;
        void react(DrawEvent const &) override;
    };

    class ChannelsOverview : public UIFSM {
    public:
        void react(InputEvent const &) override;
        void react(DrawEvent const &) override;    
    };

    class ChannelView : public ChannelsOverview {
        // Input handled by ChannelsOverview
        void react(DrawEvent const &) override;
    };

    class PatternView : public UIFSM {
    public:
        void react(InputEvent const &) override;
        void react(DrawEvent const &) override;
    };

    class StepView : public PatternView {
        void react(InputEvent const &ievt) override;
        void react(DrawEvent const &) override;
    };

    class SampleSelector : public PatternView {
        // Input events handled by PatternView
        void react(DrawEvent const &) override;
    };
}