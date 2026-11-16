// Smoke test: capture the template's real JUCE AudioProcessorEditor.
// Compare every shared Studio UI skin at multiple sizes, never a mockup.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <SkellCore/HeadlessUIRenderer.h>
#include <SkellCore/StudioUI.h>
#include <SkellCore/GuidedTour.h>
#include <SkellCore/FontSystem.h>
#include <JuceHeader.h>
#include <cstdio>

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI juceGui;
    SkellAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    SkellAudioProcessorEditor editor(processor);

    // A render-only JUCE label in the real editor, used for font QA only.
    juce::Label unicodeProbe;
    unicodeProbe.setText(juce::String::fromUTF8(u8"SKELL  ✓  ♪  →  ⚙  ★  ☠  🎹  🩻"),
                         juce::dontSendNotification);
    unicodeProbe.setFont(skellcore::ui::FontSystem::make(20.f,
                            skellcore::ui::FontRole::Emoji));
    unicodeProbe.setJustificationType(juce::Justification::centred);
    unicodeProbe.setComponentID("skellcore.font-emoji-probe");
    editor.addAndMakeVisible(unicodeProbe);
    unicodeProbe.setVisible(false);

    // Real JUCE Unicode shaping and preferred symbol/emoji font smoke check.
    const auto font = skellcore::ui::FontSystem::make(17.0f, skellcore::ui::FontRole::Emoji);
    if (!font.getFallbackEnabled()) {
        std::fprintf(stderr, "Shared emoji font has fallback disabled\n");
        return 1;
    }
    juce::Image glyphProbe(juce::Image::ARGB, 360, 48, true);
    juce::Graphics glyphGraphics(glyphProbe);
    glyphGraphics.setFont(font);
    glyphGraphics.setColour(juce::Colours::white);
    glyphGraphics.drawSingleLineText(juce::String::fromUTF8("♪ ✓ → ⚙ 🎹"), 8, 30);

    // Test converted coordinate handling through a nested JUCE component
    // and a per-step panel activation callback without touching plugin DSP.
    juce::Component geometryRoot, nestedPanel;
    juce::TextButton nestedButton { "Nested Target" }, hiddenButton { "Hidden Tab" };
    geometryRoot.setSize(600, 400);
    geometryRoot.setVisible(true);
    geometryRoot.addAndMakeVisible(nestedPanel);
    nestedPanel.setBounds(50, 60, 260, 140);
    nestedPanel.addAndMakeVisible(nestedButton);
    nestedButton.setBounds(20, 30, 90, 27);
    geometryRoot.addAndMakeVisible(hiddenButton);
    hiddenButton.setBounds(340, 230, 120, 28);
    hiddenButton.setVisible(false);
    std::vector<skellcore::ui::GuidedTourStep> geometrySteps;
    geometrySteps.emplace_back(&nestedButton, "NESTED", "A control inside its own panel");
    geometrySteps.emplace_back(&hiddenButton, "OTHER TAB", "The correct page becomes visible",
                                [&hiddenButton] { hiddenButton.setVisible(true); });
    skellcore::ui::GuidedTourOverlay geometryTour(
        std::move(geometrySteps), {}, [](skellcore::ui::GuidedTourFinish){});
    geometryRoot.addAndMakeVisible(geometryTour);
    geometryTour.setBounds(geometryRoot.getLocalBounds());
    geometryTour.begin();
    // Navigation must use its own reliable LookAndFeel; Spectre and Silence
    // have editor-wide button painters that make NEXT dark-on-dark otherwise.
    bool nextHasIsolatedLookAndFeel = false;
    for (int i = 0; i < geometryTour.getNumChildComponents(); ++i) {
        if (auto* button = dynamic_cast<juce::TextButton*>(geometryTour.getChildComponent(i))) {
            if (button->getButtonText() == "NEXT")
                nextHasIsolatedLookAndFeel =
                    &button->getLookAndFeel() != &geometryRoot.getLookAndFeel();
        }
    }
    if (!nextHasIsolatedLookAndFeel) {
        std::fprintf(stderr,"GuidedTour NEXT button inherited host LookAndFeel\\n");
        return 1;
    }
    const auto actualTarget = geometryTour.spotlightBounds();
    if (actualTarget != juce::Rectangle<int>(64, 84, 102, 39)) {
        std::fprintf(stderr, "GuidedTour nested spotlight: actual %d,%d %dx%d; expected 64,84 102x39\\n",
            actualTarget.getX(), actualTarget.getY(),
            actualTarget.getWidth(), actualTarget.getHeight());
        return 1;
    }
    geometryTour.advance();
    if (geometryTour.stepIndex() != 1 || !hiddenButton.isVisible()
        || geometryTour.spotlightBounds().isEmpty()) {
        std::fprintf(stderr, "GuidedTour did not reveal or locate the next tab\n");
        return 1;
    }
    geometryTour.previous();
    if (geometryTour.stepIndex() != 0) {
        std::fprintf(stderr, "GuidedTour BACK navigation failed\n");
        return 1;
    }
    // Regression: when a user maximizes the real plugin editor, the tutorial
    // must grow with it instead of leaving 11px text on an 1800px canvas.
    const auto compactBubble = geometryTour.informationBounds();
    geometryRoot.setSize(1800,1140);
    geometryTour.setBounds(geometryRoot.getLocalBounds());
    const auto enlargedBubble = geometryTour.informationBounds();
    if (compactBubble.getWidth() > 355
        || enlargedBubble.getWidth() < 500
        || enlargedBubble.getHeight() < 195
        || !geometryTour.getLocalBounds().contains(enlargedBubble)) {
        std::fprintf(stderr,"GuidedTour large-editor text card did not scale or fit\\n");
        return 1;
    }
    geometryRoot.setSize(600,400);
    geometryTour.setBounds(geometryRoot.getLocalBounds());
    geometryTour.setColours({ juce::Colour(0xffd4a3f2),
                               juce::Colour(0xff201528),
                               juce::Colours::white,
                               juce::Colours::lightgrey,
                               juce::Colour(0xdd000000) });

    // Validate interaction defaults without changing a host parameter/attachment.
    juce::Slider knob;
    knob.setRange(0.0, 2.0, 0.001);
    skellcore::ui::configureStudioKnob(knob, 1.0);
    if (knob.getSliderStyle() != juce::Slider::RotaryHorizontalVerticalDrag) {
        std::fprintf(stderr, "Studio knob did not install rotary drag style\n");
        return 1;
    }

    // Actual template editor, actual component pointers and actual tour overlay.
    // The quickstart plugin itself stays tour-free; this is a renderer-only
    // demonstration to validate the shared UI before complex plugins opt in.
    juce::Slider* realKnob = nullptr;
    juce::ComboBox* realSkin = nullptr;
    for (int i = 0; i < editor.getNumChildComponents(); ++i) {
        auto* child = editor.getChildComponent(i);
        if (realKnob == nullptr) realKnob = dynamic_cast<juce::Slider*>(child);
        if (realSkin == nullptr) realSkin = dynamic_cast<juce::ComboBox*>(child);
    }
    if (realKnob == nullptr || realSkin == nullptr) {
        std::fprintf(stderr, "Tour targets not found in real JUCE editor\n");
        return 1;
    }
    skellcore::ui::GuidedTourController testTour(editor, "skellcore-ci-render-only", 1);
    std::vector<skellcore::ui::GuidedTourStep> editorSteps;
    editorSteps.emplace_back(realKnob, "YOUR GAIN", "Drag the metal knob to adjust the audio gain.");
    editorSteps.emplace_back(realSkin, "CHOOSE A SKIN", "Switch your plugin appearance independently of the sound.");
    testTour.setSteps(std::move(editorSteps));

    skellcore::ui::headless::Options options;
    options.project = "SkellCore template Studio UI";
    options.scenes = {
        {"ivory_small",     390, 250, 0},
        {"obsidian_small",  390, 250, 1},
        {"amethyst_small",  390, 250, 2},
        {"crimson_small",   390, 250, 3},
        {"ivory_default",   480, 330, 0},
        {"obsidian_default",480, 330, 1},
        {"amethyst_default",480, 330, 2},
        {"crimson_default", 480, 330, 3},
        {"ivory_large",     840, 560, 0},
        {"obsidian_large",  840, 560, 1},
        {"tour_ivory_default", 480, 330, 0, 1},
        {"tour_obsidian_small", 390, 250, 1, 1},
        {"tour_amethyst_large", 840, 560, 2, 1},
        {"unicode_ivory_large", 840, 560, 0, 2}
    };
    options.configure = [&editor, &testTour, &unicodeProbe](const skellcore::ui::headless::Scene& scene) {
        unicodeProbe.setVisible(scene.mode == 2);
        unicodeProbe.setBounds((scene.width - 520) / 2, 315, 520, 38);
        testTour.dismiss();
        const auto selection = skellcore::ui::studioSkinFromIndex(scene.theme);
        editor.setStudioSkin(selection, false);
        const auto p = skellcore::ui::studioPalette(selection);
        unicodeProbe.setColour(juce::Label::textColourId, p.foreground);
        testTour.setPalette({p.accent, p.surface, p.foreground, p.secondary,
                             juce::Colour(0xd9000000)});
        if (scene.mode == 1 && !testTour.replay())
            std::fprintf(stderr, "Tour did not show in renderer scene\n");
    };
    options.inspect = [&editor, &testTour, realKnob] {
        juce::StringArray problems;
        bool hasVersionBadge = false;
        bool hasInitialWorldStatus = false;
        for (int i = 0; i < editor.getNumChildComponents(); ++i) {
            const auto* child = editor.getChildComponent(i);
            if (child->getComponentID() == "skellcore.version-check")
                hasVersionBadge = true;
            if (child->getComponentID() == "skellcore.world-status")
                if (auto* label = dynamic_cast<const juce::Label*>(child))
                    hasInitialWorldStatus = label->getText().isNotEmpty();
            if (child->isVisible() && !editor.getLocalBounds().contains(child->getBounds()))
                problems.add("Visible control extends outside window: " + child->getName());
        }
        if (!hasVersionBadge) problems.add("Visible clickable version badge is missing");
        // Static test for a previously observed white-on-ivory value field.
        if (realKnob->findColour(juce::Slider::textBoxTextColourId) !=
            skellcore::ui::studioPalette(editor.studioSkin()).foreground)
            problems.add("Gain text box colour does not match active skin");
        if (!hasInitialWorldStatus) problems.add("World status is blank on initial editor paint");
        if (const auto* overlay = testTour.overlay()) {
            if (overlay->informationBounds().intersects(overlay->spotlightBounds()))
                problems.add("GuidedTour info panel overlaps spotlight target");
            if (!editor.getLocalBounds().contains(overlay->informationBounds()))
                problems.add("GuidedTour info panel clipped outside editor");
        }
        return problems;
    };
    options.mustDiffer = {
        {"ivory_default", "obsidian_default"},
        {"ivory_default", "amethyst_default"},
        {"ivory_default", "crimson_default"},
        {"obsidian_default", "amethyst_default"},
        {"amethyst_default", "crimson_default"},
        {"ivory_default", "tour_ivory_default"},
        {"obsidian_small", "tour_obsidian_small"},
        {"tour_ivory_default", "tour_obsidian_small"},
        {"ivory_large", "unicode_ivory_large"}
    };

    const auto directory = argc > 1
        ? juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[1]))
        : juce::File::getCurrentWorkingDirectory().getChildFile("ui-screenshots");
    const auto report = skellcore::ui::headless::render(editor, directory, options);
    for (const auto& error : report.errors)
        std::fprintf(stderr, "%s\n", error.toRawUTF8());
    std::printf("Captured %d / %zu Studio UI and GuidedTour scenes\n",
                report.captured, options.scenes.size());
    processor.releaseResources();
    return report.passed() && report.captured == static_cast<int>(options.scenes.size()) ? 0 : 1;
}
