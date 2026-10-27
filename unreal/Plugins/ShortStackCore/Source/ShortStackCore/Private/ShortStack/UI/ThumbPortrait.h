#pragma once

// Update 3 (SPEC §6.1): the seam between the portrait (Portrait.cpp) and Rerun's thumbnail compositor (ThumbArt.cpp), both
// ui-thumbs' files. Private to the module: no screen but the thumbnails draws a person's cut-out.
#include "ShortStack/Game/Hero.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/Portrait.h"

#include <vector>

namespace ss
{
namespace ui
{
namespace thumbportrait
{
/**
 * The person DrawPortrait's expression overload draws at (X, Y, Scale) with E and P, as convex pieces in the canvas's
 * units: the head with its hair and any hat (tilted with the head), the neck, the torso, and the pose's arms and hands.
 * The thumbnails' cut-out halo grows each piece and glows it (one feathered concave path would overlap its own fringe).
 */
std::vector<std::vector<Vec2>> Silhouette(const hero::Character& Who, float X, float Y, float Scale, const PortraitExpression& E, PortraitPose P);
/** The side a one-armed pose (Point, FistPump, Facepalm) uses: +1 the frame's right, -1 its left (from where the eyes look). */
float PoseSide(const PortraitExpression& E);
} // namespace thumbportrait
} // namespace ui
} // namespace ss
