#!/bin/sh
# Previews the site at http://127.0.0.1:4000/metronom/ with live rebuilds.
# Needs Jekyll: `cd site && bundle install` (the github-pages gem, the same
# versions GitHub Pages builds with).
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
"$root/scripts/site-assets.sh"
cd "$root/site"
if [ -f Gemfile.lock ] || bundle check >/dev/null 2>&1; then
    exec bundle exec jekyll serve --livereload "$@"
fi
exec jekyll serve --livereload "$@"
