#include "pgenheaders.h"
#include "grammar.h"
static arc arcs_0_0[3] = {
	{2, 1},
	{3, 1},
	{4, 2},
};
static arc arcs_0_1[1] = {
	{0, 1},
};
static arc arcs_0_2[1] = {
	{2, 1},
};
static state states_0[3] = {
	{3, arcs_0_0},
	{1, arcs_0_1},
	{1, arcs_0_2},
};
static arc arcs_1_0[3] = {
	{2, 0},
	{6, 0},
	{7, 1},
};
static arc arcs_1_1[1] = {
	{0, 1},
};
static state states_1[2] = {
	{3, arcs_1_0},
	{1, arcs_1_1},
};
static arc arcs_2_0[1] = {
	{9, 1},
};
static arc arcs_2_1[1] = {
	{2, 2},
};
static arc arcs_2_2[1] = {
	{0, 2},
};
static state states_2[3] = {
	{1, arcs_2_0},
	{1, arcs_2_1},
	{1, arcs_2_2},
};
static arc arcs_3_0[1] = {
	{9, 1},
};
static arc arcs_3_1[2] = {
	{2, 1},
	{7, 2},
};
static arc arcs_3_2[1] = {
	{0, 2},
};
static state states_3[3] = {
	{1, arcs_3_0},
	{2, arcs_3_1},
	{1, arcs_3_2},
};
static arc arcs_4_0[1] = {
	{12, 1},
};
static arc arcs_4_1[1] = {
	{13, 2},
};
static arc arcs_4_2[1] = {
	{14, 3},
};
static arc arcs_4_3[1] = {
	{15, 4},
};
static arc arcs_4_4[1] = {
	{16, 5},
};
static arc arcs_4_5[1] = {
	{0, 5},
};
static state states_4[6] = {
	{1, arcs_4_0},
	{1, arcs_4_1},
	{1, arcs_4_2},
	{1, arcs_4_3},
	{1, arcs_4_4},
	{1, arcs_4_5},
};
static arc arcs_5_0[1] = {
	{17, 1},
};
static arc arcs_5_1[2] = {
	{18, 2},
	{19, 3},
};
static arc arcs_5_2[1] = {
	{19, 3},
};
static arc arcs_5_3[1] = {
	{0, 3},
};
static state states_5[4] = {
	{1, arcs_5_0},
	{2, arcs_5_1},
	{1, arcs_5_2},
	{1, arcs_5_3},
};
static arc arcs_6_0[3] = {
	{20, 1},
	{22, 2},
	{23, 2},
};
static arc arcs_6_1[2] = {
	{21, 3},
	{0, 1},
};
static arc arcs_6_2[1] = {
	{13, 4},
};
static arc arcs_6_3[4] = {
	{20, 1},
	{22, 2},
	{23, 2},
	{0, 3},
};
static arc arcs_6_4[1] = {
	{0, 4},
};
static state states_6[5] = {
	{3, arcs_6_0},
	{2, arcs_6_1},
	{1, arcs_6_2},
	{4, arcs_6_3},
	{1, arcs_6_4},
};
static arc arcs_7_0[2] = {
	{13, 1},
	{17, 2},
};
static arc arcs_7_1[1] = {
	{0, 1},
};
static arc arcs_7_2[1] = {
	{24, 3},
};
static arc arcs_7_3[1] = {
	{19, 1},
};
static state states_7[4] = {
	{2, arcs_7_0},
	{1, arcs_7_1},
	{1, arcs_7_2},
	{1, arcs_7_3},
};
static arc arcs_8_0[1] = {
	{20, 1},
};
static arc arcs_8_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_8_2[2] = {
	{20, 1},
	{0, 2},
};
static state states_8[3] = {
	{1, arcs_8_0},
	{2, arcs_8_1},
	{2, arcs_8_2},
};
static arc arcs_9_0[2] = {
	{3, 1},
	{4, 1},
};
static arc arcs_9_1[1] = {
	{0, 1},
};
static state states_9[2] = {
	{2, arcs_9_0},
	{1, arcs_9_1},
};
static arc arcs_10_0[1] = {
	{25, 1},
};
static arc arcs_10_1[2] = {
	{26, 2},
	{2, 3},
};
static arc arcs_10_2[2] = {
	{25, 1},
	{2, 3},
};
static arc arcs_10_3[1] = {
	{0, 3},
};
static state states_10[4] = {
	{1, arcs_10_0},
	{2, arcs_10_1},
	{2, arcs_10_2},
	{1, arcs_10_3},
};
static arc arcs_11_0[7] = {
	{27, 1},
	{28, 1},
	{29, 1},
	{30, 1},
	{31, 1},
	{32, 1},
	{33, 1},
};
static arc arcs_11_1[1] = {
	{0, 1},
};
static state states_11[2] = {
	{7, arcs_11_0},
	{1, arcs_11_1},
};
static arc arcs_12_0[1] = {
	{34, 1},
};
static arc arcs_12_1[2] = {
	{35, 0},
	{0, 1},
};
static state states_12[2] = {
	{1, arcs_12_0},
	{2, arcs_12_1},
};
static arc arcs_13_0[1] = {
	{36, 1},
};
static arc arcs_13_1[2] = {
	{37, 2},
	{0, 1},
};
static arc arcs_13_2[2] = {
	{21, 1},
	{0, 2},
};
static state states_13[3] = {
	{1, arcs_13_0},
	{2, arcs_13_1},
	{2, arcs_13_2},
};
static arc arcs_14_0[1] = {
	{38, 1},
};
static arc arcs_14_1[1] = {
	{34, 2},
};
static arc arcs_14_2[1] = {
	{0, 2},
};
static state states_14[3] = {
	{1, arcs_14_0},
	{1, arcs_14_1},
	{1, arcs_14_2},
};
static arc arcs_15_0[1] = {
	{39, 1},
};
static arc arcs_15_1[1] = {
	{0, 1},
};
static state states_15[2] = {
	{1, arcs_15_0},
	{1, arcs_15_1},
};
static arc arcs_16_0[4] = {
	{40, 1},
	{41, 1},
	{42, 1},
	{43, 1},
};
static arc arcs_16_1[1] = {
	{0, 1},
};
static state states_16[2] = {
	{4, arcs_16_0},
	{1, arcs_16_1},
};
static arc arcs_17_0[1] = {
	{44, 1},
};
static arc arcs_17_1[1] = {
	{0, 1},
};
static state states_17[2] = {
	{1, arcs_17_0},
	{1, arcs_17_1},
};
static arc arcs_18_0[1] = {
	{45, 1},
};
static arc arcs_18_1[1] = {
	{0, 1},
};
static state states_18[2] = {
	{1, arcs_18_0},
	{1, arcs_18_1},
};
static arc arcs_19_0[1] = {
	{46, 1},
};
static arc arcs_19_1[2] = {
	{9, 2},
	{0, 1},
};
static arc arcs_19_2[1] = {
	{0, 2},
};
static state states_19[3] = {
	{1, arcs_19_0},
	{2, arcs_19_1},
	{1, arcs_19_2},
};
static arc arcs_20_0[1] = {
	{47, 1},
};
static arc arcs_20_1[1] = {
	{37, 2},
};
static arc arcs_20_2[2] = {
	{21, 3},
	{0, 2},
};
static arc arcs_20_3[1] = {
	{37, 4},
};
static arc arcs_20_4[1] = {
	{0, 4},
};
static state states_20[5] = {
	{1, arcs_20_0},
	{1, arcs_20_1},
	{2, arcs_20_2},
	{1, arcs_20_3},
	{1, arcs_20_4},
};
static arc arcs_21_0[2] = {
	{48, 1},
	{49, 2},
};
static arc arcs_21_1[1] = {
	{13, 3},
};
static arc arcs_21_2[1] = {
	{13, 4},
};
static arc arcs_21_3[2] = {
	{21, 1},
	{0, 3},
};
static arc arcs_21_4[1] = {
	{48, 5},
};
static arc arcs_21_5[2] = {
	{23, 6},
	{13, 7},
};
static arc arcs_21_6[1] = {
	{0, 6},
};
static arc arcs_21_7[2] = {
	{21, 8},
	{0, 7},
};
static arc arcs_21_8[1] = {
	{13, 7},
};
static state states_21[9] = {
	{2, arcs_21_0},
	{1, arcs_21_1},
	{1, arcs_21_2},
	{2, arcs_21_3},
	{1, arcs_21_4},
	{2, arcs_21_5},
	{1, arcs_21_6},
	{2, arcs_21_7},
	{1, arcs_21_8},
};
static arc arcs_22_0[1] = {
	{50, 1},
};
static arc arcs_22_1[1] = {
	{13, 2},
};
static arc arcs_22_2[2] = {
	{21, 1},
	{0, 2},
};
static state states_22[3] = {
	{1, arcs_22_0},
	{1, arcs_22_1},
	{2, arcs_22_2},
};
static arc arcs_23_0[6] = {
	{51, 1},
	{52, 1},
	{53, 1},
	{54, 1},
	{11, 1},
	{55, 1},
};
static arc arcs_23_1[1] = {
	{0, 1},
};
static state states_23[2] = {
	{6, arcs_23_0},
	{1, arcs_23_1},
};
static arc arcs_24_0[1] = {
	{56, 1},
};
static arc arcs_24_1[1] = {
	{37, 2},
};
static arc arcs_24_2[1] = {
	{15, 3},
};
static arc arcs_24_3[1] = {
	{16, 4},
};
static arc arcs_24_4[3] = {
	{57, 1},
	{58, 5},
	{0, 4},
};
static arc arcs_24_5[1] = {
	{15, 6},
};
static arc arcs_24_6[1] = {
	{16, 7},
};
static arc arcs_24_7[1] = {
	{0, 7},
};
static state states_24[8] = {
	{1, arcs_24_0},
	{1, arcs_24_1},
	{1, arcs_24_2},
	{1, arcs_24_3},
	{3, arcs_24_4},
	{1, arcs_24_5},
	{1, arcs_24_6},
	{1, arcs_24_7},
};
static arc arcs_25_0[1] = {
	{59, 1},
};
static arc arcs_25_1[1] = {
	{37, 2},
};
static arc arcs_25_2[1] = {
	{15, 3},
};
static arc arcs_25_3[1] = {
	{16, 4},
};
static arc arcs_25_4[2] = {
	{58, 5},
	{0, 4},
};
static arc arcs_25_5[1] = {
	{15, 6},
};
static arc arcs_25_6[1] = {
	{16, 7},
};
static arc arcs_25_7[1] = {
	{0, 7},
};
static state states_25[8] = {
	{1, arcs_25_0},
	{1, arcs_25_1},
	{1, arcs_25_2},
	{1, arcs_25_3},
	{2, arcs_25_4},
	{1, arcs_25_5},
	{1, arcs_25_6},
	{1, arcs_25_7},
};
static arc arcs_26_0[1] = {
	{60, 1},
};
static arc arcs_26_1[1] = {
	{34, 2},
};
static arc arcs_26_2[1] = {
	{61, 3},
};
static arc arcs_26_3[1] = {
	{9, 4},
};
static arc arcs_26_4[1] = {
	{15, 5},
};
static arc arcs_26_5[1] = {
	{16, 6},
};
static arc arcs_26_6[2] = {
	{58, 7},
	{0, 6},
};
static arc arcs_26_7[1] = {
	{15, 8},
};
static arc arcs_26_8[1] = {
	{16, 9},
};
static arc arcs_26_9[1] = {
	{0, 9},
};
static state states_26[10] = {
	{1, arcs_26_0},
	{1, arcs_26_1},
	{1, arcs_26_2},
	{1, arcs_26_3},
	{1, arcs_26_4},
	{1, arcs_26_5},
	{2, arcs_26_6},
	{1, arcs_26_7},
	{1, arcs_26_8},
	{1, arcs_26_9},
};
static arc arcs_27_0[1] = {
	{62, 1},
};
static arc arcs_27_1[1] = {
	{15, 2},
};
static arc arcs_27_2[1] = {
	{16, 3},
};
static arc arcs_27_3[3] = {
	{63, 1},
	{64, 4},
	{0, 3},
};
static arc arcs_27_4[1] = {
	{15, 5},
};
static arc arcs_27_5[1] = {
	{16, 6},
};
static arc arcs_27_6[1] = {
	{0, 6},
};
static state states_27[7] = {
	{1, arcs_27_0},
	{1, arcs_27_1},
	{1, arcs_27_2},
	{3, arcs_27_3},
	{1, arcs_27_4},
	{1, arcs_27_5},
	{1, arcs_27_6},
};
static arc arcs_28_0[1] = {
	{65, 1},
};
static arc arcs_28_1[2] = {
	{37, 2},
	{0, 1},
};
static arc arcs_28_2[2] = {
	{21, 3},
	{0, 2},
};
static arc arcs_28_3[1] = {
	{37, 4},
};
static arc arcs_28_4[1] = {
	{0, 4},
};
static state states_28[5] = {
	{1, arcs_28_0},
	{2, arcs_28_1},
	{2, arcs_28_2},
	{1, arcs_28_3},
	{1, arcs_28_4},
};
static arc arcs_29_0[2] = {
	{3, 1},
	{2, 2},
};
static arc arcs_29_1[1] = {
	{0, 1},
};
static arc arcs_29_2[1] = {
	{66, 3},
};
static arc arcs_29_3[1] = {
	{6, 4},
};
static arc arcs_29_4[2] = {
	{6, 4},
	{67, 1},
};
static state states_29[5] = {
	{2, arcs_29_0},
	{1, arcs_29_1},
	{1, arcs_29_2},
	{1, arcs_29_3},
	{2, arcs_29_4},
};
static arc arcs_30_0[1] = {
	{68, 1},
};
static arc arcs_30_1[2] = {
	{69, 0},
	{0, 1},
};
static state states_30[2] = {
	{1, arcs_30_0},
	{2, arcs_30_1},
};
static arc arcs_31_0[1] = {
	{70, 1},
};
static arc arcs_31_1[2] = {
	{71, 0},
	{0, 1},
};
static state states_31[2] = {
	{1, arcs_31_0},
	{2, arcs_31_1},
};
static arc arcs_32_0[2] = {
	{72, 1},
	{73, 2},
};
static arc arcs_32_1[1] = {
	{70, 2},
};
static arc arcs_32_2[1] = {
	{0, 2},
};
static state states_32[3] = {
	{2, arcs_32_0},
	{1, arcs_32_1},
	{1, arcs_32_2},
};
static arc arcs_33_0[1] = {
	{74, 1},
};
static arc arcs_33_1[2] = {
	{75, 0},
	{0, 1},
};
static state states_33[2] = {
	{1, arcs_33_0},
	{2, arcs_33_1},
};
static arc arcs_34_0[10] = {
	{76, 1},
	{77, 1},
	{78, 1},
	{79, 1},
	{80, 1},
	{81, 1},
	{82, 1},
	{61, 1},
	{72, 2},
	{83, 3},
};
static arc arcs_34_1[1] = {
	{0, 1},
};
static arc arcs_34_2[1] = {
	{61, 1},
};
static arc arcs_34_3[2] = {
	{72, 1},
	{0, 3},
};
static state states_34[4] = {
	{10, arcs_34_0},
	{1, arcs_34_1},
	{1, arcs_34_2},
	{2, arcs_34_3},
};
static arc arcs_35_0[1] = {
	{84, 1},
};
static arc arcs_35_1[2] = {
	{85, 0},
	{0, 1},
};
static state states_35[2] = {
	{1, arcs_35_0},
	{2, arcs_35_1},
};
static arc arcs_36_0[1] = {
	{86, 1},
};
static arc arcs_36_1[2] = {
	{87, 0},
	{0, 1},
};
static state states_36[2] = {
	{1, arcs_36_0},
	{2, arcs_36_1},
};
static arc arcs_37_0[1] = {
	{88, 1},
};
static arc arcs_37_1[2] = {
	{89, 0},
	{0, 1},
};
static state states_37[2] = {
	{1, arcs_37_0},
	{2, arcs_37_1},
};
static arc arcs_38_0[1] = {
	{90, 1},
};
static arc arcs_38_1[3] = {
	{91, 0},
	{92, 0},
	{0, 1},
};
static state states_38[2] = {
	{1, arcs_38_0},
	{3, arcs_38_1},
};
static arc arcs_39_0[1] = {
	{93, 1},
};
static arc arcs_39_1[3] = {
	{22, 0},
	{94, 0},
	{0, 1},
};
static state states_39[2] = {
	{1, arcs_39_0},
	{3, arcs_39_1},
};
static arc arcs_40_0[1] = {
	{95, 1},
};
static arc arcs_40_1[4] = {
	{23, 0},
	{96, 0},
	{97, 0},
	{0, 1},
};
static state states_40[2] = {
	{1, arcs_40_0},
	{4, arcs_40_1},
};
static arc arcs_41_0[4] = {
	{22, 1},
	{94, 1},
	{98, 1},
	{99, 2},
};
static arc arcs_41_1[1] = {
	{95, 3},
};
static arc arcs_41_2[2] = {
	{100, 2},
	{0, 2},
};
static arc arcs_41_3[1] = {
	{0, 3},
};
static state states_41[4] = {
	{4, arcs_41_0},
	{1, arcs_41_1},
	{2, arcs_41_2},
	{1, arcs_41_3},
};
static arc arcs_42_0[7] = {
	{17, 1},
	{101, 2},
	{103, 3},
	{106, 4},
	{13, 5},
	{107, 5},
	{108, 5},
};
static arc arcs_42_1[2] = {
	{9, 6},
	{19, 5},
};
static arc arcs_42_2[2] = {
	{9, 7},
	{102, 5},
};
static arc arcs_42_3[2] = {
	{104, 8},
	{105, 5},
};
static arc arcs_42_4[1] = {
	{9, 9},
};
static arc arcs_42_5[1] = {
	{0, 5},
};
static arc arcs_42_6[1] = {
	{19, 5},
};
static arc arcs_42_7[1] = {
	{102, 5},
};
static arc arcs_42_8[1] = {
	{105, 5},
};
static arc arcs_42_9[1] = {
	{106, 5},
};
static state states_42[10] = {
	{7, arcs_42_0},
	{2, arcs_42_1},
	{2, arcs_42_2},
	{2, arcs_42_3},
	{1, arcs_42_4},
	{1, arcs_42_5},
	{1, arcs_42_6},
	{1, arcs_42_7},
	{1, arcs_42_8},
	{1, arcs_42_9},
};
static arc arcs_43_0[3] = {
	{17, 1},
	{101, 2},
	{110, 3},
};
static arc arcs_43_1[2] = {
	{9, 4},
	{19, 5},
};
static arc arcs_43_2[1] = {
	{109, 6},
};
static arc arcs_43_3[1] = {
	{13, 5},
};
static arc arcs_43_4[1] = {
	{19, 5},
};
static arc arcs_43_5[1] = {
	{0, 5},
};
static arc arcs_43_6[1] = {
	{102, 5},
};
static state states_43[7] = {
	{3, arcs_43_0},
	{2, arcs_43_1},
	{1, arcs_43_2},
	{1, arcs_43_3},
	{1, arcs_43_4},
	{1, arcs_43_5},
	{1, arcs_43_6},
};
static arc arcs_44_0[2] = {
	{37, 1},
	{15, 2},
};
static arc arcs_44_1[2] = {
	{15, 2},
	{0, 1},
};
static arc arcs_44_2[2] = {
	{37, 3},
	{0, 2},
};
static arc arcs_44_3[1] = {
	{0, 3},
};
static state states_44[4] = {
	{2, arcs_44_0},
	{2, arcs_44_1},
	{2, arcs_44_2},
	{1, arcs_44_3},
};
static arc arcs_45_0[1] = {
	{74, 1},
};
static arc arcs_45_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_45_2[2] = {
	{74, 1},
	{0, 2},
};
static state states_45[3] = {
	{1, arcs_45_0},
	{2, arcs_45_1},
	{2, arcs_45_2},
};
static arc arcs_46_0[1] = {
	{37, 1},
};
static arc arcs_46_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_46_2[2] = {
	{37, 1},
	{0, 2},
};
static state states_46[3] = {
	{1, arcs_46_0},
	{2, arcs_46_1},
	{2, arcs_46_2},
};
static arc arcs_47_0[1] = {
	{37, 1},
};
static arc arcs_47_1[1] = {
	{15, 2},
};
static arc arcs_47_2[1] = {
	{37, 3},
};
static arc arcs_47_3[2] = {
	{21, 4},
	{0, 3},
};
static arc arcs_47_4[2] = {
	{37, 1},
	{0, 4},
};
static state states_47[5] = {
	{1, arcs_47_0},
	{1, arcs_47_1},
	{1, arcs_47_2},
	{2, arcs_47_3},
	{2, arcs_47_4},
};
static arc arcs_48_0[1] = {
	{111, 1},
};
static arc arcs_48_1[1] = {
	{13, 2},
};
static arc arcs_48_2[2] = {
	{17, 3},
	{15, 4},
};
static arc arcs_48_3[2] = {
	{9, 5},
	{19, 6},
};
static arc arcs_48_4[1] = {
	{16, 7},
};
static arc arcs_48_5[1] = {
	{19, 8},
};
static arc arcs_48_6[2] = {
	{35, 9},
	{15, 4},
};
static arc arcs_48_7[1] = {
	{0, 7},
};
static arc arcs_48_8[1] = {
	{15, 4},
};
static arc arcs_48_9[1] = {
	{112, 8},
};
static state states_48[10] = {
	{1, arcs_48_0},
	{1, arcs_48_1},
	{2, arcs_48_2},
	{2, arcs_48_3},
	{1, arcs_48_4},
	{1, arcs_48_5},
	{2, arcs_48_6},
	{1, arcs_48_7},
	{1, arcs_48_8},
	{1, arcs_48_9},
};
static arc arcs_49_0[1] = {
	{99, 1},
};
static arc arcs_49_1[1] = {
	{113, 2},
};
static arc arcs_49_2[2] = {
	{21, 0},
	{0, 2},
};
static state states_49[3] = {
	{1, arcs_49_0},
	{1, arcs_49_1},
	{2, arcs_49_2},
};
static arc arcs_50_0[1] = {
	{17, 1},
};
static arc arcs_50_1[1] = {
	{19, 2},
};
static arc arcs_50_2[1] = {
	{0, 2},
};
static state states_50[3] = {
	{1, arcs_50_0},
	{1, arcs_50_1},
	{1, arcs_50_2},
};
static dfa dfas[51] = {
	{256, "single_input", 0, 3, states_0,
	 "\004\060\102\000\320\360\007\131\000\000\000\100\244\234\000"},
	{257, "file_input", 0, 2, states_1,
	 "\204\060\102\000\320\360\007\131\000\000\000\100\244\234\000"},
	{258, "expr_input", 0, 3, states_2,
	 "\000\040\102\000\000\000\000\000\000\001\000\100\244\034\000"},
	{259, "eval_input", 0, 3, states_3,
	 "\000\040\102\000\000\000\000\000\000\001\000\100\244\034\000"},
	{260, "funcdef", 0, 6, states_4,
	 "\000\020\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{261, "parameters", 0, 4, states_5,
	 "\000\000\002\000\000\000\000\000\000\000\000\000\000\000\000"},
	{262, "varargslist", 0, 5, states_6,
	 "\000\040\302\000\000\000\000\000\000\000\000\000\000\000\000"},
	{263, "fpdef", 0, 4, states_7,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\000\000\000"},
	{264, "fplist", 0, 3, states_8,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\000\000\000"},
	{265, "stmt", 0, 2, states_9,
	 "\000\060\102\000\320\360\007\131\000\000\000\100\244\234\000"},
	{266, "simple_stmt", 0, 4, states_10,
	 "\000\040\102\000\320\360\007\000\000\000\000\100\244\034\000"},
	{267, "small_stmt", 0, 2, states_11,
	 "\000\040\102\000\320\360\007\000\000\000\000\100\244\034\000"},
	{268, "expr_stmt", 0, 2, states_12,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{269, "print_stmt", 0, 3, states_13,
	 "\000\000\000\000\020\000\000\000\000\000\000\000\000\000\000"},
	{270, "del_stmt", 0, 3, states_14,
	 "\000\000\000\000\100\000\000\000\000\000\000\000\000\000\000"},
	{271, "pass_stmt", 0, 2, states_15,
	 "\000\000\000\000\200\000\000\000\000\000\000\000\000\000\000"},
	{272, "flow_stmt", 0, 2, states_16,
	 "\000\000\000\000\000\360\000\000\000\000\000\000\000\000\000"},
	{273, "break_stmt", 0, 2, states_17,
	 "\000\000\000\000\000\020\000\000\000\000\000\000\000\000\000"},
	{274, "continue_stmt", 0, 2, states_18,
	 "\000\000\000\000\000\040\000\000\000\000\000\000\000\000\000"},
	{275, "return_stmt", 0, 3, states_19,
	 "\000\000\000\000\000\100\000\000\000\000\000\000\000\000\000"},
	{276, "raise_stmt", 0, 5, states_20,
	 "\000\000\000\000\000\200\000\000\000\000\000\000\000\000\000"},
	{277, "import_stmt", 0, 9, states_21,
	 "\000\000\000\000\000\000\003\000\000\000\000\000\000\000\000"},
	{278, "global_stmt", 0, 3, states_22,
	 "\000\000\000\000\000\000\004\000\000\000\000\000\000\000\000"},
	{279, "compound_stmt", 0, 2, states_23,
	 "\000\020\000\000\000\000\000\131\000\000\000\000\000\200\000"},
	{280, "if_stmt", 0, 8, states_24,
	 "\000\000\000\000\000\000\000\001\000\000\000\000\000\000\000"},
	{281, "while_stmt", 0, 8, states_25,
	 "\000\000\000\000\000\000\000\010\000\000\000\000\000\000\000"},
	{282, "for_stmt", 0, 10, states_26,
	 "\000\000\000\000\000\000\000\020\000\000\000\000\000\000\000"},
	{283, "try_stmt", 0, 7, states_27,
	 "\000\000\000\000\000\000\000\100\000\000\000\000\000\000\000"},
	{284, "except_clause", 0, 5, states_28,
	 "\000\000\000\000\000\000\000\000\002\000\000\000\000\000\000"},
	{285, "suite", 0, 5, states_29,
	 "\004\040\102\000\320\360\007\000\000\000\000\100\244\034\000"},
	{286, "test", 0, 2, states_30,
	 "\000\040\102\000\000\000\000\000\000\001\000\100\244\034\000"},
	{287, "and_test", 0, 2, states_31,
	 "\000\040\102\000\000\000\000\000\000\001\000\100\244\034\000"},
	{288, "not_test", 0, 3, states_32,
	 "\000\040\102\000\000\000\000\000\000\001\000\100\244\034\000"},
	{289, "comparison", 0, 2, states_33,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{290, "comp_op", 0, 4, states_34,
	 "\000\000\000\000\000\000\000\040\000\361\017\000\000\000\000"},
	{291, "expr", 0, 2, states_35,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{292, "xor_expr", 0, 2, states_36,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{293, "and_expr", 0, 2, states_37,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{294, "shift_expr", 0, 2, states_38,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{295, "arith_expr", 0, 2, states_39,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{296, "term", 0, 2, states_40,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{297, "factor", 0, 4, states_41,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{298, "atom", 0, 10, states_42,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\240\034\000"},
	{299, "trailer", 0, 7, states_43,
	 "\000\000\002\000\000\000\000\000\000\000\000\000\040\100\000"},
	{300, "subscript", 0, 4, states_44,
	 "\000\240\102\000\000\000\000\000\000\001\000\100\244\034\000"},
	{301, "exprlist", 0, 3, states_45,
	 "\000\040\102\000\000\000\000\000\000\000\000\100\244\034\000"},
	{302, "testlist", 0, 3, states_46,
	 "\000\040\102\000\000\000\000\000\000\001\000\100\244\034\000"},
	{303, "dictmaker", 0, 5, states_47,
	 "\000\040\102\000\000\000\000\000\000\001\000\100\244\034\000"},
	{304, "classdef", 0, 10, states_48,
	 "\000\000\000\000\000\000\000\000\000\000\000\000\000\200\000"},
	{305, "baselist", 0, 3, states_49,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\240\034\000"},
	{306, "arguments", 0, 3, states_50,
	 "\000\000\002\000\000\000\000\000\000\000\000\000\000\000\000"},
};
static label labels[114] = {
	{0, "EMPTY"},
	{256, 0},
	{4, 0},
	{266, 0},
	{279, 0},
	{257, 0},
	{265, 0},
	{0, 0},
	{258, 0},
	{302, 0},
	{259, 0},
	{260, 0},
	{1, "def"},
	{1, 0},
	{261, 0},
	{11, 0},
	{285, 0},
	{7, 0},
	{262, 0},
	{8, 0},
	{263, 0},
	{12, 0},
	{14, 0},
	{16, 0},
	{264, 0},
	{267, 0},
	{13, 0},
	{268, 0},
	{269, 0},
	{270, 0},
	{271, 0},
	{272, 0},
	{277, 0},
	{278, 0},
	{301, 0},
	{22, 0},
	{1, "print"},
	{286, 0},
	{1, "del"},
	{1, "pass"},
	{273, 0},
	{274, 0},
	{275, 0},
	{276, 0},
	{1, "break"},
	{1, "continue"},
	{1, "return"},
	{1, "raise"},
	{1, "import"},
	{1, "from"},
	{1, "global"},
	{280, 0},
	{281, 0},
	{282, 0},
	{283, 0},
	{304, 0},
	{1, "if"},
	{1, "elif"},
	{1, "else"},
	{1, "while"},
	{1, "for"},
	{1, "in"},
	{1, "try"},
	{284, 0},
	{1, "finally"},
	{1, "except"},
	{5, 0},
	{6, 0},
	{287, 0},
	{1, "or"},
	{288, 0},
	{1, "and"},
	{1, "not"},
	{289, 0},
	{291, 0},
	{290, 0},
	{20, 0},
	{21, 0},
	{28, 0},
	{31, 0},
	{30, 0},
	{29, 0},
	{29, 0},
	{1, "is"},
	{292, 0},
	{18, 0},
	{293, 0},
	{33, 0},
	{294, 0},
	{19, 0},
	{295, 0},
	{34, 0},
	{35, 0},
	{296, 0},
	{15, 0},
	{297, 0},
	{17, 0},
	{24, 0},
	{32, 0},
	{298, 0},
	{299, 0},
	{9, 0},
	{10, 0},
	{26, 0},
	{303, 0},
	{27, 0},
	{25, 0},
	{2, 0},
	{3, 0},
	{300, 0},
	{23, 0},
	{1, "class"},
	{305, 0},
	{306, 0},
};
grammar gram = {
	51,
	dfas,
	{114, labels},
	256
};
