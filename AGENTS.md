# AGENTS.md

Guidance for AI agents working in this repository.

## Project

- Metronom — a metronome app in **Qt 6 / QML** (Qt Quick + Material style).
- Toolchain: C++23, Qt ≥ 6.10 (developed on 6.11/6.12), CMake ≥ 3.16 + Ninja.
- Target platforms: **Android, macOS and Linux** (X11 and Wayland). Every change must keep all three building.

## Structure

- `CMakeLists.txt` — app target `metronom`, QML module `App` (`Main.qml`, `Splash.qml`, `Controls/`), translations,
  install/deploy and Linux DEB packaging (CPack).
- `Metronom/` — static QML module `metronom_qml` (URI `Metronom`): the C++ types and their QML controls.
  - `Metronome` — audio engine (`QAudioSink`), beat scheduling, auto-increment.
  - `ScreenLockInhibitor` — keeps the screen awake while playing (see *Platform code*).
  - `PlaybackService` — Android foreground service (`android/java/.../MetronomeService.java`) that keeps the
    metronome playing in the background: a notification with Stop/Resume whose icon is the current note value
    (`android/res/drawable/ic_note_*.xml`); no-op elsewhere.
  - `StatusBarController` — Android status bar appearance.
  - `LanguageManager` — UI language selection (QSettings `language`).
  - `NativeWindow` — translucent, blurred window with the content under the title bar (macOS; no-op elsewhere).
  - `AppInfo` — QML singleton: Qt build/runtime versions for the About dialog, `nativeMenuBar` (macOS).
  - `Appearance` — QML singleton: light/dark choice (QSettings `colorScheme`, a `Qt::ColorScheme`), applied with
    `QStyleHints::setColorScheme`; `dark` is the effective result.
  - `Theme` — QML singleton (`Theme.qml`): the app colours (orange accent, backgrounds, borders), independent of the
    Controls style.
  - `MacUpdater` — Sparkle updates and "Check for Updates…" (macOS only, started from `main.cpp`).
- `MacStyle/` — Qt Quick Controls style used on macOS (static QML module, `APPLE` only); see *Controls style*.
- `main.cpp` — `QGuiApplication` + `QQmlApplicationEngine`, selects the Controls style, loads the translator and
  `App.Splash`.
- `android/` — Android package sources (manifest, Gradle, `SplashActivity`, icons).
- `cmake/` — macOS `Info.plist.in`, `PruneBundle.cmake` (install step of `package-macos`) and `Sparkle.cmake`.
- `docs/releases/<tag>.md` — release notes (GitHub Release and the Sparkle update dialog).
- `docs/images/` — the site's screenshots and GIFs (macOS window over the desktop).
- `site/` — the project's website (Jekyll, GitHub Pages: <https://rask-project.github.io/metronom/>); see
  `site/README.md`. Published by `.github/workflows/pages.yml`. In the app's five languages: its text is in
  `site/_data/i18n/<lang>.yml`, and a new string goes into every one of them.
- `resources/` — icons theme, logo, macOS `metronom.icns`, `qtquickcontrols2.conf`, Linux `.desktop` template.
- `scripts/` — `format-code.sh` (clang-format), `make-macos-icon.sh` (regenerates the `.icns`), `update-appcast.py`
  (adds a release to the Sparkle appcast; run by `release.yml`), `site-assets.sh` / `site-serve.sh` (the website).
- `translations/` — `.ts` files (pt_BR, es_ES, fr_FR, it_IT).
- `build/` — out-of-source build directories (gitignored), one per Qt Creator kit.

## Build and run

Build to validate changes (the macOS Debug build below); also run it when the user asks.

```sh
# macOS (kit: Qt 6.12.0 for macOS)
cmake -B build/Qt_6_12_0_for_macOS_Debug -G Ninja \
  -DCMAKE_PREFIX_PATH=/opt/Qt/6.12.0/macos -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/Qt_6_12_0_for_macOS_Debug
open build/Qt_6_12_0_for_macOS_Debug/metronom.app

# Linux (kit: Qt 6.11.2 gcc_64)
cmake -B build/Desktop_Qt_6_11_2_Debug -G Ninja \
  -DCMAKE_PREFIX_PATH=/opt/Qt/6.11.2/gcc_64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Desktop_Qt_6_11_2_Debug
./build/Desktop_Qt_6_11_2_Debug/metronom
# DEB package (installs under /opt/metronom)
cmake -B build/release -G Ninja -DCMAKE_PREFIX_PATH=/opt/Qt/6.11.2/gcc_64 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/metronom && cmake --build build/release && cpack --config build/release/CPackConfig.cmake

# Android (needs ANDROID_SDK_ROOT and ANDROID_NDK_ROOT)
/opt/Qt/6.12.0/android_arm64_v8a/bin/qt-cmake -B build/android -G Ninja \
  -DQT_HOST_PATH=/opt/Qt/6.12.0/macos -DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT -DANDROID_NDK_ROOT=$ANDROID_NDK_ROOT
cmake --build build/android --target apk
```

Qt Creator opens the project directly with the matching kit.

## Conventions

- **Language:** respond in Portuguese. Code, comments and commit messages in English.
- **Comments:** minimal — names speak for themselves; at most one short line when the "why" is not obvious.
- **Work order:** get the code compiling/working first; review tests later.
- **Intentionally commented-out code** for testing parts in isolation is not a bug — confirm it's intentional before
  "fixing" it.
- **Hot paths (audio callback, sample generation):** zero allocation, no locks, no logging; avoid needless
  abstractions.
- **Design:** minimal solutions, no speculative fields or options.
- No debug logging (`qDebug`) left in committed code.
- **Commits and PRs:** never add `Co-authored-by` (or any other AI attribution trailer, e.g. "Generated with")
  to commit messages or pull request descriptions. The author is the repository owner.

## C++ rules

- **C++23** is required (enforced in `CMakeLists.txt`).
- **Ownership:** every owned object has exactly one owner, a `std::unique_ptr` (or a plain value member). Ownership
  moves with `std::move`; code that only uses an object gets a non-owning `const T *` (or a reference) and never keeps
  it past the owner's lifetime. QObject children owned by their Qt parent are fine.
- **`std::shared_ptr` is forbidden** (and `std::make_shared`, `std::weak_ptr`, `QSharedPointer`). If the logic seems
  to need shared ownership, rethink it so one object owns the thing and the others borrow it.
- **Raw pointers are forbidden** — no `new`/`delete`, no owning raw pointers. Exceptions: the non-owning `const T *`
  above, and pointers required by C/platform APIs (Wayland, XCB, IOKit, JNI). Wrap C handles in `std::unique_ptr` with
  a custom deleter (see `ScreenLockInhibitor`).
- **Exceptions are forbidden** — no `throw`, `try` or `catch`.
- **Error handling:** fallible operations return `std::expected<T, E>` with a concrete error type (or `bool`/
  `std::optional` when there is nothing to report).
- **Types:** use Qt's typedefs, never the `<cstdint>` ones: `qint8`…`qint64` / `quint8`…`quint64` for fixed width,
  `qsizetype` for sizes and indices (not `std::size_t`), `qreal` for geometry. Integers are `qint32` instead of
  `int`, `Q_PROPERTY` and `QList<qint32>` included (QML still sees `int`); only `main(int argc, …)` keeps `int`.
  Lists are `QList`, not `std::vector`. Platform API types (`jint`, `CGFloat`, `NSInteger`, …) stay at the API
  boundary.
- **Initialization:** brace-initialize every variable — data members, constants, locals and `for` counters
  (`bool m_running{false};`, `const qint32 bpm{unpackField(...)};`, `for (qsizetype i{0}; ...)`): braces reject
  narrowing conversions. `=` only where the grammar requires it (default arguments, enumerators). Exception: a
  container constructor that takes a size or other non-element arguments keeps parentheses (`QList<float> click(n);`),
  since braces would pick the `std::initializer_list` constructor and build a one-element list.
- **Algorithms:** prefer `std::ranges` / `<algorithm>` over hand-written loops.
- **Control flow:** early return over nesting; flatten `if`/`else` chains with guard clauses.
- **Performance:** `emplace_back` over `push_back`, move over copy, no needless temporaries; mark `noexcept` where it
  holds.
- **Member ordering:** order data members from largest alignment to smallest (`std::unique_ptr`/pointers/`double`
  first, `bool` and small enums last) to minimise padding.

## Qt / QML

- Follow Qt idioms: signals/slots, `Q_PROPERTY`, `QML_ELEMENT`, and Qt's model classes for list data.
- QML stays declarative. Business logic belongs in C++ — a QML file that grows imperative logic is a bug in the
  layering, not a style preference.
- Native/platform behaviour is exposed to QML through a C++ type's properties; QML never reaches into platform code.
- New user-visible strings use `qsTr()` and must be added to every `.ts` file in `translations/`.
- Check <https://doc.qt.io> for component and API behaviour rather than guessing.
  - QML best practices: <https://doc.qt.io/qt-6/qtquick-bestpractices.html>
  - QML coding conventions: <https://doc.qt.io/qt-6/qml-codingconventions.html>

## Controls style

- Chosen at run time in `main.cpp` (`QQuickStyle::setStyle`): **Material** on Android and Linux (configured in
  `resources/qtquickcontrols2.conf`), **MacStyle** on macOS with `macOS` as the explicit fallback style (its plugin is
  what keeps the palette in sync with the system and the colour scheme override).
- Shared QML imports plain `QtQuick.Controls` and takes colours from `Theme`, never from `Material.*`. Only `Main.qml`
  and `Splash.qml` import `QtQuick.Controls.Material as M`, for `M.Material.theme` (it has no effect under MacStyle).
  Never import a style module unqualified: that is compile-time style selection.
- `MacStyle` imports `QtQuick.Controls.macOS` and adds the controls the macOS style lacks (they would fall back to
  Fusion) or draws unlike AppKit: `Button` (flat ones borderless), `RoundButton`, `ToolButton`, `Tumbler`, `ToolTip`,
  `SwitchDelegate`/`ItemDelegate` (form rows). A control the app starts using that the macOS style does not implement
  gets a file there. Tints use `palette.accent`, which `Main.qml` sets to `Theme.accent`.
- qmllint reports `controls-native-customize` for every `MacStyle` file: expected, the module is the style.

## Platform code

- Use `Q_OS_MACOS`, `Q_OS_ANDROID` and `defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)` (Android also defines
  `Q_OS_LINUX`). `Q_OS_*` macros only exist after a Qt header is included.
- Platform-specific libraries are linked per platform in `Metronom/CMakeLists.txt`: IOKit/CoreFoundation on macOS,
  `Qt6::CorePrivate` (JNI helpers) on Android, Qt DBus on Linux. Prefer Qt/D-Bus APIs over linking X11 or Wayland
  libraries directly.
- `ScreenLockInhibitor`: IOPM assertion `PreventUserIdleDisplaySleep` on macOS, `FLAG_KEEP_SCREEN_ON` on Android,
  D-Bus `org.freedesktop.ScreenSaver` + `org.freedesktop.portal.Inhibit` on Linux (works on X11 and Wayland).
- `NativeWindow` (`Main.qml`): on macOS the window uses `ExpandedClientAreaHint | NoTitleBarBackgroundHint`, a
  transparent `color` and the `Header` starts after the window buttons (`leadingInset`); elsewhere `translucent` is
  false and the window is unchanged.
- macOS app icon: `resources/metronom.icns`, generated from `resources/logo.svg` by `scripts/make-macos-icon.sh`
  (run on macOS whenever the logo changes and commit the result).

## Native macOS interop

- Native AppKit behaviour is written in **Objective-C++** (`Metronom/*.mm`, compiled with ARC: `-fobjc-arc`). The
  top-level `CMakeLists.txt` enables `OBJCXX` only on Apple, and `Metronom/CMakeLists.txt` adds the `.mm` sources and
  AppKit inside `if(APPLE)`, so Linux and Android never see them.
- The `.mm` file exposes plain C++ functions in a C++-only header (e.g. `MacWindowEffects.h`); the QML type calls them
  under `#ifdef Q_OS_MACOS`.
- On macOS `QWindow::winId()` is the window's `contentView` (`NSView *`), not the `NSWindow` — get the window via
  `[nsView window]`, and only when `QGuiApplication::platformName()` is `cocoa`.
- **Configurable native behaviour is exposed to QML as a C++ QML type** (`QML_ELEMENT`), and QML binds its properties —
  e.g. `NativeWindow` (`window`/`color`/`alpha`). Don't reach into native code from QML; add a property instead.
- The app menu is a QML `MenuBar` (`Main.qml`), loaded only when `AppInfo.nativeMenuBar`. Qt moves items into the
  macOS app menu by matching their text ("About…", "Settings…", "Quit…"), so those texts stay in English; any other
  item of that menu must be added natively (see `MacUpdater.mm`).
- AppKit colors expect **0.0–1.0** components: use `QColor::redF()`/`greenF()`/`blueF()`, not `red()`/`green()`/
  `blue()`.

## Versioning and releases

- The version comes from the **latest Git tag** (`git describe --tags --abbrev=0`, leading `v` stripped), falling
  back to `project(... VERSION ...)` in `CMakeLists.txt` when there is no tag. The app reports it as
  `QCoreApplication::applicationVersion()` = `"<version> (<short commit>)"`.
- Tags: `vMAJOR.MINOR.PATCH` for releases, `vMAJOR.MINOR.PATCH-alpha.N` / `-beta.N` / `-rc.N` for pre-releases.
- Derived numbers (all computed in `CMakeLists.txt`; never edit them by hand):
  - build number: alpha.N → N, beta.N → 100 + N, rc.N → 500 + N, release → 1000;
  - macOS `CFBundleVersion` = `MAJOR.MINOR.PATCH.<build>`, `CFBundleShortVersionString` = the tag;
  - Android `versionCode` = `((MAJOR * 100 + MINOR) * 100 + PATCH) * 10000 + <build>`, `versionName` = the tag;
  - DEB version = the tag with `-` → `~` (so `1.2.0~beta.1` sorts before `1.2.0`).
- To release: write `docs/releases/<tag>.md` (what changed, for users) and a post in `site/_posts/`, bump `project(... VERSION ...)` in the commit
  the tag points at, tag it (`git tag v1.1.0 && git push origin v1.1.0`), then reconfigure CMake so the new tag is
  picked up.

## Updates (macOS, Sparkle)

- Sparkle (version and SHA-256 pinned in `cmake/Sparkle.cmake`, downloaded at configure time) is embedded in
  `metronom.app/Contents/Frameworks`. Release builds get `SUFeedURL` (`METRONOM_UPDATE_FEED_URL`, the `appcast` branch)
  and `SUPublicEDKey` (`METRONOM_UPDATE_PUBLIC_KEY`, the same key as Konsoll) in `Info.plist`; Debug builds have no feed
  and never update.
- Checks are daily and updates install on quit (`SUEnableAutomaticChecks`, `SUAutomaticallyUpdate`). Sparkle compares
  `CFBundleVersion`, so pre-releases sort before their release; they go in the `beta` channel, which the app does not
  read.
- `release.yml` (job `appcast`) signs the macOS zip with the `SPARKLE_ED_PRIVATE_KEY` secret and adds it to
  `appcast.xml` on the `appcast` branch, with `docs/releases/<tag>.md` as the notes. Without the secret it only warns.
- The `QtActivity` declares `android.app.background_running`: without it Qt blocks its event loop in the background
  and the notification's Stop (or any queued call) waits until the app returns.
- Android JNI calls that touch the activity/window run on the Android UI thread via
  `QNativeInterface::QAndroidApplication::runOnAndroidMainThread`.

## CI and packages

- `.github/workflows/build.yml` (reusable) builds and packages every platform with Qt 6.12.0:
  - macOS: arm64 `.zip` + `.dmg` (deployment target 14.4) from the `package-macos` target, which installs to
    `build/<dir>/dist`, runs macdeployqt, prunes unused Qt parts (`cmake/PruneBundle.cmake`) and signs the bundle
    (ad hoc, or with `-DMETRONOM_CODESIGN_IDENTITY=<Developer ID>`); not notarized;
  - Linux: `.deb` built on Ubuntu 24.04 (`cpack`, Qt deployed under `/opt/metronom`);
  - Android: arm64-v8a `.apk` + `.aab` (NDK r27c, JDK 17).
- `ci.yml` runs it on pushes to `main` and on pull requests (packages as workflow artifacts); `release.yml` runs it
  on `v*` tags and publishes a GitHub Release (pre-release for `-alpha/-beta/-rc` tags).
- Android signing uses the repository secrets `ANDROID_KEYSTORE_BASE64` (`base64 -i release.keystore`),
  `ANDROID_KEYSTORE_ALIAS`, `ANDROID_KEYSTORE_PASSWORD` and `ANDROID_KEY_PASSWORD`. Without them the job builds a
  debug-signed APK.
- `scripts/ci-version.sh` names the package files (tag on tag builds, `<last tag>-<commit>` otherwise).
- Keep `scripts/format-code.sh -n` passing: CI fails on unformatted C++ (clang-format 19.1.6).

## Formatting

- **Run `scripts/format-code.sh` after every change**, before building or committing. It formats C++ and
  Objective-C++ (`.cpp`, `.h`, `.mm`) with clang-format (`.clang-format`: LLVM-based, 120 columns, 4 spaces, never tabs).
- `scripts/format-code.sh -n` changes nothing and exits 1 listing the files that need formatting.
- QML follows the Qt QML coding conventions (4-space indent, attribute order: id, properties, signals, functions,
  children).

## clang-tidy

- **Follow the clang-tidy rules proactively.** Read `.clang-tidy` before writing C++ and write code that already
  conforms. `WarningsAsErrors: "*"` makes every enabled check an error.
- Key rules: `readability-function-cognitive-complexity`, `misc-no-recursion`, `cppcoreguidelines-avoid-do-while`,
  `readability-implicit-bool-conversion`, `misc-const-correctness`, `cppcoreguidelines-init-variables`.
- No `NOLINT` to suppress findings — refactor the code instead.
- Verify per file (needs `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`; with Homebrew's clang-tidy on macOS pass the SDK):
  `clang-tidy -p build/Qt_6_12_0_for_macOS_Debug --config-file=.clang-tidy --extra-arg=-isysroot$(xcrun --show-sdk-path) <file>`.
  On macOS only the macOS branches of platform code are analysed — check Linux code on a Linux build.
- **`.mm` caveat:** clang-tidy 19 segfaults on `.mm` files while running
  `cppcoreguidelines-avoid-const-or-ref-data-members` (AST-matcher bug, not a code issue). Run `.mm` files with that
  single check disabled (`--checks=-cppcoreguidelines-avoid-const-or-ref-data-members`); all other checks still apply.
