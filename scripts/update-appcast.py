#!/usr/bin/env python3
"""Add a release to Rask Metronom's Sparkle appcast (the feed "Check for Updates…" reads).

The release workflow runs it for every tag and pushes the result to the
`appcast` branch, served at
https://raw.githubusercontent.com/rask-project/metronom/appcast/appcast.xml.

    scripts/update-appcast.py appcast.xml --version 1.1.0-beta.1 \
        --build 1.1.0.101 --url https://.../Metronom-1.1.0-beta.1-macos-arm64.zip \
        --length 42121444 --signature BASE64 --notes docs/releases/v1.1.0-beta.1.md \
        --channel beta

A pre-release goes in the "beta" channel, which the app does not read, so
only stable releases are offered. An item with the same build number is replaced,
so running it again for a tag is harmless. Newest first; the appcast keeps
the last --keep items.
"""

import argparse
import email.utils
import html
import pathlib
import re
import sys

HEADER = """<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
  <channel>
    <title>Rask Metronom</title>
    <link>https://github.com/rask-project/metronom/releases</link>
    <description>Rask Metronom releases</description>
    <language>en</language>
"""
FOOTER = """  </channel>
</rss>
"""
ITEM_RE = re.compile(r"[ \t]*<item>.*?</item>\n?", re.DOTALL)
BUILD_RE = re.compile(r"<sparkle:version>([^<]*)</sparkle:version>")


def build_key(build: str) -> tuple:
    return tuple(int(part) if part.isdigit() else 0 for part in build.split("."))


def make_item(args: argparse.Namespace) -> str:
    esc = html.escape
    lines = [
        "    <item>",
        f"      <title>Rask Metronom {esc(args.version)}</title>",
        f"      <link>{esc(args.link)}</link>" if args.link else None,
        f"      <sparkle:version>{esc(args.build)}</sparkle:version>",
        f"      <sparkle:shortVersionString>{esc(args.version)}</sparkle:shortVersionString>",
        f"      <sparkle:channel>{esc(args.channel)}</sparkle:channel>" if args.channel else None,
        f"      <sparkle:minimumSystemVersion>{esc(args.minimum_system)}</sparkle:minimumSystemVersion>",
        f"      <pubDate>{email.utils.formatdate(usegmt=True)}</pubDate>",
    ]
    if args.notes and args.notes.is_file():
        notes = args.notes.read_text(encoding="utf-8").replace("]]>", "]]&gt;")
        lines.append(f'      <description sparkle:format="markdown"><![CDATA[{notes}]]></description>')
    lines += [
        f'      <enclosure url="{esc(args.url)}" length="{args.length}"'
        f' type="application/octet-stream" sparkle:edSignature="{esc(args.signature)}"/>',
        "    </item>",
    ]
    return "\n".join(line for line in lines if line is not None) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("appcast", type=pathlib.Path, help="appcast.xml to update (created if missing)")
    parser.add_argument("--version", required=True, help="the version users see (the tag without v)")
    parser.add_argument("--build", required=True, help="CFBundleVersion of the release")
    parser.add_argument("--url", required=True, help="download URL of the zipped app")
    parser.add_argument("--length", required=True, type=int, help="size of the zip in bytes")
    parser.add_argument("--signature", required=True, help="EdDSA signature of the zip (sign_update -p)")
    parser.add_argument("--notes", type=pathlib.Path, help="Markdown release notes")
    parser.add_argument("--link", help="web page of the release")
    parser.add_argument("--channel", help="Sparkle channel (beta for pre-releases)")
    parser.add_argument("--minimum-system", default="14.4", help="minimum macOS version")
    parser.add_argument("--keep", type=int, default=30, help="items to keep")
    args = parser.parse_args()

    existing = args.appcast.read_text(encoding="utf-8") if args.appcast.exists() else ""
    items = [item.rstrip("\n") + "\n" for item in ITEM_RE.findall(existing)]

    def item_build(item: str) -> str:
        match = BUILD_RE.search(item)
        return match.group(1) if match else ""

    items = [item for item in items if item_build(item) != args.build]
    items.append(make_item(args))
    items.sort(key=lambda item: build_key(item_build(item)), reverse=True)

    args.appcast.write_text(HEADER + "".join(items[: args.keep]) + FOOTER, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
