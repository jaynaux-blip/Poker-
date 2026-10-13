#include "ShortStack/Game/LiveAuto.h"

// The live skip (update 2 §2.12). Phase A, day 1 (the lead): a stub that plays nothing (nothing calls it yet).
// core-autoplay owns this file from here.

namespace ss
{
namespace live
{
SkipOut SkipSlice(Tournament& T, const style::HeroBrain& B, const autoplay::TableAuto& A, double BudgetMs, SkipSink& Sink)
{
	(void)T;
	(void)B;
	(void)A;
	(void)BudgetMs;
	(void)Sink;
	SkipOut Out;
	Out.Why = SkipWhy::Stopped;
	return Out;
}
} // namespace live
} // namespace ss
