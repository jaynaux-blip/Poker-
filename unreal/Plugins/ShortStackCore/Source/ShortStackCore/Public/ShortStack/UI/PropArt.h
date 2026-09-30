#pragma once

#include "ShortStack/UI/Canvas.h"

namespace ss
{
namespace ui
{
/**
 * Printed and glowing things in the apartment, drawn with the same vector
 * canvas as the laptop client (ports of the canvas textures in
 * web/src/scene/props.ts and outside.ts). Hosts show them on quads.
 */
namespace props
{
/** Paper sizes in canvas units (roughly 3.3 units per millimeter). */
constexpr float NoticeW = 700.0f;
constexpr float NoticeH = 990.0f;
constexpr float BillW = 700.0f;
constexpr float BillH = 910.0f;
constexpr float NoteSize = 256.0f;
constexpr float PosterW = 600.0f;
constexpr float PosterH = 900.0f;
constexpr float NeonW = 1024.0f;
constexpr float NeonH = 320.0f;
constexpr float KeyboardW = 1024.0f;
constexpr float KeyboardH = 440.0f;

void EvictionNotice(Canvas& C);
void PowerBill(Canvas& C);
void StickyNote(Canvas& C, const std::vector<std::string>& Lines, const Color& Paper);
void Poster(Canvas& C);
/** Transparent background: the host adds glow (bloom) by tinting brighter than white. */
void NeonSign(Canvas& C);
void Keyboard(Canvas& C);
} // namespace props
} // namespace ui
} // namespace ss
