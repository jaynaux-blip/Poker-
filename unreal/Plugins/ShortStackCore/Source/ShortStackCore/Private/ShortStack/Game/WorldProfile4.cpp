// Update 4, the card's story (SPEC v1.7 §3.19): World::StoryOf (the STORY tab's data), the Notebook's views, the ProfileOf cache and
// World::SceneChart. W-ui's file from G2.
//
// Phase A (§6 PA-19, PA-30; the lead): ProfileOf's fall-through, story::ProfileExtras, adds nothing.
#include "ShortStack/Game/WorldStory.h"

#include "ShortStack/Game/World.h"

namespace ss
{
namespace world
{
namespace story
{
void ProfileExtras(const World& /*W*/, const Npc& /*N*/, Profile& /*Pr*/)
{
	// Phase A: the card is today's.
}
} // namespace story

// Update 4 (LEAD_CALLS Q-5; the gate sections' surfaces, declared before the freeze): display reads, empty in Phase A (W-ui fills
// them at G2, cached by (id, World::Rev)).
const StoryView& World::StoryOf(int /*Npc*/) const
{
	static const StoryView ProfStoryNone;
	return ProfStoryNone;
}

const NotebookView& World::NotebookOf(NotebookTab Tab) const
{
	static const NotebookView ProfNotebookNone[static_cast<size_t>(NotebookTab::Count)] = {
		{NotebookTab::Following, {}}, {NotebookTab::SinceYouLooked, {}}, {NotebookTab::NamesToWatch, {}}, {NotebookTab::Risers, {}},
		{NotebookTab::FirstYears, {}}, {NotebookTab::Scene, {}}, {NotebookTab::You, {}}};
	const size_t K = static_cast<size_t>(Tab) < static_cast<size_t>(NotebookTab::Count) ? static_cast<size_t>(Tab) : 0;
	return ProfNotebookNone[K];
}

const SceneChartView& World::SceneChart() const
{
	static const SceneChartView ProfSceneNone;
	return ProfSceneNone;
}
} // namespace world
} // namespace ss
