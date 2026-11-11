// Update 4, the light card (SPEC v1.7 §5.9): a crowd regular's card by name over any page (World::CrowdProfileOf, every number from
// the record), the crowd's rows in RISERS and FIRST-YEARS and THE SCENE's counter. S-ui's file from G4.
//
// Phase A (§6 PA-47; the lead): stubs. ShowCrowd opens nothing and CrowdCard, which Draw calls after the app's frame, draws nothing;
// no crowd seat exists before G4 (Session::FieldCrowd is empty).
#include "ShortStack/UI/RiverLine.h"

#include <string>

namespace ss
{
namespace ui
{
void RiverLine::ShowCrowd(const std::string& /*Name*/, double /*Now*/)
{
}

void RiverLine::CrowdCard(double /*Now*/)
{
}
} // namespace ui
} // namespace ss
