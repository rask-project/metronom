# Rask Metronom site

The project's home page, built by Jekyll and published to GitHub Pages
(https://rask-project.github.io/metronom/) by `.github/workflows/pages.yml`
on every push to `main` that touches it.

- Every page exists in the app's languages: English at the root and `pt-br/`, `es/`, `fr/`, `it/`
  (`_data/languages.yml`). Each page file (`index.html`, `features.html`, `guide.html` — How to use —,
  `download.html`, `news.html`) is only front matter (`lang`, `ref`, `permalink`, the translated `title` and
  `description`) and includes its markup from `_includes/pages/`, shared by all languages.
- The text is in `_data/i18n/<lang>.yml`, one file per language with the same keys; note names and
  settings follow the app's `translations/*.ts`. `%quarter%`, `%play%`, `%base%`… in a string are replaced
  by `_includes/tr.html` (a note icon and name, an icon, the language's root URL); `{name}` placeholders in
  `js:` are filled in by `site.js`. A new string goes into every language file.
- `_includes/languages.html` lists the same page in every language (the menu's language switcher and the
  `hreflang` links). Posts and `404.html` are in English only.
- `_layouts/` (`default` → `page`, `post`) and `_includes/` (menu bar, footer, `i18n.html` for the page's
  language, `icon.html` for a line icon, `tile.html` for an icon on the orange tile, `shot.html` for a
  screenshot, `beatbar.html` for a beat bar diagram, `rows.html` for Settings-style rows, `note.html` for a
  note value's name with its icon).
- `assets/css/site.css` (all styles; dark, macOS look, the app's orange),
  `assets/js/site.js` (the Download buttons point at the latest release's files through the GitHub API; Copy
  buttons; the Features sidebar; the beat explorer on How to use, with the same rules as `Metronome.cpp` and
  a Web Audio player that plays the app's three clicks) and `assets/glyphs/` (the site's own line icons).
- Screenshots, note icons and the logo are **not** kept here:
  `scripts/site-assets.sh` copies `docs/images`, `resources/icons/metronom/svg`
  and `resources/logo.png` into `assets/` (ignored by git) before a build.

## Screenshots

The GIFs and screenshots in `docs/images` show the macOS window over the
desktop, 40 points of wallpaper around it, so the blur shows. Record at 2× and
convert a GIF to 460 px wide at 20 fps with one palette for the whole clip
(`palettegen=stats_mode=full`, `paletteuse=dither=bayer`): that keeps the
wallpaper from flickering and each GIF under 2 MB.

## Preview

```sh
cd site && bundle install   # once: the github-pages gem (Ruby 3.x)
scripts/site-serve.sh       # http://127.0.0.1:4000/metronom/
```

## A new post

Add `_posts/YYYY-MM-DD-slug.md`:

```markdown
---
title: "Rask Metronom 3.1.0: …"
summary: "One sentence for the News list and the page's lead."
release: v3.1.0   # optional: links the GitHub release
---

Markdown body (a release's notes from docs/releases/ fit as they are).
```
