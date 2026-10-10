// riverline_ui: core-sponsor's additions (next_segment_spec §7.6, §8.3), compiled into ui_test and called from main() in
// ui_main.cpp (the shared kit is UiSeg.h):
// Mail (mail_empty, mail_offer_luckypenny, mail_offer_conflict, mail_counter_mood, mail_contract_tally, mail_strike,
// mail_ended), Kast (kast_channel_sponsors, kast_overlay_marks), the art (sponsor_patch_<id> x13, ambassador_banner,
// polaroid), embercrest_card_covered.
//
// Phase A, day 1: the track's own file, registered and empty. core-sponsor fills it in Phase B.
#include "UiSeg.h"

#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/SponsorArt.h"

namespace ui_test
{
void SegSponsor()
{
}
} // namespace ui_test
