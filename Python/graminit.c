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
static arc arcs_2_1[2] = {
	{2, 1},
	{7, 2},
};
static arc arcs_2_2[1] = {
	{0, 2},
};
static state states_2[3] = {
	{1, arcs_2_0},
	{2, arcs_2_1},
	{1, arcs_2_2},
};
static arc arcs_3_0[1] = {
	{11, 1},
};
static arc arcs_3_1[1] = {
	{12, 2},
};
static arc arcs_3_2[1] = {
	{13, 3},
};
static arc arcs_3_3[1] = {
	{14, 4},
};
static arc arcs_3_4[1] = {
	{15, 5},
};
static arc arcs_3_5[1] = {
	{0, 5},
};
static state states_3[6] = {
	{1, arcs_3_0},
	{1, arcs_3_1},
	{1, arcs_3_2},
	{1, arcs_3_3},
	{1, arcs_3_4},
	{1, arcs_3_5},
};
static arc arcs_4_0[1] = {
	{16, 1},
};
static arc arcs_4_1[2] = {
	{17, 2},
	{18, 3},
};
static arc arcs_4_2[1] = {
	{18, 3},
};
static arc arcs_4_3[1] = {
	{0, 3},
};
static state states_4[4] = {
	{1, arcs_4_0},
	{2, arcs_4_1},
	{1, arcs_4_2},
	{1, arcs_4_3},
};
static arc arcs_5_0[3] = {
	{19, 1},
	{23, 2},
	{24, 3},
};
static arc arcs_5_1[3] = {
	{20, 4},
	{22, 5},
	{0, 1},
};
static arc arcs_5_2[2] = {
	{12, 6},
	{23, 3},
};
static arc arcs_5_3[1] = {
	{12, 7},
};
static arc arcs_5_4[1] = {
	{21, 8},
};
static arc arcs_5_5[4] = {
	{19, 1},
	{23, 2},
	{24, 3},
	{0, 5},
};
static arc arcs_5_6[2] = {
	{22, 9},
	{0, 6},
};
static arc arcs_5_7[1] = {
	{0, 7},
};
static arc arcs_5_8[2] = {
	{22, 5},
	{0, 8},
};
static arc arcs_5_9[2] = {
	{24, 3},
	{23, 10},
};
static arc arcs_5_10[1] = {
	{23, 3},
};
static state states_5[11] = {
	{3, arcs_5_0},
	{3, arcs_5_1},
	{2, arcs_5_2},
	{1, arcs_5_3},
	{1, arcs_5_4},
	{4, arcs_5_5},
	{2, arcs_5_6},
	{1, arcs_5_7},
	{2, arcs_5_8},
	{2, arcs_5_9},
	{1, arcs_5_10},
};
static arc arcs_6_0[2] = {
	{12, 1},
	{16, 2},
};
static arc arcs_6_1[1] = {
	{0, 1},
};
static arc arcs_6_2[1] = {
	{25, 3},
};
static arc arcs_6_3[1] = {
	{18, 1},
};
static state states_6[4] = {
	{2, arcs_6_0},
	{1, arcs_6_1},
	{1, arcs_6_2},
	{1, arcs_6_3},
};
static arc arcs_7_0[1] = {
	{19, 1},
};
static arc arcs_7_1[2] = {
	{22, 2},
	{0, 1},
};
static arc arcs_7_2[2] = {
	{19, 1},
	{0, 2},
};
static state states_7[3] = {
	{1, arcs_7_0},
	{2, arcs_7_1},
	{2, arcs_7_2},
};
static arc arcs_8_0[2] = {
	{3, 1},
	{4, 1},
};
static arc arcs_8_1[1] = {
	{0, 1},
};
static state states_8[2] = {
	{2, arcs_8_0},
	{1, arcs_8_1},
};
static arc arcs_9_0[1] = {
	{26, 1},
};
static arc arcs_9_1[2] = {
	{27, 2},
	{2, 3},
};
static arc arcs_9_2[2] = {
	{26, 1},
	{2, 3},
};
static arc arcs_9_3[1] = {
	{0, 3},
};
static state states_9[4] = {
	{1, arcs_9_0},
	{2, arcs_9_1},
	{2, arcs_9_2},
	{1, arcs_9_3},
};
static arc arcs_10_0[9] = {
	{28, 1},
	{29, 1},
	{30, 1},
	{31, 1},
	{32, 1},
	{33, 1},
	{34, 1},
	{35, 1},
	{36, 1},
};
static arc arcs_10_1[1] = {
	{0, 1},
};
static state states_10[2] = {
	{9, arcs_10_0},
	{1, arcs_10_1},
};
static arc arcs_11_0[1] = {
	{9, 1},
};
static arc arcs_11_1[2] = {
	{20, 0},
	{0, 1},
};
static state states_11[2] = {
	{1, arcs_11_0},
	{2, arcs_11_1},
};
static arc arcs_12_0[1] = {
	{37, 1},
};
static arc arcs_12_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_12_2[2] = {
	{22, 1},
	{0, 2},
};
static state states_12[3] = {
	{1, arcs_12_0},
	{2, arcs_12_1},
	{2, arcs_12_2},
};
static arc arcs_13_0[1] = {
	{38, 1},
};
static arc arcs_13_1[1] = {
	{39, 2},
};
static arc arcs_13_2[1] = {
	{0, 2},
};
static state states_13[3] = {
	{1, arcs_13_0},
	{1, arcs_13_1},
	{1, arcs_13_2},
};
static arc arcs_14_0[1] = {
	{40, 1},
};
static arc arcs_14_1[1] = {
	{0, 1},
};
static state states_14[2] = {
	{1, arcs_14_0},
	{1, arcs_14_1},
};
static arc arcs_15_0[4] = {
	{41, 1},
	{42, 1},
	{43, 1},
	{44, 1},
};
static arc arcs_15_1[1] = {
	{0, 1},
};
static state states_15[2] = {
	{4, arcs_15_0},
	{1, arcs_15_1},
};
static arc arcs_16_0[1] = {
	{45, 1},
};
static arc arcs_16_1[1] = {
	{0, 1},
};
static state states_16[2] = {
	{1, arcs_16_0},
	{1, arcs_16_1},
};
static arc arcs_17_0[1] = {
	{46, 1},
};
static arc arcs_17_1[1] = {
	{0, 1},
};
static state states_17[2] = {
	{1, arcs_17_0},
	{1, arcs_17_1},
};
static arc arcs_18_0[1] = {
	{47, 1},
};
static arc arcs_18_1[2] = {
	{9, 2},
	{0, 1},
};
static arc arcs_18_2[1] = {
	{0, 2},
};
static state states_18[3] = {
	{1, arcs_18_0},
	{2, arcs_18_1},
	{1, arcs_18_2},
};
static arc arcs_19_0[1] = {
	{48, 1},
};
static arc arcs_19_1[1] = {
	{21, 2},
};
static arc arcs_19_2[2] = {
	{22, 3},
	{0, 2},
};
static arc arcs_19_3[1] = {
	{21, 4},
};
static arc arcs_19_4[2] = {
	{22, 5},
	{0, 4},
};
static arc arcs_19_5[1] = {
	{21, 6},
};
static arc arcs_19_6[1] = {
	{0, 6},
};
static state states_19[7] = {
	{1, arcs_19_0},
	{1, arcs_19_1},
	{2, arcs_19_2},
	{1, arcs_19_3},
	{2, arcs_19_4},
	{1, arcs_19_5},
	{1, arcs_19_6},
};
static arc arcs_20_0[2] = {
	{49, 1},
	{51, 2},
};
static arc arcs_20_1[1] = {
	{50, 3},
};
static arc arcs_20_2[1] = {
	{50, 4},
};
static arc arcs_20_3[2] = {
	{22, 1},
	{0, 3},
};
static arc arcs_20_4[1] = {
	{49, 5},
};
static arc arcs_20_5[2] = {
	{23, 6},
	{12, 7},
};
static arc arcs_20_6[1] = {
	{0, 6},
};
static arc arcs_20_7[2] = {
	{22, 8},
	{0, 7},
};
static arc arcs_20_8[1] = {
	{12, 7},
};
static state states_20[9] = {
	{2, arcs_20_0},
	{1, arcs_20_1},
	{1, arcs_20_2},
	{2, arcs_20_3},
	{1, arcs_20_4},
	{2, arcs_20_5},
	{1, arcs_20_6},
	{2, arcs_20_7},
	{1, arcs_20_8},
};
static arc arcs_21_0[1] = {
	{12, 1},
};
static arc arcs_21_1[2] = {
	{52, 0},
	{0, 1},
};
static state states_21[2] = {
	{1, arcs_21_0},
	{2, arcs_21_1},
};
static arc arcs_22_0[1] = {
	{53, 1},
};
static arc arcs_22_1[1] = {
	{12, 2},
};
static arc arcs_22_2[2] = {
	{22, 1},
	{0, 2},
};
static state states_22[3] = {
	{1, arcs_22_0},
	{1, arcs_22_1},
	{2, arcs_22_2},
};
static arc arcs_23_0[1] = {
	{54, 1},
};
static arc arcs_23_1[2] = {
	{23, 2},
	{12, 3},
};
static arc arcs_23_2[1] = {
	{14, 4},
};
static arc arcs_23_3[2] = {
	{22, 5},
	{14, 4},
};
static arc arcs_23_4[1] = {
	{55, 6},
};
static arc arcs_23_5[1] = {
	{12, 3},
};
static arc arcs_23_6[2] = {
	{22, 4},
	{0, 6},
};
static state states_23[7] = {
	{1, arcs_23_0},
	{2, arcs_23_1},
	{1, arcs_23_2},
	{2, arcs_23_3},
	{1, arcs_23_4},
	{1, arcs_23_5},
	{2, arcs_23_6},
};
static arc arcs_24_0[1] = {
	{12, 1},
};
static arc arcs_24_1[2] = {
	{12, 1},
	{0, 1},
};
static state states_24[2] = {
	{1, arcs_24_0},
	{2, arcs_24_1},
};
static arc arcs_25_0[1] = {
	{56, 1},
};
static arc arcs_25_1[1] = {
	{57, 2},
};
static arc arcs_25_2[2] = {
	{58, 3},
	{0, 2},
};
static arc arcs_25_3[1] = {
	{21, 4},
};
static arc arcs_25_4[2] = {
	{22, 5},
	{0, 4},
};
static arc arcs_25_5[1] = {
	{21, 6},
};
static arc arcs_25_6[1] = {
	{0, 6},
};
static state states_25[7] = {
	{1, arcs_25_0},
	{1, arcs_25_1},
	{2, arcs_25_2},
	{1, arcs_25_3},
	{2, arcs_25_4},
	{1, arcs_25_5},
	{1, arcs_25_6},
};
static arc arcs_26_0[6] = {
	{59, 1},
	{60, 1},
	{61, 1},
	{62, 1},
	{10, 1},
	{63, 1},
};
static arc arcs_26_1[1] = {
	{0, 1},
};
static state states_26[2] = {
	{6, arcs_26_0},
	{1, arcs_26_1},
};
static arc arcs_27_0[1] = {
	{64, 1},
};
static arc arcs_27_1[1] = {
	{21, 2},
};
static arc arcs_27_2[1] = {
	{14, 3},
};
static arc arcs_27_3[1] = {
	{15, 4},
};
static arc arcs_27_4[3] = {
	{65, 1},
	{66, 5},
	{0, 4},
};
static arc arcs_27_5[1] = {
	{14, 6},
};
static arc arcs_27_6[1] = {
	{15, 7},
};
static arc arcs_27_7[1] = {
	{0, 7},
};
static state states_27[8] = {
	{1, arcs_27_0},
	{1, arcs_27_1},
	{1, arcs_27_2},
	{1, arcs_27_3},
	{3, arcs_27_4},
	{1, arcs_27_5},
	{1, arcs_27_6},
	{1, arcs_27_7},
};
static arc arcs_28_0[1] = {
	{67, 1},
};
static arc arcs_28_1[1] = {
	{21, 2},
};
static arc arcs_28_2[1] = {
	{14, 3},
};
static arc arcs_28_3[1] = {
	{15, 4},
};
static arc arcs_28_4[2] = {
	{66, 5},
	{0, 4},
};
static arc arcs_28_5[1] = {
	{14, 6},
};
static arc arcs_28_6[1] = {
	{15, 7},
};
static arc arcs_28_7[1] = {
	{0, 7},
};
static state states_28[8] = {
	{1, arcs_28_0},
	{1, arcs_28_1},
	{1, arcs_28_2},
	{1, arcs_28_3},
	{2, arcs_28_4},
	{1, arcs_28_5},
	{1, arcs_28_6},
	{1, arcs_28_7},
};
static arc arcs_29_0[1] = {
	{68, 1},
};
static arc arcs_29_1[1] = {
	{39, 2},
};
static arc arcs_29_2[1] = {
	{58, 3},
};
static arc arcs_29_3[1] = {
	{9, 4},
};
static arc arcs_29_4[1] = {
	{14, 5},
};
static arc arcs_29_5[1] = {
	{15, 6},
};
static arc arcs_29_6[2] = {
	{66, 7},
	{0, 6},
};
static arc arcs_29_7[1] = {
	{14, 8},
};
static arc arcs_29_8[1] = {
	{15, 9},
};
static arc arcs_29_9[1] = {
	{0, 9},
};
static state states_29[10] = {
	{1, arcs_29_0},
	{1, arcs_29_1},
	{1, arcs_29_2},
	{1, arcs_29_3},
	{1, arcs_29_4},
	{1, arcs_29_5},
	{2, arcs_29_6},
	{1, arcs_29_7},
	{1, arcs_29_8},
	{1, arcs_29_9},
};
static arc arcs_30_0[1] = {
	{69, 1},
};
static arc arcs_30_1[1] = {
	{14, 2},
};
static arc arcs_30_2[1] = {
	{15, 3},
};
static arc arcs_30_3[2] = {
	{70, 4},
	{71, 5},
};
static arc arcs_30_4[1] = {
	{14, 6},
};
static arc arcs_30_5[1] = {
	{14, 7},
};
static arc arcs_30_6[1] = {
	{15, 8},
};
static arc arcs_30_7[1] = {
	{15, 9},
};
static arc arcs_30_8[3] = {
	{70, 4},
	{66, 5},
	{0, 8},
};
static arc arcs_30_9[1] = {
	{0, 9},
};
static state states_30[10] = {
	{1, arcs_30_0},
	{1, arcs_30_1},
	{1, arcs_30_2},
	{2, arcs_30_3},
	{1, arcs_30_4},
	{1, arcs_30_5},
	{1, arcs_30_6},
	{1, arcs_30_7},
	{3, arcs_30_8},
	{1, arcs_30_9},
};
static arc arcs_31_0[1] = {
	{72, 1},
};
static arc arcs_31_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_31_2[2] = {
	{22, 3},
	{0, 2},
};
static arc arcs_31_3[1] = {
	{21, 4},
};
static arc arcs_31_4[1] = {
	{0, 4},
};
static state states_31[5] = {
	{1, arcs_31_0},
	{2, arcs_31_1},
	{2, arcs_31_2},
	{1, arcs_31_3},
	{1, arcs_31_4},
};
static arc arcs_32_0[2] = {
	{3, 1},
	{2, 2},
};
static arc arcs_32_1[1] = {
	{0, 1},
};
static arc arcs_32_2[1] = {
	{73, 3},
};
static arc arcs_32_3[1] = {
	{6, 4},
};
static arc arcs_32_4[2] = {
	{6, 4},
	{74, 1},
};
static state states_32[5] = {
	{2, arcs_32_0},
	{1, arcs_32_1},
	{1, arcs_32_2},
	{1, arcs_32_3},
	{2, arcs_32_4},
};
static arc arcs_33_0[2] = {
	{75, 1},
	{77, 2},
};
static arc arcs_33_1[2] = {
	{76, 3},
	{0, 1},
};
static arc arcs_33_2[1] = {
	{0, 2},
};
static arc arcs_33_3[1] = {
	{75, 1},
};
static state states_33[4] = {
	{2, arcs_33_0},
	{2, arcs_33_1},
	{1, arcs_33_2},
	{1, arcs_33_3},
};
static arc arcs_34_0[1] = {
	{78, 1},
};
static arc arcs_34_1[2] = {
	{79, 0},
	{0, 1},
};
static state states_34[2] = {
	{1, arcs_34_0},
	{2, arcs_34_1},
};
static arc arcs_35_0[2] = {
	{80, 1},
	{81, 2},
};
static arc arcs_35_1[1] = {
	{78, 2},
};
static arc arcs_35_2[1] = {
	{0, 2},
};
static state states_35[3] = {
	{2, arcs_35_0},
	{1, arcs_35_1},
	{1, arcs_35_2},
};
static arc arcs_36_0[1] = {
	{57, 1},
};
static arc arcs_36_1[2] = {
	{82, 0},
	{0, 1},
};
static state states_36[2] = {
	{1, arcs_36_0},
	{2, arcs_36_1},
};
static arc arcs_37_0[10] = {
	{83, 1},
	{84, 1},
	{85, 1},
	{86, 1},
	{87, 1},
	{88, 1},
	{89, 1},
	{58, 1},
	{80, 2},
	{90, 3},
};
static arc arcs_37_1[1] = {
	{0, 1},
};
static arc arcs_37_2[1] = {
	{58, 1},
};
static arc arcs_37_3[2] = {
	{80, 1},
	{0, 3},
};
static state states_37[4] = {
	{10, arcs_37_0},
	{1, arcs_37_1},
	{1, arcs_37_2},
	{2, arcs_37_3},
};
static arc arcs_38_0[1] = {
	{91, 1},
};
static arc arcs_38_1[2] = {
	{92, 0},
	{0, 1},
};
static state states_38[2] = {
	{1, arcs_38_0},
	{2, arcs_38_1},
};
static arc arcs_39_0[1] = {
	{93, 1},
};
static arc arcs_39_1[2] = {
	{94, 0},
	{0, 1},
};
static state states_39[2] = {
	{1, arcs_39_0},
	{2, arcs_39_1},
};
static arc arcs_40_0[1] = {
	{95, 1},
};
static arc arcs_40_1[2] = {
	{96, 0},
	{0, 1},
};
static state states_40[2] = {
	{1, arcs_40_0},
	{2, arcs_40_1},
};
static arc arcs_41_0[1] = {
	{97, 1},
};
static arc arcs_41_1[3] = {
	{98, 0},
	{99, 0},
	{0, 1},
};
static state states_41[2] = {
	{1, arcs_41_0},
	{3, arcs_41_1},
};
static arc arcs_42_0[1] = {
	{100, 1},
};
static arc arcs_42_1[3] = {
	{101, 0},
	{102, 0},
	{0, 1},
};
static state states_42[2] = {
	{1, arcs_42_0},
	{3, arcs_42_1},
};
static arc arcs_43_0[1] = {
	{103, 1},
};
static arc arcs_43_1[4] = {
	{23, 0},
	{104, 0},
	{105, 0},
	{0, 1},
};
static state states_43[2] = {
	{1, arcs_43_0},
	{4, arcs_43_1},
};
static arc arcs_44_0[4] = {
	{101, 1},
	{102, 1},
	{106, 1},
	{107, 2},
};
static arc arcs_44_1[1] = {
	{103, 2},
};
static arc arcs_44_2[1] = {
	{0, 2},
};
static state states_44[3] = {
	{4, arcs_44_0},
	{1, arcs_44_1},
	{1, arcs_44_2},
};
static arc arcs_45_0[1] = {
	{108, 1},
};
static arc arcs_45_1[3] = {
	{109, 1},
	{24, 2},
	{0, 1},
};
static arc arcs_45_2[1] = {
	{103, 3},
};
static arc arcs_45_3[2] = {
	{24, 2},
	{0, 3},
};
static state states_45[4] = {
	{1, arcs_45_0},
	{3, arcs_45_1},
	{1, arcs_45_2},
	{2, arcs_45_3},
};
static arc arcs_46_0[7] = {
	{16, 1},
	{110, 2},
	{112, 3},
	{115, 4},
	{12, 5},
	{116, 5},
	{117, 6},
};
static arc arcs_46_1[2] = {
	{9, 7},
	{18, 5},
};
static arc arcs_46_2[2] = {
	{9, 8},
	{111, 5},
};
static arc arcs_46_3[2] = {
	{113, 9},
	{114, 5},
};
static arc arcs_46_4[1] = {
	{9, 10},
};
static arc arcs_46_5[1] = {
	{0, 5},
};
static arc arcs_46_6[2] = {
	{117, 6},
	{0, 6},
};
static arc arcs_46_7[1] = {
	{18, 5},
};
static arc arcs_46_8[1] = {
	{111, 5},
};
static arc arcs_46_9[1] = {
	{114, 5},
};
static arc arcs_46_10[1] = {
	{115, 5},
};
static state states_46[11] = {
	{7, arcs_46_0},
	{2, arcs_46_1},
	{2, arcs_46_2},
	{2, arcs_46_3},
	{1, arcs_46_4},
	{1, arcs_46_5},
	{2, arcs_46_6},
	{1, arcs_46_7},
	{1, arcs_46_8},
	{1, arcs_46_9},
	{1, arcs_46_10},
};
static arc arcs_47_0[1] = {
	{118, 1},
};
static arc arcs_47_1[2] = {
	{17, 2},
	{14, 3},
};
static arc arcs_47_2[1] = {
	{14, 3},
};
static arc arcs_47_3[1] = {
	{21, 4},
};
static arc arcs_47_4[1] = {
	{0, 4},
};
static state states_47[5] = {
	{1, arcs_47_0},
	{2, arcs_47_1},
	{1, arcs_47_2},
	{1, arcs_47_3},
	{1, arcs_47_4},
};
static arc arcs_48_0[3] = {
	{16, 1},
	{110, 2},
	{52, 3},
};
static arc arcs_48_1[2] = {
	{119, 4},
	{18, 5},
};
static arc arcs_48_2[1] = {
	{120, 6},
};
static arc arcs_48_3[1] = {
	{12, 5},
};
static arc arcs_48_4[1] = {
	{18, 5},
};
static arc arcs_48_5[1] = {
	{0, 5},
};
static arc arcs_48_6[1] = {
	{111, 5},
};
static state states_48[7] = {
	{3, arcs_48_0},
	{2, arcs_48_1},
	{1, arcs_48_2},
	{1, arcs_48_3},
	{1, arcs_48_4},
	{1, arcs_48_5},
	{1, arcs_48_6},
};
static arc arcs_49_0[1] = {
	{121, 1},
};
static arc arcs_49_1[2] = {
	{22, 2},
	{0, 1},
};
static arc arcs_49_2[2] = {
	{121, 1},
	{0, 2},
};
static state states_49[3] = {
	{1, arcs_49_0},
	{2, arcs_49_1},
	{2, arcs_49_2},
};
static arc arcs_50_0[3] = {
	{52, 1},
	{21, 2},
	{14, 3},
};
static arc arcs_50_1[1] = {
	{52, 4},
};
static arc arcs_50_2[2] = {
	{14, 3},
	{0, 2},
};
static arc arcs_50_3[3] = {
	{21, 5},
	{122, 6},
	{0, 3},
};
static arc arcs_50_4[1] = {
	{52, 6},
};
static arc arcs_50_5[2] = {
	{122, 6},
	{0, 5},
};
static arc arcs_50_6[1] = {
	{0, 6},
};
static state states_50[7] = {
	{3, arcs_50_0},
	{1, arcs_50_1},
	{2, arcs_50_2},
	{3, arcs_50_3},
	{1, arcs_50_4},
	{2, arcs_50_5},
	{1, arcs_50_6},
};
static arc arcs_51_0[1] = {
	{14, 1},
};
static arc arcs_51_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_51_2[1] = {
	{0, 2},
};
static state states_51[3] = {
	{1, arcs_51_0},
	{2, arcs_51_1},
	{1, arcs_51_2},
};
static arc arcs_52_0[1] = {
	{57, 1},
};
static arc arcs_52_1[2] = {
	{22, 2},
	{0, 1},
};
static arc arcs_52_2[2] = {
	{57, 1},
	{0, 2},
};
static state states_52[3] = {
	{1, arcs_52_0},
	{2, arcs_52_1},
	{2, arcs_52_2},
};
static arc arcs_53_0[1] = {
	{21, 1},
};
static arc arcs_53_1[2] = {
	{22, 2},
	{0, 1},
};
static arc arcs_53_2[2] = {
	{21, 1},
	{0, 2},
};
static state states_53[3] = {
	{1, arcs_53_0},
	{2, arcs_53_1},
	{2, arcs_53_2},
};
static arc arcs_54_0[1] = {
	{21, 1},
};
static arc arcs_54_1[1] = {
	{14, 2},
};
static arc arcs_54_2[1] = {
	{21, 3},
};
static arc arcs_54_3[2] = {
	{22, 4},
	{0, 3},
};
static arc arcs_54_4[2] = {
	{21, 1},
	{0, 4},
};
static state states_54[5] = {
	{1, arcs_54_0},
	{1, arcs_54_1},
	{1, arcs_54_2},
	{2, arcs_54_3},
	{2, arcs_54_4},
};
static arc arcs_55_0[1] = {
	{123, 1},
};
static arc arcs_55_1[1] = {
	{12, 2},
};
static arc arcs_55_2[2] = {
	{16, 3},
	{14, 4},
};
static arc arcs_55_3[1] = {
	{9, 5},
};
static arc arcs_55_4[1] = {
	{15, 6},
};
static arc arcs_55_5[1] = {
	{18, 7},
};
static arc arcs_55_6[1] = {
	{0, 6},
};
static arc arcs_55_7[1] = {
	{14, 4},
};
static state states_55[8] = {
	{1, arcs_55_0},
	{1, arcs_55_1},
	{2, arcs_55_2},
	{1, arcs_55_3},
	{1, arcs_55_4},
	{1, arcs_55_5},
	{1, arcs_55_6},
	{1, arcs_55_7},
};
static arc arcs_56_0[1] = {
	{124, 1},
};
static arc arcs_56_1[2] = {
	{22, 2},
	{0, 1},
};
static arc arcs_56_2[2] = {
	{124, 1},
	{0, 2},
};
static state states_56[3] = {
	{1, arcs_56_0},
	{2, arcs_56_1},
	{2, arcs_56_2},
};
static arc arcs_57_0[1] = {
	{21, 1},
};
static arc arcs_57_1[2] = {
	{20, 2},
	{0, 1},
};
static arc arcs_57_2[1] = {
	{21, 3},
};
static arc arcs_57_3[1] = {
	{0, 3},
};
static state states_57[4] = {
	{1, arcs_57_0},
	{2, arcs_57_1},
	{1, arcs_57_2},
	{1, arcs_57_3},
};
static dfa dfas[58] = {
	{256, "single_input", 0, 3, states_0,
	 "\004\030\001\000\140\341\153\001\071\000\001\000\140\104\171\010"},
	{257, "file_input", 0, 2, states_1,
	 "\204\030\001\000\140\341\153\001\071\000\001\000\140\104\171\010"},
	{258, "eval_input", 0, 3, states_2,
	 "\000\020\001\000\000\000\000\000\000\000\001\000\140\104\171\000"},
	{259, "funcdef", 0, 6, states_3,
	 "\000\010\000\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{260, "parameters", 0, 4, states_4,
	 "\000\000\001\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{261, "varargslist", 0, 11, states_5,
	 "\000\020\201\001\000\000\000\000\000\000\000\000\000\000\000\000"},
	{262, "fpdef", 0, 4, states_6,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{263, "fplist", 0, 3, states_7,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{264, "stmt", 0, 2, states_8,
	 "\000\030\001\000\140\341\153\001\071\000\001\000\140\104\171\010"},
	{265, "simple_stmt", 0, 4, states_9,
	 "\000\020\001\000\140\341\153\001\000\000\001\000\140\104\171\000"},
	{266, "small_stmt", 0, 2, states_10,
	 "\000\020\001\000\140\341\153\001\000\000\001\000\140\104\171\000"},
	{267, "expr_stmt", 0, 2, states_11,
	 "\000\020\001\000\000\000\000\000\000\000\001\000\140\104\171\000"},
	{268, "print_stmt", 0, 3, states_12,
	 "\000\000\000\000\040\000\000\000\000\000\000\000\000\000\000\000"},
	{269, "del_stmt", 0, 3, states_13,
	 "\000\000\000\000\100\000\000\000\000\000\000\000\000\000\000\000"},
	{270, "pass_stmt", 0, 2, states_14,
	 "\000\000\000\000\000\001\000\000\000\000\000\000\000\000\000\000"},
	{271, "flow_stmt", 0, 2, states_15,
	 "\000\000\000\000\000\340\001\000\000\000\000\000\000\000\000\000"},
	{272, "break_stmt", 0, 2, states_16,
	 "\000\000\000\000\000\040\000\000\000\000\000\000\000\000\000\000"},
	{273, "continue_stmt", 0, 2, states_17,
	 "\000\000\000\000\000\100\000\000\000\000\000\000\000\000\000\000"},
	{274, "return_stmt", 0, 3, states_18,
	 "\000\000\000\000\000\200\000\000\000\000\000\000\000\000\000\000"},
	{275, "raise_stmt", 0, 7, states_19,
	 "\000\000\000\000\000\000\001\000\000\000\000\000\000\000\000\000"},
	{276, "import_stmt", 0, 9, states_20,
	 "\000\000\000\000\000\000\012\000\000\000\000\000\000\000\000\000"},
	{277, "dotted_name", 0, 2, states_21,
	 "\000\020\000\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{278, "global_stmt", 0, 3, states_22,
	 "\000\000\000\000\000\000\040\000\000\000\000\000\000\000\000\000"},
	{279, "access_stmt", 0, 7, states_23,
	 "\000\000\000\000\000\000\100\000\000\000\000\000\000\000\000\000"},
	{280, "accesstype", 0, 2, states_24,
	 "\000\020\000\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{281, "exec_stmt", 0, 7, states_25,
	 "\000\000\000\000\000\000\000\001\000\000\000\000\000\000\000\000"},
	{282, "compound_stmt", 0, 2, states_26,
	 "\000\010\000\000\000\000\000\000\071\000\000\000\000\000\000\010"},
	{283, "if_stmt", 0, 8, states_27,
	 "\000\000\000\000\000\000\000\000\001\000\000\000\000\000\000\000"},
	{284, "while_stmt", 0, 8, states_28,
	 "\000\000\000\000\000\000\000\000\010\000\000\000\000\000\000\000"},
	{285, "for_stmt", 0, 10, states_29,
	 "\000\000\000\000\000\000\000\000\020\000\000\000\000\000\000\000"},
	{286, "try_stmt", 0, 10, states_30,
	 "\000\000\000\000\000\000\000\000\040\000\000\000\000\000\000\000"},
	{287, "except_clause", 0, 5, states_31,
	 "\000\000\000\000\000\000\000\000\000\001\000\000\000\000\000\000"},
	{288, "suite", 0, 5, states_32,
	 "\004\020\001\000\140\341\153\001\000\000\001\000\140\104\171\000"},
	{289, "test", 0, 4, states_33,
	 "\000\020\001\000\000\000\000\000\000\000\001\000\140\104\171\000"},
	{290, "and_test", 0, 2, states_34,
	 "\000\020\001\000\000\000\000\000\000\000\001\000\140\104\071\000"},
	{291, "not_test", 0, 3, states_35,
	 "\000\020\001\000\000\000\000\000\000\000\001\000\140\104\071\000"},
	{292, "comparison", 0, 2, states_36,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\140\104\071\000"},
	{293, "comp_op", 0, 4, states_37,
	 "\000\000\000\000\000\000\000\004\000\000\371\007\000\000\000\000"},
	{294, "expr", 0, 2, states_38,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\140\104\071\000"},
	{295, "xor_expr", 0, 2, states_39,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\140\104\071\000"},
	{296, "and_expr", 0, 2, states_40,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\140\104\071\000"},
	{297, "shift_expr", 0, 2, states_41,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\140\104\071\000"},
	{298, "arith_expr", 0, 2, states_42,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\140\104\071\000"},
	{299, "term", 0, 2, states_43,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\140\104\071\000"},
	{300, "factor", 0, 3, states_44,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\140\104\071\000"},
	{301, "power", 0, 4, states_45,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\000\100\071\000"},
	{302, "atom", 0, 11, states_46,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\000\100\071\000"},
	{303, "lambdef", 0, 5, states_47,
	 "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\100\000"},
	{304, "trailer", 0, 7, states_48,
	 "\000\000\001\000\000\000\020\000\000\000\000\000\000\100\000\000"},
	{305, "subscriptlist", 0, 3, states_49,
	 "\000\120\001\000\000\000\020\000\000\000\001\000\140\104\171\000"},
	{306, "subscript", 0, 7, states_50,
	 "\000\120\001\000\000\000\020\000\000\000\001\000\140\104\171\000"},
	{307, "sliceop", 0, 3, states_51,
	 "\000\100\000\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{308, "exprlist", 0, 3, states_52,
	 "\000\020\001\000\000\000\000\000\000\000\000\000\140\104\071\000"},
	{309, "testlist", 0, 3, states_53,
	 "\000\020\001\000\000\000\000\000\000\000\001\000\140\104\171\000"},
	{310, "dictmaker", 0, 5, states_54,
	 "\000\020\001\000\000\000\000\000\000\000\001\000\140\104\171\000"},
	{311, "classdef", 0, 8, states_55,
	 "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\010"},
	{312, "arglist", 0, 3, states_56,
	 "\000\020\001\000\000\000\000\000\000\000\001\000\140\104\171\000"},
	{313, "argument", 0, 4, states_57,
	 "\000\020\001\000\000\000\000\000\000\000\001\000\140\104\171\000"},
};
static label labels[125] = {
	{0, "EMPTY"},
	{256, 0},
	{4, 0},
	{265, 0},
	{282, 0},
	{257, 0},
	{264, 0},
	{0, 0},
	{258, 0},
	{309, 0},
	{259, 0},
	{1, "def"},
	{1, 0},
	{260, 0},
	{11, 0},
	{288, 0},
	{7, 0},
	{261, 0},
	{8, 0},
	{262, 0},
	{22, 0},
	{289, 0},
	{12, 0},
	{16, 0},
	{36, 0},
	{263, 0},
	{266, 0},
	{13, 0},
	{267, 0},
	{268, 0},
	{269, 0},
	{270, 0},
	{271, 0},
	{276, 0},
	{278, 0},
	{279, 0},
	{281, 0},
	{1, "print"},
	{1, "del"},
	{308, 0},
	{1, "pass"},
	{272, 0},
	{273, 0},
	{274, 0},
	{275, 0},
	{1, "break"},
	{1, "continue"},
	{1, "return"},
	{1, "raise"},
	{1, "import"},
	{277, 0},
	{1, "from"},
	{23, 0},
	{1, "global"},
	{1, "access"},
	{280, 0},
	{1, "exec"},
	{294, 0},
	{1, "in"},
	{283, 0},
	{284, 0},
	{285, 0},
	{286, 0},
	{311, 0},
	{1, "if"},
	{1, "elif"},
	{1, "else"},
	{1, "while"},
	{1, "for"},
	{1, "try"},
	{287, 0},
	{1, "finally"},
	{1, "except"},
	{5, 0},
	{6, 0},
	{290, 0},
	{1, "or"},
	{303, 0},
	{291, 0},
	{1, "and"},
	{1, "not"},
	{292, 0},
	{293, 0},
	{20, 0},
	{21, 0},
	{28, 0},
	{31, 0},
	{30, 0},
	{29, 0},
	{29, 0},
	{1, "is"},
	{295, 0},
	{18, 0},
	{296, 0},
	{33, 0},
	{297, 0},
	{19, 0},
	{298, 0},
	{34, 0},
	{35, 0},
	{299, 0},
	{14, 0},
	{15, 0},
	{300, 0},
	{17, 0},
	{24, 0},
	{32, 0},
	{301, 0},
	{302, 0},
	{304, 0},
	{9, 0},
	{10, 0},
	{26, 0},
	{310, 0},
	{27, 0},
	{25, 0},
	{2, 0},
	{3, 0},
	{1, "lambda"},
	{312, 0},
	{305, 0},
	{306, 0},
	{307, 0},
	{1, "class"},
	{313, 0},
};
grammar gram = {
	58,
	dfas,
	{125, labels},
	256
};
