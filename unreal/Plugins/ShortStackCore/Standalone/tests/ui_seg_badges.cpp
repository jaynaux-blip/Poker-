// riverline_ui: core-badges's additions (next_segment_spec §7.6, §8.3), compiled into ui_test and called from main() in
// ui_main.cpp (the shared kit is UiSeg.h):
// the badges goldens (badges_board_live, badges_board_honors, badges_board_hover, badges_card_rail, badges_card_tab,
// badges_career_progress, badges_embercrest_page, badges_table_seat, badges_kast_overlay, badges_atlas,
// badges_room_leaders), every glyph's 12 px draw list differing from every other's.
//
// Phase A, day 1: the track's own file, registered and empty. core-badges fills it in Phase B.
#include "UiSeg.h"

#include "ShortStack/Game/Badges.h"
#include "ShortStack/UI/BadgeArt.h"

namespace ui_test
{
void SegBadges()
{
}
} // namespace ui_test
