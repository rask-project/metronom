#import <AppKit/AppKit.h>
#import <objc/runtime.h>

#include "MacWindowEffects.h"

#include <QGuiApplication>

#include <algorithm>
#include <array>
#include <bit>

namespace
{
const char effectViewKey{};
const char toolbarKey{};
const char inputForwardingViewKey{};

constexpr std::array windowButtonTypes{NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton};

NSWindow *windowFromId(WId winId)
{
    // Only under the cocoa platform plugin is the window id an NSView.
    if (QGuiApplication::platformName() != QLatin1String("cocoa")) {
        return nil;
    }
    NSView *const view{(__bridge NSView *) std::bit_cast<void *>(winId)};
    return [view window];
}
} // namespace

// Sits above AppKit's toolbar so clicks in the title bar area reach the Qt content view,
// except on the window buttons and the resize border.
@interface MetronomInputForwardingView : NSView
@property(nonatomic, weak) NSWindow *targetWindow;
@property(nonatomic, weak) NSView *targetView;
@end

@implementation MetronomInputForwardingView

- (BOOL)isOpaque
{
    return NO;
}

- (NSView *)hitTest:(NSPoint)point
{
    NSView *const parentView{[self superview]};
    NSWindow *const window{self.targetWindow};
    NSView *const targetView{self.targetView};
    if (parentView == nil || window == nil || targetView == nil) {
        return nil;
    }

    auto const buttonType{std::ranges::find_if(windowButtonTypes, [window, parentView, point](NSWindowButton type) {
        NSButton *const button{[window standardWindowButton:type]};
        return button != nil && NSPointInRect(point, [button convertRect:[button bounds] toView:parentView]);
    })};
    if (buttonType != windowButtonTypes.end()) {
        return [window standardWindowButton:*buttonType];
    }

    NSRect const forwardingRect{[self convertRect:[self bounds] toView:parentView]};
    if (!NSPointInRect(point, NSInsetRect(forwardingRect, 12.0, 12.0))) {
        return nil;
    }
    return targetView;
}

@end

namespace
{
void installBlur(NSWindow *window, NSView *frameView, NSView *contentView, qreal alpha)
{
    NSVisualEffectView *effectView{objc_getAssociatedObject(window, &effectViewKey)};
    if (effectView == nil) {
        // The frame view also covers the title bar, so the blur extends under it.
        effectView = [[NSVisualEffectView alloc] initWithFrame:[frameView bounds]];
        [effectView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [effectView setBlendingMode:NSVisualEffectBlendingModeBehindWindow];
        [effectView setMaterial:NSVisualEffectMaterialUnderWindowBackground];
        [effectView setState:NSVisualEffectStateActive];
        objc_setAssociatedObject(window, &effectViewKey, effectView, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [frameView addSubview:effectView positioned:NSWindowBelow relativeTo:contentView];
    }
    [effectView setAlphaValue:alpha];
}

void installToolbar(NSWindow *window)
{
    if (objc_getAssociatedObject(window, &toolbarKey) != nil) {
        return;
    }
    // An empty toolbar gives the taller unified title bar with vertically centred buttons.
    NSToolbar *const toolbar{[[NSToolbar alloc] initWithIdentifier:@"MetronomEmptyToolbar"]};
    [toolbar setDisplayMode:NSToolbarDisplayModeIconOnly];
    objc_setAssociatedObject(window, &toolbarKey, toolbar, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [window setToolbar:toolbar];
    [window setToolbarStyle:NSWindowToolbarStyleUnified];
    [window setTitlebarSeparatorStyle:NSTitlebarSeparatorStyleNone];
}

void raiseContentAboveToolbar(NSWindow *window, NSView *frameView, NSView *contentView)
{
    [frameView addSubview:contentView positioned:NSWindowAbove relativeTo:nil];

    std::ranges::for_each(windowButtonTypes, [window, frameView](NSWindowButton type) {
        NSView *container{[window standardWindowButton:type]};
        while ([container superview] != nil && [container superview] != frameView) {
            container = [container superview];
        }
        if ([container superview] == frameView) {
            [frameView addSubview:container positioned:NSWindowAbove relativeTo:nil];
        }
    });

    MetronomInputForwardingView *inputView{objc_getAssociatedObject(window, &inputForwardingViewKey)};
    if (inputView == nil) {
        inputView = [[MetronomInputForwardingView alloc] initWithFrame:[frameView bounds]];
        [inputView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        objc_setAssociatedObject(window, &inputForwardingViewKey, inputView, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [inputView setTargetWindow:window];
    [inputView setTargetView:contentView];
    [frameView addSubview:inputView positioned:NSWindowAbove relativeTo:nil];
}
} // namespace

void MacWindowEffects::applyTranslucentWindow(WId winId, const QColor &color, qreal alpha, bool dark)
{
    NSWindow *const window{windowFromId(winId)};
    if (window == nil) {
        return;
    }

    [window setAppearance:[NSAppearance appearanceNamed:dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua]];
    [window setTitleVisibility:NSWindowTitleHidden];
    [window setOpaque:NO];
    [window setBackgroundColor:[NSColor colorWithRed:color.redF() green:color.greenF() blue:color.blueF() alpha:alpha]];

    NSView *const contentView{[window contentView]};
    NSView *const frameView{[contentView superview]};
    if (frameView == nil) {
        return;
    }

    installBlur(window, frameView, contentView, alpha);
    installToolbar(window);
    raiseContentAboveToolbar(window, frameView, contentView);
}

qreal MacWindowEffects::windowButtonsTrailingEdge(WId winId)
{
    NSWindow *const window{windowFromId(winId)};
    if (window == nil) {
        return 0.0;
    }

    NSView *const contentView{[window contentView]};
    [[contentView superview] layoutSubtreeIfNeeded];

    CGFloat edge{0.0};
    std::ranges::for_each(windowButtonTypes, [window, contentView, &edge](NSWindowButton type) {
        NSButton *const button{[window standardWindowButton:type]};
        if (button == nil || [button isHidden]) {
            return;
        }
        edge = std::max(edge, NSMaxX([button convertRect:[button bounds] toView:contentView]));
    });
    return edge;
}

void MacWindowEffects::setWindowResizable(WId winId, bool resizable)
{
    NSWindow *const window{windowFromId(winId)};
    if (window == nil) {
        return;
    }
    const NSWindowStyleMask mask{[window styleMask]};
    [window setStyleMask:resizable ? (mask | NSWindowStyleMaskResizable) : (mask & ~NSWindowStyleMaskResizable)];
}
