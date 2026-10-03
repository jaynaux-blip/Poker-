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

SHORTSTACKCORE_API void EvictionNotice(Canvas& C);
SHORTSTACKCORE_API void PowerBill(Canvas& C);
SHORTSTACKCORE_API void StickyNote(Canvas& C, const std::vector<std::string>& Lines, const Color& Paper);
SHORTSTACKCORE_API void Poster(Canvas& C);
/** Transparent background: the host adds glow (bloom) by tinting brighter than white. */
SHORTSTACKCORE_API void NeonSign(Canvas& C);
SHORTSTACKCORE_API void Keyboard(Canvas& C);

// The street outside (the Street level): the Lucky Penny #212, its windows and the corner's signs.
constexpr float StoreSignW = 1600.0f;
constexpr float StoreSignH = 300.0f;
constexpr float OpenSignW = 640.0f;
constexpr float OpenSignH = 280.0f;
constexpr float StreetSignW = 900.0f;
constexpr float StreetSignH = 200.0f;
constexpr float BuildingNumberW = 400.0f;
constexpr float BuildingNumberH = 90.0f;
constexpr float DoorDecalW = 400.0f;
constexpr float DoorDecalH = 520.0f;
constexpr float PromoW = 600.0f;
constexpr float PromoH = 900.0f;
/** The backlit box sign over the store's windows. */
SHORTSTACKCORE_API void StoreSign(Canvas& C);
/** A neon OPEN in the window (transparent background, tint brighter than white for bloom). */
SHORTSTACKCORE_API void OpenSign(Canvas& C);
/** A green street-name blade: "FIFTH ST" with its block number. */
SHORTSTACKCORE_API void StreetSign(Canvas& C, const std::string& Name, const std::string& Block);
/** The apartment building's number over its door (BuildingNumberW x BuildingNumberH): brass on dark enamel. */
SHORTSTACKCORE_API void BuildingNumber(Canvas& C, const std::string& Number);
/** The hours sticker on the store's door. */
SHORTSTACKCORE_API void DoorDecal(Canvas& C);
/** A window poster for one of the store's products ("2 for $5"). */
SHORTSTACKCORE_API void Promo(Canvas& C, const std::string& ItemId, const std::string& Deal);
} // namespace props
} // namespace ui
} // namespace ss
