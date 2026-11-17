#!/bin/bash
# Version string for package file names: the tag on a tag build, otherwise "<last tag or 0.0.0>-<short commit>".
set -euo pipefail

if [[ "${GITHUB_REF_TYPE:-}" == "tag" ]]; then
    echo "${GITHUB_REF_NAME#v}"
    exit 0
fi

tag="$(git describe --tags --abbrev=0 2>/dev/null || echo v0.0.0)"
echo "${tag#v}-$(git rev-parse --short=7 HEAD)"
