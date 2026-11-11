// bigfield_calib (update 4 SPEC v1.7 §4.1, §6.5 S-shadow, §8.1; a tool, not a suite): the field model's calibration. From G2
// (B-engine) it plays the dealt references and writes the ten CalibId tables into E/FieldModelCalib.inc and
// Tests/bigfield/dealt_<id>.txt.
// Phase A (§6 PA-48, the lead): the target is registered and does nothing.
#include <cstdio>

int main(int /*Argc*/, char** /*Argv*/)
{
	std::printf("bigfield_calib: nothing to calibrate yet (Update 4 Phase A; B-engine's tool from G2)\n");
	return 0;
}
