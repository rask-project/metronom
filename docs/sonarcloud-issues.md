# SonarCloud — issues to fix

> Snapshot from 2026-11-19 · project `rask-project_metronom` (org `rask-project`)
>
> **26 open issues** — 17 vulnerabilities · 7 code smells · 2 bugs
> Severity: **1 BLOCKER** · 24 MAJOR · 1 MINOR
>
> Dashboard: <https://sonarcloud.io/project/overview?id=rask-project_metronom>

## ⚠️ Status of the CI scan

The `sonar` job in `ci.yml` **fails** because the project has SonarCloud's
*Automatic Analysis* enabled — the two cannot run at the same time:

```
ERROR You are running CI analysis while Automatic Analysis is enabled.
```

**To make the CI scan work:** disable *Automatic Analysis* in the dashboard
(project → Administration → Automatic Analysis). These 26 issues come from that
automatic analysis (default config, no exclusions); the CI scan, with the
build's `compile_commands.json`, is what gives the precise C++ analysis.

> As the scope now includes scripts, workflows, site and android (see
> `sonar-project.properties`), the next CI scan will re-analyze everything and
> keep these issues visible.

## How to use

- Each item: severity, type, rule, message and **how to fix**.
- When fixed in the code, the next scan marks the issue as resolved on its own.
- Items marked as *false positive* should be accepted in the dashboard
  (False positive / Won't fix), not changed in the code.

---

## 1. `scripts/update-appcast.py` — 5 issues (includes the only BLOCKER)

### 🔴 BLOCKER · `pythonsecurity:S2083` · line 94
**Path Traversal via file content**
`args.appcast.write_text(...)` writes to an unvalidated path; a path with `..`
could write outside the expected directory.
**Fix:** validate the path before using it — `resolve()` to an absolute path and
check it is inside the repo, or reject `..`.

### 🟠 MAJOR · `pythonsecurity:S8707` · lines 58, 83, 94
**Path Traversal via faulty LLM-supplied CLI arguments**
The CLI paths `--notes` (l. 58) and `appcast` (l. 83, 94) are not validated.
The script is called by `release.yml` and may receive LLM-generated arguments.
**Fix:** a `safe_path(p)` function (resolve + check the directory + reject `..`)
used on the 3 lines resolves all 3 issues at once.

### 🟠 MAJOR · `python:S8786` · line 37
**Regex with super-linear backtracking**
`ITEM_RE = re.compile(r"[ \t]*<item>.*?</item>\n?", re.DOTALL)` — the `[ \t]*`
+ `.*?` can degrade on input without `</item>`.
**Fix:** simplify the regex (e.g. `<item>.*?</item>` with `re.DOTALL`, handling
indentation/newline separately) to remove the overlapping quantifier.

---

## 2. `.github/workflows/` — 9 issues

### 🟠 MAJOR · `githubactions:S7637` · pin third-party actions by SHA
**Use full commit SHA hash for this dependency.**
Third-party actions by tag (`@v4`/`@v8`) can change without notice. (The
official `actions/*` are not flagged.)
- `build.yml` l. 35, 92, 151 — `jurplel/install-qt-action@v4`
- `ci.yml` l. 33 — `jurplel/install-qt-action@v4`
- `ci.yml` l. 60 — `SonarSource/sonarqube-scan-action@v8`

**Fix:** replace with the full SHA and a tag comment:
`uses: jurplel/install-qt-action::<full-sha>  # v4`. The SHA is on the action's
releases page; the *Pin GitHub Actions* extension or `actionlint` automate it.

### 🟠 MAJOR · `githubactions:S8233` · move the `write` permission to the job
**Move this write permission from workflow level to job level.**
A `write` permission at the workflow level applies to all jobs; it is better to
grant it only to the job that needs it.
- `pages.yml` l. 21, 22 — `pages: write`, `id-token: write`
- `release.yml` l. 11 — `contents: write`

**Fix:**
- `pages.yml`: keep `contents: read` at the workflow; move `pages: write` and
  `id-token: write` to the `deploy` job.
- `release.yml`: remove `contents: write` from the workflow; add
  `permissions: contents: write` to the `publish` and `appcast` jobs.

### 🟠 MAJOR · `githubactions:S6506` · `release.yml` l. 101
**Not enforcing HTTPS here might allow for redirections to insecure websites.**
The `curl -fsSL` follows redirects (`-L`); in theory a redirect could lead to an
insecure site. The URL is HTTPS (GitHub) and the download is verified by
SHA-256 (l. 103), so the risk is low.
**Fix:** add `--proto '=https'` to the `curl` (prevents a redirect to HTTP), or
accept it as a known risk (trusted URL + checksum).

---

## 3. `Metronom/Metronome.cpp` — 5 code smells

### 🟠 MAJOR · `cpp:S8417` · lines 386, 391, 446, 470, 501
**Use `std::memory_order::seq_cst` (or remove this argument to use its default
value) to ensure sequential consistency.**

The atomic operations use `release`/`acquire` (and one `relaxed`):

| Line | Operation | Function |
|---|---|---|
| 386 | `m_tickConfig.store(…, release)` | `publishTickConfig` (GUI) |
| 391 | `m_tickPosition.load(acquire)` | `pollPosition` (GUI) |
| 446 | `m_tickConfig.load(acquire)` | `startBeat` (audio) |
| 470 | `m_tickPosition.store(…, release)` | `startTick` (audio) |
| 501 | `m_tickPosition.store(0, relaxed)` | `start` (init) |

The acquire/release pattern is **correct** for producer/consumer (the audio
thread produces the position and the GUI consumes it; the GUI produces the
config and the audio consumes it). The rule is conservative and prefers
`seq_cst`.

**Fix (recommended):** remove the memory-order argument on the 5 lines (uses the
default `seq_cst`). The cost is negligible — the operations are per tick/beat,
not per sample.
**Alternative:** keep acquire/release (idiomatic) and accept the issues in the
dashboard with the producer/consumer pattern justification.

---

## 4. `android/` — 3 issues

### 🟠 MAJOR · `xml:S6358` · `AndroidManifest.xml` l. 6
**Make sure backup of application data is safe here.**
`android:allowBackup="true"` allows a backup of the app's data (an attacker with
adb/root could copy it).
**Fix:** `android:allowBackup="false"` (the app stores no sensitive data). The
manifest is the Qt template (placeholders `-- %%INSERT_*%% --`); editing the
attributes is safe.

### 🟡 MINOR · `xml:S5332` · `AndroidManifest.xml` l. 6
**"usesCleartextTraffic" is implicitly enabled for older Android versions.**
Without the flag, older Android versions allow non-HTTPS traffic by default.
**Fix:** add `android:usesCleartextTraffic="false"` to the `<application>` (the
app does not need cleartext traffic).

### 🟠 MAJOR · `text:S8569` · `android/build.gradle`
**Dependency versions are not predictable if the lock file is missing.**
Without a lock file, the Gradle dependency versions are not pinned.
**Fix:** generate a `gradle.lockfile` (or `gradle/verification-metadata.xml`).
In Android/Qt this can be cumbersome — evaluate whether it is worth it or
accept it.

---

## 5. `site/` — 3 issues

### 🟠 MAJOR · `text:S8568` · `site/Gemfile`
**Dependency versions are not predictable if the lock file (Gemfile.lock) is
missing.**
The `Gemfile` does not pin versions and the `Gemfile.lock` is ignored
(`site/.gitignore` l. 4).
**Fix:** remove `Gemfile.lock` from the `.gitignore` and commit it (`bundle
lock`), or pin the versions. (The site runs on GitHub Pages, which uses its own
gem set — evaluate whether the lock file is useful.)

### 🟠 MAJOR · `javascript:S9383` · `site/assets/js/site.js` l. 76
**Promises must be awaited, end with a call to .catch, …**
`navigator.clipboard.writeText(…).then(…)` without a rejection handler.
**Fix:** add `.catch(function () {})` at the end (the same pattern already used
on l. 62 for the version fetch).

### 🟠 MAJOR · `Web:PageWithoutTitleCheck` · `site/_layouts/default.html` l. 3
**Add a `<title>` tag to this page.** — *false positive*
The `<title>` is generated by `{% seo %}` in `head.html` (an include), which the
analyzer does not interpret.
**Fix:** accept it as a false positive in the dashboard. (Optional: a literal
`<title>` fallback in the layout.)

---

## 6. `scripts/format-code.sh` — 1 issue

### 🟠 MAJOR · `shell:S7688` · line 11
**Use `[[` instead of `[` for conditional tests.**
`if [ $# -eq 0 ]` uses `[`; the rest of the script already uses `[[` (e.g. l. 4).
**Fix:** replace with `if [[ $# -eq 0 ]]`.

---

## Summary

| Severity | Count | | Type | Count |
|---|---|---|---|---|
| 🔴 BLOCKER | 1 | | Vulnerability | 17 |
| 🟠 MAJOR | 24 | | Code smell | 7 |
| 🟡 MINOR | 1 | | Bug | 2 |

**Suggested order:** start with the BLOCKER (`update-appcast.py`) and the
security MAJORs (path traversal, workflow permissions, Android), then the code
smells (C++ atomics, shell, regex), and finally the ones that can be accepted
(the `<title>` false positive, the lock files).
