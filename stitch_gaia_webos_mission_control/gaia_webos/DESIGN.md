---
name: Gaia WebOS
colors:
  surface: '#111319'
  surface-dim: '#111319'
  surface-bright: '#37393f'
  surface-container-lowest: '#0c0e13'
  surface-container-low: '#191c21'
  surface-container: '#1d2025'
  surface-container-high: '#282a30'
  surface-container-highest: '#33353b'
  on-surface: '#e2e2ea'
  on-surface-variant: '#b9cacb'
  inverse-surface: '#e2e2ea'
  inverse-on-surface: '#2e3036'
  outline: '#849495'
  outline-variant: '#3a494b'
  surface-tint: '#00dce6'
  primary: '#e0fdff'
  on-primary: '#00373a'
  primary-container: '#00f2fe'
  on-primary-container: '#006a70'
  inverse-primary: '#00696f'
  secondary: '#ddb7ff'
  on-secondary: '#490080'
  secondary-container: '#6f00be'
  on-secondary-container: '#d6a9ff'
  tertiary: '#fff5f6'
  on-tertiary: '#640039'
  tertiary-container: '#ffcede'
  on-tertiary-container: '#b6156f'
  error: '#ffb4ab'
  on-error: '#690005'
  error-container: '#93000a'
  on-error-container: '#ffdad6'
  primary-fixed: '#6ff6ff'
  primary-fixed-dim: '#00dce6'
  on-primary-fixed: '#002022'
  on-primary-fixed-variant: '#004f53'
  secondary-fixed: '#f0dbff'
  secondary-fixed-dim: '#ddb7ff'
  on-secondary-fixed: '#2c0051'
  on-secondary-fixed-variant: '#6900b3'
  tertiary-fixed: '#ffd9e4'
  tertiary-fixed-dim: '#ffb0cd'
  on-tertiary-fixed: '#3e0022'
  on-tertiary-fixed-variant: '#8c0053'
  background: '#111319'
  on-background: '#e2e2ea'
  surface-variant: '#33353b'
typography:
  display-hero:
    fontFamily: Plus Jakarta Sans
    fontSize: 56px
    fontWeight: '700'
    lineHeight: 64px
    letterSpacing: -0.03em
  display-hero-mobile:
    fontFamily: Plus Jakarta Sans
    fontSize: 36px
    fontWeight: '700'
    lineHeight: 44px
    letterSpacing: -0.02em
  headline-lg:
    fontFamily: Plus Jakarta Sans
    fontSize: 32px
    fontWeight: '600'
    lineHeight: 40px
    letterSpacing: -0.02em
  headline-lg-mobile:
    fontFamily: Plus Jakarta Sans
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
    letterSpacing: -0.01em
  headline-md:
    fontFamily: Plus Jakarta Sans
    fontSize: 20px
    fontWeight: '600'
    lineHeight: 28px
    letterSpacing: -0.01em
  headline-sm:
    fontFamily: Plus Jakarta Sans
    fontSize: 16px
    fontWeight: '600'
    lineHeight: 24px
  body-lg:
    fontFamily: Inter
    fontSize: 16px
    fontWeight: '400'
    lineHeight: 24px
  body-md:
    fontFamily: Inter
    fontSize: 14px
    fontWeight: '400'
    lineHeight: 20px
  body-sm:
    fontFamily: Inter
    fontSize: 12px
    fontWeight: '400'
    lineHeight: 16px
  telemetry-code:
    fontFamily: JetBrains Mono
    fontSize: 13px
    fontWeight: '500'
    lineHeight: 18px
    letterSpacing: -0.01em
  telemetry-micro:
    fontFamily: JetBrains Mono
    fontSize: 11px
    fontWeight: '400'
    lineHeight: 14px
    letterSpacing: 0.04em
  label-caps:
    fontFamily: JetBrains Mono
    fontSize: 10px
    fontWeight: '600'
    lineHeight: 12px
    letterSpacing: 0.08em
rounded:
  sm: 0.25rem
  DEFAULT: 0.5rem
  md: 0.75rem
  lg: 1rem
  xl: 1.5rem
  full: 9999px
spacing:
  gutter: 1rem
  gutter-lg: 1.5rem
  margin: 1rem
  margin-lg: 2rem
  space-xs: 0.25rem
  space-sm: 0.5rem
  space-md: 1rem
  space-lg: 1.5rem
  space-xl: 2.5rem
---

## Brand & Style

This design system embodies an advanced web desktop operating system and AI mission control center. Built for power users, autonomous agent operators, and frontier AI researchers, the interface evokes the sensation of commanding an orbital intelligence deck: precise, hyper-responsive, and visually weightless.

The aesthetic fuses **refined glassmorphism** with **cybernetic aerospace minimalism**. Frosted vitreous layers float seamlessly above deep-space backdrops, illuminated by targeted atmospheric luminescences and precise structural neon indicators. Dense data streams, terminal diagnostics, and fluid window management coexist harmoniously without cognitive strain.

## Colors

The palette simulates high-contrast cockpit telemetry set against cosmic darkness.

- **Neutral Dark Canvas (`#07090E`)**: Serves as the deep-space baseline for root viewports, complemented by stepped surface tiers:
  - `surface-glass`: `rgba(15, 23, 42, 0.65)` for main window chassis and panel layers.
  - `surface-overlay`: `rgba(7, 9, 14, 0.85)` for modal sheets and context menus.
  - `surface-elevated`: `rgba(30, 41, 59, 0.5)` for interactive inner wells and cards.
- **Primary Cyber Cyan (`#00F2FE`)**: Active execution states, operational confirmations, dynamic audio/AI wave telemetry, and high-priority focuses.
- **Secondary Electric Violet (`#A855F7`)**: Agent neural status, background inference pipelines, and asynchronous execution indicators.
- **Tertiary Neon Magenta (`#EC4899`)**: Critical attention states, manual override authorizations, and high-impact actions.
- **Border Gradients**: Default edges use `rgba(255, 255, 255, 0.08)` to `rgba(255, 255, 255, 0.16)`. Focused components transition into dual-gradient cyan/violet edge glows.

## Typography

The typographic hierarchy separates structural shell interface, body content, and real-time telemetry.

- **Plus Jakarta Sans** provides a sleek geometric identity for window titles, app launcher headers, and high-level dashboard summaries.
- **Inter** ensures neutral, ultra-clear legibility across complex multi-column parameters, data tables, and agent conversation trees.
- **JetBrains Mono** governs hardware diagnostics, neural tokens/sec metrics, memory registers, terminal outputs, and system status tags. All tabular figures across data grids must utilize tabular lining numbers (`font-variant-numeric: tabular-nums`).

## Layout & Spacing

The layout model adapts between a free-floating desktop workspace and a structured responsive grid:

- **Desktop (1024px+)**: Freeform multi-window coordinate space anchored by a top system menu bar (height: 36px) and a bottom centered floating dock (height: 64px, safe bottom margin: `space-lg`). Content within open windows organizes via an 8pt layout scale with flexible sub-panes.
- **Tablet (768px – 1023px)**: Auto-tiled split-screen mode with stage manager snapping (2 active windows maximum). Dock switches to an auto-hiding panel.
- **Mobile (< 768px)**: Single active application viewport with full-screen sheets. Dock converts to a compact bottom navigation bar with a glass backdrop blur.

## Elevation & Depth

Visual hierarchy is constructed through optical diffraction, atmospheric backlight, and strict border luminescences:

- **Backdrop Filters**: All floating windows, system trays, and menus employ `backdrop-filter: blur(20px) saturate(180%)`.
- **Level 1 (Sub-Panels & Data Wells)**: `rgba(15, 23, 42, 0.4)` background, `1px solid rgba(255, 255, 255, 0.05)`, no drop shadow.
- **Level 2 (Active Workspace Windows)**: `rgba(15, 23, 42, 0.65)` background, `1px solid rgba(255, 255, 255, 0.12)`, ambient shadow `0 24px 48px -12px rgba(0, 0, 0, 0.75)`. Active windows display an outer ambient corona: `0 0 24px -4px rgba(0, 242, 254, 0.15)`.
- **Level 3 (Floating Dock & System Menus)**: `rgba(15, 23, 42, 0.75)` background, `1px solid rgba(255, 255, 255, 0.2)`, shadow `0 32px 64px -16px rgba(0, 0, 0, 0.9)`, with edge highlight `inset 0 1px 0 0 rgba(255, 255, 255, 0.2)`.
- **Level 4 (Modal Approval Overrides)**: Ambient back-layer tint `rgba(7, 9, 14, 0.8)` with an emergency or confirmation aura `0 0 40px -8px rgba(236, 72, 153, 0.3)`.

## Shapes

The interface embraces a disciplined modern radius scale (`roundedness: 2` base = `0.5rem` / 8px).

- **Windows & Outer Modal Containers**: `rounded-xl` (24px) for organic, aerospace-grade outer contours.
- **Floating Dock & Pill Buttons**: Full pill shaping (`rounded-full` / 9999px) for persistent system utilities, quick chips, and dock items.
- **Telemetry Cards, Inputs & Code Panels**: `rounded-lg` (16px) to maintain high data density without feeling aggressive.
- **Traffic Light Controls**: Perfect 12px circles (`rounded-full`) nested within window headers.

## Components

### Window Chrome & Traffic Lights
- **Header Bar**: 40px height, drag-region enabled, border-bottom `1px solid rgba(255, 255, 255, 0.06)`.
- **Traffic Lights**: Left-aligned trio spaced 8px apart.
  - Close: `#FF5F56` (Hover icon: `x`)
  - Minimize: `#FFBD2E` (Hover icon: `–`)
  - Maximize: `#27C93F` (Hover icon: `+`)
  - Hover state reveals micro-glyph actions in matching darkened stroke.

### Floating Dock
- Positioned `fixed bottom-4 left-1/2 -translate-x-1/2`.
- Frosted chassis (`rgba(15, 23, 42, 0.75)`, 24px blur, `1px solid rgba(255, 255, 255, 0.15)`).
- App icons scale with dynamic magnification on mouse proximity (1.0x to 1.35x).
- Active app indicator: 4px cyan dot underneath icon with `box-shadow: 0 0 8px #00F2FE`.

### Buttons & Interactive Approvals
- **Primary Action (Cyber Cyan)**: Background `linear-gradient(135deg, #00F2FE 0%, #00B4D8 100%)`, dark text `#07090E`, font weight 600. Glowing focus ring `0 0 0 2px #07090E, 0 0 16px #00F2FE`.
- **Agent Critical Override (Magenta)**: Dual-glow pill button with slide-to-confirm interaction or hold-for-2-seconds press state to approve destructive AI actions.
- **Ghost/Vitreous Button**: Translucent background `rgba(255, 255, 255, 0.05)`, border `1px solid rgba(255, 255, 255, 0.1)`, text `#FFFFFF`. Hover transitions to `rgba(255, 255, 255, 0.12)` with a subtle cyan edge.

### Telemetry Cards & Metric Panels
- Composed of a glassy base with a micro-header displaying `label-caps` in JetBrains Mono.
- Live values show prominent numbers in `headline-md`, flanked by real-time sparklines or SVG status pulses.
- Warning thresholds shift border glow dynamically from cyan to `#EC4899`.

### Input Fields & Terminal Consoles
- Inputs feature `rgba(7, 9, 14, 0.6)` inset background, `1px solid rgba(255, 255, 255, 0.1)`.
- Focused inputs animate a clean neon line along the bottom edge via `#00F2FE`.
- Command prompt uses JetBrains Mono with prompt character `gaia:~$` in cyan and a blinking block cursor.

### Chips & Checkboxes
- **Chips**: Compact height (24px), border `1px solid rgba(255, 255, 255, 0.12)`, font `telemetry-micro`. Active state fills with `rgba(0, 242, 254, 0.15)` and cyan border.
- **Checkboxes & Radios**: Custom 16px squares/circles with `1px solid rgba(255, 255, 255, 0.2)`. Checked state displays a glowing cyan checkmark and inset glow.