#pragma once
// User-initiated, nonblocking GitHub release check for JUCE plugin editors.
// Does NOT install files or change VST3 binaries/presets/automation.
#include <SkellCore/Version.h>
#include <SkellCore/FontSystem.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include <utility>

namespace skellcore::ui {
class UpdateBadge final : public juce::TextButton {
public:
  UpdateBadge(juce::String productName, juce::String installedVersion,
              juce::String githubOwner, juce::String githubRepo)
      : juce::TextButton("v" + installedVersion.trimCharactersAtStart("vV")),
        product_(std::move(productName)), version_(std::move(installedVersion)),
        owner_(std::move(githubOwner)), repo_(std::move(githubRepo)) {
    setComponentID("skellcore.version-check");
    setTooltip("Check for " + product_ + " updates (only when clicked)");
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    onClick = [this] { checkForUpdates(); };
  }

  void checkForUpdates() {
    if (checking_) return;
    const auto validRepository = [](const juce::String& s) {
      return s.isNotEmpty() && s.length() <= 100
          && s != "." && s != ".."
          && s.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._");
    };
    if (!validRepository(owner_) || !validRepository(repo_) ||
        !version::parse(version_.toStdString()).valid) {
      showInfo("Update check unavailable", "The plugin version or release repository is not configured.");
      return;
    }
    checking_ = true;
    const auto savedText = getButtonText();
    setButtonText("CHECKING");
    const auto weak = juce::Component::SafePointer<UpdateBadge>(this);
    const auto api = juce::String("https://api.github.com/repos/") + owner_ + "/" + repo_ + "/releases/latest";
    const bool launched = juce::Thread::launch([weak, api, savedText] {
      int code = 0;
      juce::String remoteTag;
      auto stream = juce::URL(api).createInputStream(
          juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
              .withConnectionTimeoutMs(4000)
              .withNumRedirectsToFollow(2)
              .withStatusCode(&code)
              .withExtraHeaders("User-Agent: SkellCore-UpdateBadge\r\nAccept: application/vnd.github+json\r\n"));
      if (stream != nullptr && code == 200) {
        juce::MemoryBlock data;
        stream->readIntoMemoryBlock(data, 256 * 1024 + 1);
        if (data.getSize() <= 256 * 1024) {
          const auto response = juce::JSON::parse(
              juce::String::fromUTF8(static_cast<const char*>(data.getData()),
                                     static_cast<int>(data.getSize())));
          if (auto* object = response.getDynamicObject()) {
            if (!static_cast<bool>(object->getProperty("draft")) &&
                !static_cast<bool>(object->getProperty("prerelease")))
              remoteTag = object->getProperty("tag_name").toString();
          }
        }
      }
      juce::MessageManager::callAsync([weak, savedText, remoteTag, code] {
        if (weak == nullptr) return;
        weak->checking_ = false;
        weak->setButtonText(savedText);
        weak->handleResult(code, remoteTag);
      });
    });
    if (!launched) {
      checking_ = false;
      setButtonText(savedText);
      showInfo("Update check unavailable", "Could not start the update check.");
    }
  }

private:
  void showInfo(const juce::String& title, const juce::String& body) {
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                            title, body, "OK", this);
  }

  void handleResult(int statusCode, const juce::String& remoteTag) {
    const auto safeTag = remoteTag.isNotEmpty() && remoteTag.length() < 80 &&
        remoteTag.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._+");
    if (statusCode != 200 || !safeTag || !version::parse(remoteTag.toStdString()).valid) {
      showInfo("Update check unavailable",
               "Could not read a published stable release (network, API limit, or no release). Your plugin was not changed.");
      return;
    }
    if (!version::newer(version_.toStdString(), remoteTag.toStdString())) {
      showInfo("Up to date", product_ + " v" + version_.trimCharactersAtStart("vV") +
                                   " is already current.");
      return;
    }
    // Construct an official, HTTPS GitHub release URL from validated components;
    // never launch an untrusted URL supplied by the response body.
    const auto releaseUrl = "https://github.com/" + owner_ + "/" + repo_ + "/releases/tag/" + remoteTag;
    const auto message = product_ + " " + remoteTag + " is available (installed: " +
                         version_ + "). Open the official release page to review and install it?";
    auto options = juce::MessageBoxOptions()
        .withIconType(juce::AlertWindow::QuestionIcon)
        .withTitle("Update available")
        .withMessage(message)
        .withButton("Get update")
        .withButton("Later")
        .withAssociatedComponent(this);
    juce::AlertWindow::showAsync(options, [releaseUrl](int choice) {
      if (choice == 1) juce::URL(releaseUrl).launchInDefaultBrowser();
    });
  }

  juce::String product_, version_, owner_, repo_;
  bool checking_ = false;
};
} // namespace skellcore::ui
