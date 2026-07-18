#ifndef UI_THEME_H
#define UI_THEME_H

#include <raylib.h>

namespace UiTheme {

inline constexpr Color PanelBg()      { return { 12, 14, 22, 210 }; }
inline constexpr Color PanelBorder()  { return { 255, 236, 200, 55 }; }
inline constexpr Color PanelInner()   { return { 255, 255, 255, 12 }; }
inline constexpr Color AccentGold()   { return { 232, 188, 96, 255 }; }
inline constexpr Color AccentWarm()   { return { 255, 168, 88, 255 }; }
inline constexpr Color TextPrimary()  { return { 255, 250, 242, 255 }; }
inline constexpr Color TextMuted()    { return { 190, 180, 165, 220 }; }
inline constexpr Color TextDark()     { return { 28, 24, 20, 255 }; }
inline constexpr Color ReadyGreen()   { return { 90, 200, 120, 255 }; }
inline constexpr Color Danger()       { return { 210, 70, 70, 255 }; }
inline constexpr Color SkyAccent()    { return { 120, 190, 230, 255 }; }
inline constexpr Color ButtonIdle()   { return { 28, 30, 40, 200 }; }
inline constexpr Color ButtonHover()  { return { 42, 46, 60, 230 }; }
inline constexpr Color DimOverlay()   { return { 0, 0, 0, 140 }; }

inline constexpr float FontTitle = 56.0f;
inline constexpr float FontSubtitle = 22.0f;
inline constexpr float FontButton = 28.0f;
inline constexpr float FontBody = 18.0f;
inline constexpr float FontSmall = 14.0f;

inline constexpr float PanelRadiusPad = 4.0f;

} // namespace UiTheme

#endif // UI_THEME_H
