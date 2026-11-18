#!/bin/sh
# Copies what the site shows from the repository into site/assets, so the
# screenshots, icons and logo are kept in one place (docs/images, resources/).
# Run by .github/workflows/pages.yml before the build, and by scripts/site-serve.sh.
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
assets="$root/site/assets"
rm -rf "$assets/screenshots" "$assets/icons"
mkdir -p "$assets/screenshots" "$assets/icons"
cp "$root"/docs/images/* "$assets/screenshots/"
cp "$root"/resources/icons/metronom/svg/*.svg "$assets/icons/"
cp "$root/resources/logo.png" "$assets/logo.png"
