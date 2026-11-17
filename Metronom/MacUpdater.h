#pragma once

namespace MacUpdater
{
// Starts Sparkle (feed and key from Info.plist) and adds "Check for Updates…" to the app menu.
// Does nothing in builds without a feed URL or public key (Debug).
void start();
} // namespace MacUpdater
