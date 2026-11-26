# Programmatic JUCE editor capture

The existing renderer has an opt-in --demo NEW_OUTPUT_DIR EXACT_SOURCE_COMMIT mode. It constructs a fresh Skell Quickstart processor/editor, operates its actual attached gain slider on the message thread, verifies raw and normalized readback, processes a synthetic two-tone fixture in 500-sample blocks, and captures 144 editor PNGs at 24 fps. The sample clock is 48 kHz. The sequence never loads production session state or joins SkellWorld. No changes to plugin IDs, parameters, realtime callback or normal renderer mode.

This is offline genuine editor operation, not physical mouse input, live DAW capture or evidence of the latest SkellyTune editor. The first implementation is a template gain adapter, not universal parameter automation. CI must compile and capture the actual JUCE editor; local graphics/audio development dependencies remain unavailable.

LyricForge CLI: python studio_cli.py editor-demo seal CAPTURE; then editor-demo verify CAPTURE; then editor-demo render CAPTURE --out NEW_DIRECTORY. The importer checks frame count, actual readback, timestamps and measured levels against PCM, and integrity hashes. It renders the original editor snapshots and processed audio. The MP4 may be supplied as a recorded demonstration asset to Creator. The seal does not independently attest producer identity.
