#pragma once
// Click-only stable-release check for JUCE plug-in editors, never processBlock.
// Worker is owned by this component and joined before plug-in code can unload.
#include <SkellCore/ReleasePolicy.h>
#include <SkellCore/FontSystem.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_events/juce_events.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <utility>

namespace skellcore::ui {
class UpdateBadge final : public juce::TextButton, private juce::AsyncUpdater {
  struct Response {
    int httpStatus = 0;
    juce::String releaseTag;
    bool draft = false;
    bool prerelease = false;
  };

  class CheckWorker final : public juce::Thread {
  public:
    CheckWorker(UpdateBadge& owner, juce::String api)
      : juce::Thread("SkellCore release check"),
        owner_(owner), api_(std::move(api)) {}

    void run() override {
      Response result;
      auto stream = juce::URL(api_).createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
          .withConnectionTimeoutMs(2500)
          .withNumRedirectsToFollow(2)
          .withStatusCode(&result.httpStatus)
          .withExtraHeaders("User-Agent: SkellCore-UpdateBadge\r\nAccept: application/vnd.github+json\r\n"));
      if (threadShouldExit()) return;
      if (stream == nullptr) {
        result.httpStatus = 0;
      } else if (result.httpStatus == 200) {
        // Keep memory bounded even if the server returns unusually large JSON.
        juce::MemoryBlock data;
        stream->readIntoMemoryBlock(data, 128 * 1024 + 1);
        if (threadShouldExit()) return;
        if (data.getSize() > 0 && data.getSize() <= 128 * 1024) {
          const auto parsed = juce::JSON::parse(
            juce::String::fromUTF8(static_cast<const char*>(data.getData()),
                                   static_cast<int>(data.getSize())));
          if (auto* object = parsed.getDynamicObject()) {
            result.draft = static_cast<bool>(object->getProperty("draft"));
            result.prerelease = static_cast<bool>(object->getProperty("prerelease"));
            result.releaseTag = object->getProperty("tag_name").toString();
          }
        }
      }
      if (!threadShouldExit())
        owner_.publishResponse(std::move(result));
    }

  private:
    UpdateBadge& owner_;
    juce::String api_;
  };

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

  ~UpdateBadge() override {
    // Detached lambdas risk executing VST3 code after DAW module unload.
    // Intentionally join this explicit user-triggered, network-only worker.
    // The UI may wait for the configured network timeout when it closes.
    if (worker_ != nullptr) {
      worker_->signalThreadShouldExit();
      worker_->stopThread(-1); // never kill a thread holding OS resources
      worker_.reset();
    }
    cancelPendingUpdate(); // no queued AsyncUpdater callback after destruction
  }

  void checkForUpdates() {
    if (checking_) return; // avoid parallel requests / repeated clicks
    const auto configured = updates::classify(
      version_.toStdString(), owner_.toStdString(), repo_.toStdString(), 0);
    if (configured == updates::Result::InvalidConfiguration) {
      showInfo("Update check unavailable",
               "This product has no valid release repository or version configured.");
      return;
    }

    if (worker_ != nullptr) {
      worker_->stopThread(-1); // previous check finished before UI callback
      worker_.reset();
    }

    const auto api = "https://api.github.com/repos/" + owner_ + "/" + repo_ + "/releases/latest";
    worker_ = std::make_unique<CheckWorker>(*this, api);
    checking_ = true;
    setButtonText("...");
    if (!worker_->startThread()) {
      worker_.reset();
      checking_ = false;
      setButtonText("v" + version_.trimCharactersAtStart("vV"));
      showInfo("Update check unavailable", "Could not start a release check.");
    }
  }

private:
  void publishResponse(Response response) {
    // Only the background worker calls this. Message thread never locks DSP.
    {
      const juce::ScopedLock guard(responseLock_);
      pendingResponse_ = std::move(response);
    }
    triggerAsyncUpdate(); // safe for background/UI threads, never audio thread
  }

  void handleAsyncUpdate() override {
    Response result;
    {
      const juce::ScopedLock guard(responseLock_);
      result = std::move(pendingResponse_);
    }
    checking_ = false;
    setButtonText("v" + version_.trimCharactersAtStart("vV"));
    handleResult(result);
  }

  void showInfo(const juce::String& title, const juce::String& body) {
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                           title, body, "OK", this);
  }

  void handleResult(const Response& response) {
    const auto remote = response.releaseTag.toStdString();
    const auto status = updates::classify(
      version_.toStdString(), owner_.toStdString(), repo_.toStdString(),
      response.httpStatus, remote, response.draft, response.prerelease);
    using Result = updates::Result;
    switch (status) {
      case Result::InvalidConfiguration:
        showInfo("Update check unavailable", "The release repository or local version is invalid.");
        return;
      case Result::Offline:
        showInfo("Update check unavailable", "Cannot reach the release service. Check your connection and try again.");
        return;
      case Result::NoPublishedRelease:
        showInfo("No published release", "This product has no public stable release yet. Your plug-in is unchanged.");
        return;
      case Result::AuthenticationRequired:
        showInfo("Update check unavailable", "The release service requires access. No credentials are stored in this plug-in.");
        return;
      case Result::RateLimited:
        showInfo("Update check unavailable", "The release service rejected or limited the request. Try later.");
        return;
      case Result::ServiceUnavailable:
        showInfo("Update check unavailable", "The release service returned an error. Your plug-in is unchanged.");
        return;
      case Result::UnsupportedRelease:
        showInfo("Update check unavailable", "The latest release metadata was missing, invalid, or not stable.");
        return;
      case Result::Current:
        showInfo("Up to date", product_ + " v" + version_.trimCharactersAtStart("vV") +
                              " is already current.");
        return;
      case Result::UpdateAvailable:
        break;
    }

    // This address is built from validated owner/repo/tag, not from JSON URLs.
    const auto releaseUrl = "https://github.com/" + owner_ + "/" + repo_ + "/releases/tag/" +
                            response.releaseTag;
    const auto message = product_ + " " + response.releaseTag +
      " is available (installed: " + version_ +
      "). Open the official release page to review and download?";
    const auto options = juce::MessageBoxOptions()
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
  juce::CriticalSection responseLock_;
  Response pendingResponse_;
  std::unique_ptr<CheckWorker> worker_;
  bool checking_ = false; // UI thread only
};
} // namespace skellcore::ui
