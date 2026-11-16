#include "PluginEditor.h"
#include <SkellCore/SkullTheme.h>

SkellAudioProcessorEditor::SkellAudioProcessorEditor(SkellAudioProcessor& p)
    : AudioProcessorEditor(&p),
      skin_(skellcore::ui::StudioSkinStore::load()),
      world_(p.worldLink),
      versionBadge_("Skell Quickstart", SKELL_PRODUCT_VERSION,
                    "skellyruno", "skellcore-vst-template"),
      gainAttachment(p.parameters, "gain", gain) {
  setLookAndFeel(&skin_);
  setResizable(true, true);
  setResizeLimits(390, 250, 840, 560);

  title.setText("Skell Quickstart", juce::dontSendNotification);
  title.setFont(skellcore::ui::FontSystem::make(22.0f, skellcore::ui::FontRole::Heading, {}, true));
  title.setJustificationType(juce::Justification::centredLeft);

  // Does not change the gain APVTS range, parameter ID, automation or DSP.
  skellcore::ui::configureStudioKnob(gain, 1.0);
  addAndMakeVisible(title);
  addAndMakeVisible(gain);

  skellcore::ui::populateStudioSkinSelector(skinSelector_);
  skinSelector_.setSelectedId(static_cast<int>(skin_.skin()) + 1, juce::dontSendNotification);
  skinSelector_.setTooltip("Appearance (shared across SKELL plugins that use StudioUI)");
  skinSelector_.onChange = [this] {
    if (skinSelector_.getSelectedId() > 0)
      setStudioSkin(skellcore::ui::studioSkinFromIndex(
          skinSelector_.getSelectedId() - 1), true);
  };
  addAndMakeVisible(skinSelector_);

  // Reuse SkellKey's simple skull-click interaction, but cycle the *current*
  // StudioUI skins. The existing dropdown, templates and saved appearance
  // remain intact; only the message thread touches the LookAndFeel.
  skullMark_.onClick = [this] {
    const auto next = (static_cast<int>(skin_.skin()) + 1) % 4;
    setStudioSkin(skellcore::ui::studioSkinFromIndex(next), true);
  };
  addAndMakeVisible(skullMark_);

  worldButton_.onClick = [this] {
    if (world_.requested()) world_.leave();
    else world_.join(); // explicit only: never join at plugin load
    timerCallback();
  };
  worldStatus_.setComponentID("skellcore.world-status");
  worldStatus_.setJustificationType(juce::Justification::centredLeft);
  worldStatus_.setFont(skellcore::ui::FontSystem::make(11.0f));
  addAndMakeVisible(worldButton_);
  addAndMakeVisible(worldStatus_);
  versionBadge_.setColour(juce::TextButton::textColourOffId, skin_.palette().secondary);
  addAndMakeVisible(versionBadge_);
  timerCallback(); // Populate OFFLINE/ONLINE on the very first paint.
  startTimerHz(4); // UI/status only, never processBlock
  setSize(480, 330);
}

SkellAudioProcessorEditor::~SkellAudioProcessorEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}

void SkellAudioProcessorEditor::setStudioSkin(skellcore::ui::StudioSkin selection,
                                              bool persist) {
  if (selection != skin_.skin()) {
    // Reattach to propagate updated LAF colours to each child control.
    setLookAndFeel(nullptr);
    skin_.setSkin(selection);
    setLookAndFeel(&skin_);
  }
  skinSelector_.setSelectedId(static_cast<int>(selection) + 1,
                              juce::dontSendNotification);
  // JUCE's Slider owns a nested editable TextEditor that can retain the
  // previous skin's text colour across the first immediate snapshot.
  // Pin all three colours at the Slider level on every skin change.
  const auto p = skin_.palette();
  gain.setColour(juce::Slider::textBoxTextColourId, p.foreground);
  gain.setColour(juce::Slider::textBoxBackgroundColourId, p.surface);
  gain.setColour(juce::Slider::textBoxOutlineColourId, p.border);
  versionBadge_.setColour(juce::TextButton::textColourOffId, p.secondary);
  worldStatus_.setColour(juce::Label::textColourId, p.secondary);
  if (persist)
    (void) skellcore::ui::StudioSkinStore::save(selection);
  repaint();
}

void SkellAudioProcessorEditor::paint(juce::Graphics& g) {
  const auto theme = skin_.palette();
  const auto bounds = getLocalBounds().toFloat();
  g.setGradientFill(juce::ColourGradient(
      theme.surface, bounds.getCentreX(), bounds.getY(),
      theme.background, bounds.getCentreX(), bounds.getBottom(), false));
  g.fillAll();
  const auto panel = bounds.reduced(12.f);
  g.setColour(theme.surface.withAlpha(.8f));
  g.fillRoundedRectangle(panel, 12.f);
  g.setColour(theme.border);
  g.drawRoundedRectangle(panel, 12.f, 1.25f);

  // Shared skull-key geometry, same branding across all four skins.
  skellcore::ui::drawSkullKeyMark(g, 25.f, 15.f, theme.accent, theme.surface);
  g.setFont(skellcore::ui::FontSystem::make(11.0f, skellcore::ui::FontRole::Heading, {}, true));
  g.setColour(theme.secondary);
  g.drawFittedText("SKELLCORE  /  VST TEMPLATE",
      105, 25, getWidth()-207, 18,
      juce::Justification::centredRight, 1);
  g.setColour(theme.accent);
  g.setFont(skellcore::ui::FontSystem::make(12.5f, skellcore::ui::FontRole::Heading, {}, true));
  g.drawFittedText("GAIN", getWidth()/2 - 58, 80, 116, 18,
                   juce::Justification::centred, 1);
  g.setColour(theme.secondary);
  g.setFont(skellcore::ui::FontSystem::make(10.0f, skellcore::ui::FontRole::Symbol));
  g.drawFittedText("SKELLWORLD  |  OPTIONAL LOCAL CONNECTION",
      28, getHeight() - 31, getWidth()-56, 16, juce::Justification::centred, 1);
}

void SkellAudioProcessorEditor::resized() {
  const int w = getWidth(), h = getHeight();
  title.setBounds(30, 48, w-198, 30);
  skinSelector_.setBounds(w-158, 49, 126, 27);
  skullMark_.setBounds(24, 14, 76, 34);
  versionBadge_.setBounds(w-92, 23, 64, 22);
  const int knobSize = juce::jmin(156, juce::jmax(70, h-195));
  gain.setBounds((w-knobSize)/2, 103, knobSize, knobSize);
  worldButton_.setBounds(w/2 - 151, h-77, 122, 29);
  worldStatus_.setBounds(w/2 - 22, h-77, 170, 29);
}

void SkellAudioProcessorEditor::timerCallback() {
  const bool joined = world_.requested();
  worldButton_.setButtonText(joined ? "LEAVE WORLD" : "JOIN WORLD");
  // Presence needs the actual Hub. The separate hub-free tuning mapping does
  // not imply any SkellWorld peers or a running presence broker.
  if (world_.hubOnline())
    worldStatus_.setText("WORLD ONLINE  |  " + juce::String(world_.peerCount()) + " peers",
                         juce::dontSendNotification);
  else
    worldStatus_.setText(joined ? "HUB NOT RUNNING" : "OFFLINE",
                         juce::dontSendNotification);
}
