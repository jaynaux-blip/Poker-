// riverline_ui: core-stake's additions (next_segment_spec §7.6, §8.3), compiled into ui_test and called from main() in
// ui_main.cpp (the shared kit is UiSeg.h):
// the Backing page goldens (backing_empty, backing_deals, backing_sell, backing_offer, backing_review), the
// Embercrest card staked (embercrest_card_staked), the cage split (cage_receipt_split).
//
// Phase A, day 1: the track's own file, registered and empty. core-stake fills it in Phase B.
#include "UiSeg.h"

#include "ShortStack/Game/Stake.h"

namespace ui_test
{
void SegStake()
{
}
} // namespace ui_test
