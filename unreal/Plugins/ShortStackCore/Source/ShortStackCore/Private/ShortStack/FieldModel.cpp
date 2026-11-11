// Update 4, the big fields (SPEC v1.7 §4.1): the field model's tables and, from G2/G3, its bodies (B-engine: BuildModel, the
// ring, ModelSome's modelled hand, ModelBalance, the merged eliminations' model side and ck2).
//
// Phase A (§6 PA-16, the lead): the calibration (E/FieldModelCalib.inc, Std8 filled from today's evidence) and CalibFor, the
// format rule. Nothing builds a model before G3, so nothing reads either yet.
#include "ShortStack/FieldModel.h"
#include "StrictFloat.h"

#include <array>
#include <string>

namespace ss
{
namespace fieldmodel
{
namespace fieldmodel_detail
{
using FieldCalibTables = std::array<CalibTable, static_cast<size_t>(CalibId::Count)>;

/** The ten checked-in tables, in CalibId's order. */
const FieldCalibTables& FieldCalibAll()
{
	static const FieldCalibTables Tables = {{
#include "FieldModelCalib.inc"
	}};
	return Tables;
}
} // namespace fieldmodel_detail

const CalibTable& Calibration(CalibId Id)
{
	const size_t I = static_cast<size_t>(Id);
	return fieldmodel_detail::FieldCalibAll()[I < static_cast<size_t>(CalibId::Count) ? I : 0];
}

CalibId CalibFor(int TableSize, const std::string& Speed, bool Bounty)
{
	// Size class 6 for tables of 6 or fewer, else 8 (§4.1.4); the PKO tables by size class alone; speed Hyper, Turbo, Deep, and
	// Regular (or anything else) is Std.
	const bool Six = TableSize > 0 && TableSize <= 6;
	if (Bounty)
	{
		return Six ? CalibId::Pko6 : CalibId::Pko8;
	}
	if (Speed == "Hyper")
	{
		return Six ? CalibId::Hyper6 : CalibId::Hyper8;
	}
	if (Speed == "Turbo")
	{
		return Six ? CalibId::Turbo6 : CalibId::Turbo8;
	}
	if (Speed == "Deep")
	{
		return Six ? CalibId::Deep6 : CalibId::Deep8;
	}
	return Six ? CalibId::Std6 : CalibId::Std8;
}
} // namespace fieldmodel
} // namespace ss
