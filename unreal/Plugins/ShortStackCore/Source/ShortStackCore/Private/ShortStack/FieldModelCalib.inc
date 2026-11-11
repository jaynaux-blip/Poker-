// E/FieldModelCalib.inc: the field model's calibration, one CalibTable per fieldmodel::CalibId in CalibId's order (SPEC v1.7 §4.1.4,
// §4.1.9). Generated, checked in, never edited by hand: S-shadow's bigfield_calib (tests/bigfield_calib.cpp) rewrites it whole.
// Each table is [n players 0..9][depth band 0..9] of {Called, PotNon}; rows n = 0 and 1 are unused (Cell's defaults).
//
// Phase A (SPEC §6 PA-16, the lead): Std8 from today's evidence, durable/update4/model/fest_bigfield/runs/calib_all.txt (8/9-max
// micro, standard levels; the 13 dealt runs the file holds: 1,000 x8, 3,000 x4 and 10,000, the 30,000 run having finished after
// it was written), aggregated by the probe's rule (probe_main.cpp:240-323): cells summed over the runs; a cell with 30 or more
// hands is calibrated (Called = called hands / hands; PotNon = big blinds in pots nobody shoved / those hands, 3 when none); the
// rest take the nearest calibrated cell (distance 3|dn| + |db|). 37 cells filled from a neighbour ([n][band] <- [n][band]):
//   [2][0] <- [2][2], [2][1] <- [2][2], [2][6] <- [2][5], [2][7] <- [2][5], [2][8] <- [2][5], [2][9] <- [2][5], [3][0] <- [3][2], [3][1] <- [3][2], [3][4] <- [3][3],
//   [3][5] <- [3][3], [3][6] <- [3][3], [3][7] <- [3][3], [3][8] <- [3][3], [3][9] <- [3][3], [4][0] <- [4][2], [4][1] <- [4][2], [4][4] <- [4][3], [4][5] <- [4][3],
//   [4][6] <- [4][3], [4][7] <- [4][3], [4][8] <- [4][3], [4][9] <- [4][3], [5][0] <- [5][1], [5][5] <- [5][4], [5][6] <- [5][4], [5][7] <- [5][4], [5][8] <- [5][4],
//   [5][9] <- [5][4], [6][0] <- [6][1], [6][5] <- [6][4], [6][6] <- [6][4], [6][7] <- [6][4], [6][8] <- [7][8], [6][9] <- [7][8], [7][9] <- [7][8], [8][9] <- [8][8],
//   [9][9] <- [9][8].
// The other nine are unfilled (every cell Called 0, PotNon 3) until S-shadow writes them. Nothing reads any of them before G3.
// Written by ssb/a1-engine/calib_inc.py. Included by E/FieldModel.cpp inside the initializer of its table array.
// Std8 (CalibId 0)
CalibTable{{
	{{{0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}}}, // n = 0 (unused)
	{{{0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}, {0.000000f, 3.000000f}}}, // n = 1 (unused)
	{{{0.137255f, 2.420523f}, {0.137255f, 2.420523f}, {0.137255f, 2.420523f}, {0.066667f, 2.637343f}, {0.023256f, 3.331976f}, {0.051282f, 3.741487f}, {0.051282f, 3.741487f}, {0.051282f, 3.741487f}, {0.051282f, 3.741487f}, {0.051282f, 3.741487f}}}, // n = 2
	{{{0.232558f, 2.839909f}, {0.232558f, 2.839909f}, {0.232558f, 2.839909f}, {0.104167f, 6.621582f}, {0.104167f, 6.621582f}, {0.104167f, 6.621582f}, {0.104167f, 6.621582f}, {0.104167f, 6.621582f}, {0.104167f, 6.621582f}, {0.104167f, 6.621582f}}}, // n = 3
	{{{0.285714f, 3.538829f}, {0.285714f, 3.538829f}, {0.285714f, 3.538829f}, {0.079365f, 4.260242f}, {0.079365f, 4.260242f}, {0.079365f, 4.260242f}, {0.079365f, 4.260242f}, {0.079365f, 4.260242f}, {0.079365f, 4.260242f}, {0.079365f, 4.260242f}}}, // n = 4
	{{{0.571429f, 4.496611f}, {0.571429f, 4.496611f}, {0.205357f, 4.555528f}, {0.153846f, 4.411831f}, {0.048387f, 5.922204f}, {0.048387f, 5.922204f}, {0.048387f, 5.922204f}, {0.048387f, 5.922204f}, {0.048387f, 5.922204f}, {0.048387f, 5.922204f}}}, // n = 5
	{{{0.450000f, 5.415152f}, {0.450000f, 5.415152f}, {0.286486f, 6.059076f}, {0.189349f, 5.878255f}, {0.060000f, 8.745724f}, {0.060000f, 8.745724f}, {0.060000f, 8.745724f}, {0.060000f, 8.745724f}, {0.053061f, 14.574409f}, {0.053061f, 14.574409f}}}, // n = 6
	{{{0.624161f, 9.738268f}, {0.389968f, 6.770019f}, {0.245680f, 6.845243f}, {0.166058f, 7.262736f}, {0.128660f, 8.834091f}, {0.071342f, 9.945800f}, {0.064516f, 10.521414f}, {0.029915f, 14.819537f}, {0.053061f, 14.574409f}, {0.053061f, 14.574409f}}}, // n = 7
	{{{0.616176f, 9.628928f}, {0.363643f, 8.544262f}, {0.223857f, 8.260491f}, {0.154302f, 8.905471f}, {0.112674f, 9.962566f}, {0.080349f, 11.367288f}, {0.067336f, 13.126776f}, {0.054628f, 14.550607f}, {0.045871f, 15.809133f}, {0.045871f, 15.809133f}}}, // n = 8
	{{{0.650980f, 7.501136f}, {0.442817f, 6.967125f}, {0.296984f, 7.291724f}, {0.194189f, 8.547643f}, {0.135663f, 10.293162f}, {0.104402f, 11.871481f}, {0.079609f, 14.066683f}, {0.057888f, 15.348275f}, {0.053790f, 17.305605f}, {0.053790f, 17.305605f}}}, // n = 9
}},
// Turbo8 (CalibId 1): not yet calibrated (S-shadow).
CalibTable{},
// Hyper8 (CalibId 2): not yet calibrated (S-shadow).
CalibTable{},
// Deep8 (CalibId 3): not yet calibrated (S-shadow).
CalibTable{},
// Std6 (CalibId 4): not yet calibrated (S-shadow).
CalibTable{},
// Turbo6 (CalibId 5): not yet calibrated (S-shadow).
CalibTable{},
// Hyper6 (CalibId 6): not yet calibrated (S-shadow).
CalibTable{},
// Deep6 (CalibId 7): not yet calibrated (S-shadow).
CalibTable{},
// Pko8 (CalibId 8): not yet calibrated (S-shadow).
CalibTable{},
// Pko6 (CalibId 9): not yet calibrated (S-shadow).
CalibTable{},
