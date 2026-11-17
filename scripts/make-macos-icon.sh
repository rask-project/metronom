#!/bin/bash
# Generates resources/metronom.icns from resources/logo.svg (macOS only: needs swift and iconutil).
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

iconset="$work/metronom.iconset"
mkdir -p "$iconset"

cat > "$work/render.swift" <<'EOF'
import AppKit

let args = CommandLine.arguments
let logo = NSImage(contentsOf: URL(fileURLWithPath: args[1]))!
let outDir = args[2]

func render(_ size: Int) -> Data {
    let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: size, pixelsHigh: size, bitsPerSample: 8,
                               samplesPerPixel: 4, hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB,
                               bytesPerRow: 0, bitsPerPixel: 0)!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
    let scale = CGFloat(size) / 1024

    // macOS icon grid: 824pt body inside the 1024pt canvas.
    let body = NSRect(x: 100 * scale, y: 100 * scale, width: 824 * scale, height: 824 * scale)
    let shape = NSBezierPath(roundedRect: body, xRadius: 185 * scale, yRadius: 185 * scale)

    NSGraphicsContext.saveGraphicsState()
    let shadow = NSShadow()
    shadow.shadowColor = NSColor.black.withAlphaComponent(0.3)
    shadow.shadowOffset = NSSize(width: 0, height: -10 * scale)
    shadow.shadowBlurRadius = 20 * scale
    shadow.set()
    NSColor(srgbRed: 0x26 / 255, green: 0x30 / 255, blue: 0x2D / 255, alpha: 1).setFill()
    shape.fill()
    NSGraphicsContext.restoreGraphicsState()

    let gradient = NSGradient(starting: NSColor(srgbRed: 0x3A / 255, green: 0x47 / 255, blue: 0x43 / 255, alpha: 1),
                              ending: NSColor(srgbRed: 0x1C / 255, green: 0x23 / 255, blue: 0x21 / 255, alpha: 1))!
    gradient.draw(in: shape, angle: -90)

    let logoSide = 640 * scale
    logo.draw(in: NSRect(x: (CGFloat(size) - logoSide) / 2, y: (CGFloat(size) - logoSide) / 2,
                         width: logoSide, height: logoSide))

    NSGraphicsContext.restoreGraphicsState()
    return rep.representation(using: .png, properties: [:])!
}

for base in [16, 32, 128, 256, 512] {
    try! render(base).write(to: URL(fileURLWithPath: "\(outDir)/icon_\(base)x\(base).png"))
    try! render(base * 2).write(to: URL(fileURLWithPath: "\(outDir)/icon_\(base)x\(base)@2x.png"))
}
EOF

swift "$work/render.swift" "$root/resources/logo.svg" "$iconset"
iconutil -c icns "$iconset" -o "$root/resources/metronom.icns"
echo "Wrote resources/metronom.icns"
