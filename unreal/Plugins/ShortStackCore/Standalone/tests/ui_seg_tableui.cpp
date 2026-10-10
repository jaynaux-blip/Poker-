// riverline_ui: core-tableui's additions (next_segment_spec §7.6, §8.3), compiled into ui_test and called from main() in
// ui_main.cpp (the shared kit is UiSeg.h):
// the table HUD (table_hud_turn_open, table_hud_allin_count, table_hud_break, table_hud_bubble, table_hud_itm,
// table_hud_onfoot_marker, table_hud_rail_onair, table_hud_dees), table_card_night, desk_register_funding,
// ControlParity, SettingsMigration.
//
// Phase A, day 1: the track's own file, registered and empty. core-tableui fills it in Phase B.
#include "UiSeg.h"

#include "ShortStack/UI/Captions.h"
#include "ShortStack/UI/TableHud.h"

namespace ui_test
{
void SegTableUi()
{
}
} // namespace ui_test
