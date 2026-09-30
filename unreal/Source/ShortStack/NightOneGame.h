#pragma once

#include "CoreMinimal.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Phone.h"
#include "ShortStack/UI/RiverLine.h"
#include "SlateDrawList.h"

class ANightOneGameMode;

/** The game session and its hooks into the world (sound, the phone, the desk, saves). */
class FNightOneGame final : public ss::SessionHooks
{
public:
	FNightOneGame(ANightOneGameMode& InMode, const ss::SaveData* Loaded, const std::string& Seed);

	ANightOneGameMode& Mode;
	ss::Session Session;
	ss::ui::RiverLine Client;
	ss::ui::PhoneScreen Phone;
	FSlateTextMeasurer Measurer;
	double RealNow = 0.0;

	virtual void Sound(ss::SoundId Id, double Volume) override;
	virtual void Text(const std::string& From, const std::string& Body) override;
	virtual void Heartbeat(bool bOn) override;
	virtual void AddCan() override;
	virtual void Celebrate() override;
	virtual void Save(const ss::SaveData& Data) override;
};
