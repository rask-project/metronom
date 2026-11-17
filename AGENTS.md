# AGENTS.md

Guidance for AI agents working in this repository.

## Project

- Rask Metronom — a metronome app in **Qt 6 / QML** (Qt Quick + Material style).
- Toolchain: C++23, Qt ≥ 6.10 (developed on 6.11/6.12), CMake ≥ 3.16 + Ninja.
- Target platforms: **Android, macOS and Linux** (X11 and Wayland). Every change must keep all three building.

## Structure

- `CMakeLists.txt` — app target `metronom`, QML module `App` (`Main.qml`, `Splash.qml`, `Controls/`), translations,
  install/deploy and Linux DEB packaging (CPack).
- `Metronom/` — static QML module `metronom_qml` (URI `Metronom`): the C++ types and their QML controls.
  - `Metronome` — audio engine (`QAudioSink`), beat scheduling, auto-increment.
  - `ScreenLockInhibitor` — keeps the screen awake while playing (see *Platform code*).
  - `StatusBarController` — Android status bar appearance.
  - `LanguageManager` — UI language selection (QSettings `language`).
- `main.cpp` — `QGuiApplication` + `QQmlApplicationEngine`, loads the translator and `App.Splash`.
- `android/` — Android package sources (manifest, Gradle, `SplashActivity`, icons).
- `cmake/` — macOS `Info.plist.in` and `PruneBundle.cmake` (install step of `package-macos`).
- `resources/` — icons theme, logo, macOS `metronom.icns`, `qtquickcontrols2.conf`, Linux `.desktop` template.
- `scripts/` — `format-code.sh` (clang-format), `make-macos-icon.sh` (regenerates the `.icns`).
- `translations/` — `.ts` files (pt_BR, es_ES, fr_FR, it_IT).
- `build/` — out-of-source build directories (gitignored), one per Qt Creator kit.

## Build and run

Only build when the user explicitly asks; don't run a build on your own to "validate" changes.

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

## Platform code

- Use `Q_OS_MACOS`, `Q_OS_ANDROID` and `defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)` (Android also defines
  `Q_OS_LINUX`). `Q_OS_*` macros only exist after a Qt header is included.
- Platform-specific libraries are linked per platform in `Metronom/CMakeLists.txt`: IOKit/CoreFoundation on macOS,
  `Qt6::CorePrivate` (JNI helpers) on Android, Qt DBus on Linux. Prefer Qt/D-Bus APIs over linking X11 or Wayland
  libraries directly.
- `ScreenLockInhibitor`: IOPM assertion `PreventUserIdleDisplaySleep` on macOS, `FLAG_KEEP_SCREEN_ON` on Android,
  D-Bus `org.freedesktop.ScreenSaver` + `org.freedesktop.portal.Inhibit` on Linux (works on X11 and Wayland).
- macOS app icon: `resources/metronom.icns`, generated from `resources/logo.svg` by `scripts/make-macos-icon.sh`
  (run on macOS whenever the logo changes and commit the result).

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
- To release: bump `project(... VERSION ...)` in the commit the tag points at, tag it
  (`git tag v1.1.0 && git push origin v1.1.0`), then reconfigure CMake so the new tag is picked up.
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

- **Run `scripts/format-code.sh` after every change**, before building or committing. It formats C/C++ (`.cpp`, `.h`)
  with clang-format (`.clang-format`: LLVM-based, 120 columns, 4 spaces, never tabs).
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
