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
	{9, 3},
};
static arc arcs_3_3[2] = {
	{2, 3},
	{7, 4},
};
static arc arcs_3_4[1] = {
	{0, 4},
};
static state states_3[5] = {
	{1, arcs_3_0},
	{1, arcs_3_1},
	{1, arcs_3_2},
	{2, arcs_3_3},
	{1, arcs_3_4},
};
static arc arcs_4_0[1] = {
	{14, 1},
};
static arc arcs_4_1[1] = {
	{15, 2},
};
static arc arcs_4_2[1] = {
	{16, 3},
};
static arc arcs_4_3[1] = {
	{12, 4},
};
static arc arcs_4_4[1] = {
	{17, 5},
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
	{18, 1},
};
static arc arcs_5_1[2] = {
	{11, 2},
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
static arc arcs_6_0[2] = {
	{20, 1},
	{22, 2},
};
static arc arcs_6_1[2] = {
	{21, 3},
	{0, 1},
};
static arc arcs_6_2[1] = {
	{15, 4},
};
static arc arcs_6_3[3] = {
	{20, 1},
	{22, 2},
	{0, 3},
};
static arc arcs_6_4[1] = {
	{0, 4},
};
static state states_6[5] = {
	{2, arcs_6_0},
	{2, arcs_6_1},
	{1, arcs_6_2},
	{3, arcs_6_3},
	{1, arcs_6_4},
};
static arc arcs_7_0[2] = {
	{15, 1},
	{18, 2},
};
static arc arcs_7_1[1] = {
	{0, 1},
};
static arc arcs_7_2[1] = {
	{23, 3},
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
	{24, 1},
};
static arc arcs_10_1[2] = {
	{25, 2},
	{2, 3},
};
static arc arcs_10_2[2] = {
	{24, 1},
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
static arc arcs_11_0[9] = {
	{26, 1},
	{27, 1},
	{28, 1},
	{29, 1},
	{30, 1},
	{31, 1},
	{32, 1},
	{33, 1},
	{34, 1},
};
static arc arcs_11_1[1] = {
	{0, 1},
};
static state states_11[2] = {
	{9, arcs_11_0},
	{1, arcs_11_1},
};
static arc arcs_12_0[1] = {
	{9, 1},
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
	{39, 2},
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
	{40, 1},
};
static arc arcs_15_1[1] = {
	{0, 1},
};
static state states_15[2] = {
	{1, arcs_15_0},
	{1, arcs_15_1},
};
static arc arcs_16_0[4] = {
	{41, 1},
	{42, 1},
	{43, 1},
	{44, 1},
};
static arc arcs_16_1[1] = {
	{0, 1},
};
static state states_16[2] = {
	{4, arcs_16_0},
	{1, arcs_16_1},
};
static arc arcs_17_0[1] = {
	{45, 1},
};
static arc arcs_17_1[1] = {
	{0, 1},
};
static state states_17[2] = {
	{1, arcs_17_0},
	{1, arcs_17_1},
};
static arc arcs_18_0[1] = {
	{46, 1},
};
static arc arcs_18_1[1] = {
	{0, 1},
};
static state states_18[2] = {
	{1, arcs_18_0},
	{1, arcs_18_1},
};
static arc arcs_19_0[1] = {
	{47, 1},
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
	{48, 1},
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
	{49, 1},
	{50, 2},
};
static arc arcs_21_1[1] = {
	{15, 3},
};
static arc arcs_21_2[1] = {
	{15, 4},
};
static arc arcs_21_3[2] = {
	{21, 1},
	{0, 3},
};
static arc arcs_21_4[1] = {
	{49, 5},
};
static arc arcs_21_5[2] = {
	{22, 6},
	{15, 7},
};
static arc arcs_21_6[1] = {
	{0, 6},
};
static arc arcs_21_7[2] = {
	{21, 8},
	{0, 7},
};
static arc arcs_21_8[1] = {
	{15, 7},
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
	{51, 1},
};
static arc arcs_22_1[1] = {
	{15, 2},
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
static arc arcs_23_0[1] = {
	{52, 1},
};
static arc arcs_23_1[2] = {
	{22, 2},
	{15, 3},
};
static arc arcs_23_2[1] = {
	{12, 4},
};
static arc arcs_23_3[2] = {
	{21, 5},
	{12, 4},
};
static arc arcs_23_4[1] = {
	{53, 6},
};
static arc arcs_23_5[1] = {
	{15, 3},
};
static arc arcs_23_6[2] = {
	{21, 4},
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
	{15, 1},
};
static arc arcs_24_1[2] = {
	{15, 1},
	{0, 1},
};
static state states_24[2] = {
	{1, arcs_24_0},
	{2, arcs_24_1},
};
static arc arcs_25_0[1] = {
	{54, 1},
};
static arc arcs_25_1[1] = {
	{55, 2},
};
static arc arcs_25_2[2] = {
	{56, 3},
	{0, 2},
};
static arc arcs_25_3[1] = {
	{37, 4},
};
static arc arcs_25_4[2] = {
	{21, 5},
	{0, 4},
};
static arc arcs_25_5[1] = {
	{37, 6},
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
	{57, 1},
	{58, 1},
	{59, 1},
	{60, 1},
	{13, 1},
	{61, 1},
};
static arc arcs_26_1[1] = {
	{0, 1},
};
static state states_26[2] = {
	{6, arcs_26_0},
	{1, arcs_26_1},
};
static arc arcs_27_0[1] = {
	{62, 1},
};
static arc arcs_27_1[1] = {
	{37, 2},
};
static arc arcs_27_2[1] = {
	{12, 3},
};
static arc arcs_27_3[1] = {
	{17, 4},
};
static arc arcs_27_4[3] = {
	{63, 1},
	{64, 5},
	{0, 4},
};
static arc arcs_27_5[1] = {
	{12, 6},
};
static arc arcs_27_6[1] = {
	{17, 7},
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
	{65, 1},
};
static arc arcs_28_1[1] = {
	{37, 2},
};
static arc arcs_28_2[1] = {
	{12, 3},
};
static arc arcs_28_3[1] = {
	{17, 4},
};
static arc arcs_28_4[2] = {
	{64, 5},
	{0, 4},
};
static arc arcs_28_5[1] = {
	{12, 6},
};
static arc arcs_28_6[1] = {
	{17, 7},
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
	{66, 1},
};
static arc arcs_29_1[1] = {
	{39, 2},
};
static arc arcs_29_2[1] = {
	{56, 3},
};
static arc arcs_29_3[1] = {
	{9, 4},
};
static arc arcs_29_4[1] = {
	{12, 5},
};
static arc arcs_29_5[1] = {
	{17, 6},
};
static arc arcs_29_6[2] = {
	{64, 7},
	{0, 6},
};
static arc arcs_29_7[1] = {
	{12, 8},
};
static arc arcs_29_8[1] = {
	{17, 9},
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
	{67, 1},
};
static arc arcs_30_1[1] = {
	{12, 2},
};
static arc arcs_30_2[1] = {
	{17, 3},
};
static arc arcs_30_3[2] = {
	{68, 4},
	{69, 5},
};
static arc arcs_30_4[1] = {
	{12, 6},
};
static arc arcs_30_5[1] = {
	{12, 7},
};
static arc arcs_30_6[1] = {
	{17, 8},
};
static arc arcs_30_7[1] = {
	{17, 9},
};
static arc arcs_30_8[2] = {
	{68, 4},
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
	{2, arcs_30_8},
	{1, arcs_30_9},
};
static arc arcs_31_0[1] = {
	{70, 1},
};
static arc arcs_31_1[2] = {
	{37, 2},
	{0, 1},
};
static arc arcs_31_2[2] = {
	{21, 3},
	{0, 2},
};
static arc arcs_31_3[1] = {
	{37, 4},
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
	{71, 3},
};
static arc arcs_32_3[1] = {
	{6, 4},
};
static arc arcs_32_4[2] = {
	{6, 4},
	{72, 1},
};
static state states_32[5] = {
	{2, arcs_32_0},
	{1, arcs_32_1},
	{1, arcs_32_2},
	{1, arcs_32_3},
	{2, arcs_32_4},
};
static arc arcs_33_0[1] = {
	{73, 1},
};
static arc arcs_33_1[2] = {
	{74, 0},
	{0, 1},
};
static state states_33[2] = {
	{1, arcs_33_0},
	{2, arcs_33_1},
};
static arc arcs_34_0[1] = {
	{75, 1},
};
static arc arcs_34_1[2] = {
	{76, 0},
	{0, 1},
};
static state states_34[2] = {
	{1, arcs_34_0},
	{2, arcs_34_1},
};
static arc arcs_35_0[2] = {
	{77, 1},
	{78, 2},
};
static arc arcs_35_1[1] = {
	{75, 2},
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
	{55, 1},
};
static arc arcs_36_1[2] = {
	{79, 0},
	{0, 1},
};
static state states_36[2] = {
	{1, arcs_36_0},
	{2, arcs_36_1},
};
static arc arcs_37_0[10] = {
	{80, 1},
	{81, 1},
	{82, 1},
	{83, 1},
	{84, 1},
	{85, 1},
	{86, 1},
	{56, 1},
	{77, 2},
	{87, 3},
};
static arc arcs_37_1[1] = {
	{0, 1},
};
static arc arcs_37_2[1] = {
	{56, 1},
};
static arc arcs_37_3[2] = {
	{77, 1},
	{0, 3},
};
static state states_37[4] = {
	{10, arcs_37_0},
	{1, arcs_37_1},
	{1, arcs_37_2},
	{2, arcs_37_3},
};
static arc arcs_38_0[1] = {
	{88, 1},
};
static arc arcs_38_1[2] = {
	{89, 0},
	{0, 1},
};
static state states_38[2] = {
	{1, arcs_38_0},
	{2, arcs_38_1},
};
static arc arcs_39_0[1] = {
	{90, 1},
};
static arc arcs_39_1[2] = {
	{91, 0},
	{0, 1},
};
static state states_39[2] = {
	{1, arcs_39_0},
	{2, arcs_39_1},
};
static arc arcs_40_0[1] = {
	{92, 1},
};
static arc arcs_40_1[2] = {
	{93, 0},
	{0, 1},
};
static state states_40[2] = {
	{1, arcs_40_0},
	{2, arcs_40_1},
};
static arc arcs_41_0[1] = {
	{94, 1},
};
static arc arcs_41_1[3] = {
	{95, 0},
	{96, 0},
	{0, 1},
};
static state states_41[2] = {
	{1, arcs_41_0},
	{3, arcs_41_1},
};
static arc arcs_42_0[1] = {
	{97, 1},
};
static arc arcs_42_1[3] = {
	{98, 0},
	{99, 0},
	{0, 1},
};
static state states_42[2] = {
	{1, arcs_42_0},
	{3, arcs_42_1},
};
static arc arcs_43_0[1] = {
	{100, 1},
};
static arc arcs_43_1[4] = {
	{22, 0},
	{101, 0},
	{102, 0},
	{0, 1},
};
static state states_43[2] = {
	{1, arcs_43_0},
	{4, arcs_43_1},
};
static arc arcs_44_0[4] = {
	{98, 1},
	{99, 1},
	{103, 1},
	{104, 2},
};
static arc arcs_44_1[1] = {
	{100, 3},
};
static arc arcs_44_2[2] = {
	{105, 2},
	{0, 2},
};
static arc arcs_44_3[1] = {
	{0, 3},
};
static state states_44[4] = {
	{4, arcs_44_0},
	{1, arcs_44_1},
	{2, arcs_44_2},
	{1, arcs_44_3},
};
static arc arcs_45_0[7] = {
	{18, 1},
	{106, 2},
	{108, 3},
	{111, 4},
	{15, 5},
	{112, 5},
	{113, 5},
};
static arc arcs_45_1[2] = {
	{9, 6},
	{19, 5},
};
static arc arcs_45_2[2] = {
	{9, 7},
	{107, 5},
};
static arc arcs_45_3[2] = {
	{109, 8},
	{110, 5},
};
static arc arcs_45_4[1] = {
	{9, 9},
};
static arc arcs_45_5[1] = {
	{0, 5},
};
static arc arcs_45_6[1] = {
	{19, 5},
};
static arc arcs_45_7[1] = {
	{107, 5},
};
static arc arcs_45_8[1] = {
	{110, 5},
};
static arc arcs_45_9[1] = {
	{111, 5},
};
static state states_45[10] = {
	{7, arcs_45_0},
	{2, arcs_45_1},
	{2, arcs_45_2},
	{2, arcs_45_3},
	{1, arcs_45_4},
	{1, arcs_45_5},
	{1, arcs_45_6},
	{1, arcs_45_7},
	{1, arcs_45_8},
	{1, arcs_45_9},
};
static arc arcs_46_0[3] = {
	{18, 1},
	{106, 2},
	{115, 3},
};
static arc arcs_46_1[2] = {
	{9, 4},
	{19, 5},
};
static arc arcs_46_2[1] = {
	{114, 6},
};
static arc arcs_46_3[1] = {
	{15, 5},
};
static arc arcs_46_4[1] = {
	{19, 5},
};
static arc arcs_46_5[1] = {
	{0, 5},
};
static arc arcs_46_6[1] = {
	{107, 5},
};
static state states_46[7] = {
	{3, arcs_46_0},
	{2, arcs_46_1},
	{1, arcs_46_2},
	{1, arcs_46_3},
	{1, arcs_46_4},
	{1, arcs_46_5},
	{1, arcs_46_6},
};
static arc arcs_47_0[2] = {
	{37, 1},
	{12, 2},
};
static arc arcs_47_1[2] = {
	{12, 2},
	{0, 1},
};
static arc arcs_47_2[2] = {
	{37, 3},
	{0, 2},
};
static arc arcs_47_3[1] = {
	{0, 3},
};
static state states_47[4] = {
	{2, arcs_47_0},
	{2, arcs_47_1},
	{2, arcs_47_2},
	{1, arcs_47_3},
};
static arc arcs_48_0[1] = {
	{55, 1},
};
static arc arcs_48_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_48_2[2] = {
	{55, 1},
	{0, 2},
};
static state states_48[3] = {
	{1, arcs_48_0},
	{2, arcs_48_1},
	{2, arcs_48_2},
};
static arc arcs_49_0[1] = {
	{37, 1},
};
static arc arcs_49_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_49_2[2] = {
	{37, 1},
	{0, 2},
};
static state states_49[3] = {
	{1, arcs_49_0},
	{2, arcs_49_1},
	{2, arcs_49_2},
};
static arc arcs_50_0[1] = {
	{37, 1},
};
static arc arcs_50_1[1] = {
	{12, 2},
};
static arc arcs_50_2[1] = {
	{37, 3},
};
static arc arcs_50_3[2] = {
	{21, 4},
	{0, 3},
};
static arc arcs_50_4[2] = {
	{37, 1},
	{0, 4},
};
static state states_50[5] = {
	{1, arcs_50_0},
	{1, arcs_50_1},
	{1, arcs_50_2},
	{2, arcs_50_3},
	{2, arcs_50_4},
};
static arc arcs_51_0[1] = {
	{116, 1},
};
static arc arcs_51_1[1] = {
	{15, 2},
};
static arc arcs_51_2[2] = {
	{18, 3},
	{12, 4},
};
static arc arcs_51_3[1] = {
	{9, 5},
};
static arc arcs_51_4[1] = {
	{17, 6},
};
static arc arcs_51_5[1] = {
	{19, 7},
};
static arc arcs_51_6[1] = {
	{0, 6},
};
static arc arcs_51_7[1] = {
	{12, 4},
};
static state states_51[8] = {
	{1, arcs_51_0},
	{1, arcs_51_1},
	{2, arcs_51_2},
	{1, arcs_51_3},
	{1, arcs_51_4},
	{1, arcs_51_5},
	{1, arcs_51_6},
	{1, arcs_51_7},
};
static dfa dfas[52] = {
	{256, "single_input", 0, 3, states_0,
	 "\004\300\004\000\120\341\137\100\016\040\000\000\214\224\023"},
	{257, "file_input", 0, 2, states_1,
	 "\204\300\004\000\120\341\137\100\016\040\000\000\214\224\023"},
	{258, "eval_input", 0, 3, states_2,
	 "\000\200\004\000\000\000\000\000\000\040\000\000\214\224\003"},
	{259, "lambda_input", 0, 5, states_3,
	 "\000\200\104\000\000\000\000\000\000\000\000\000\000\000\000"},
	{260, "funcdef", 0, 6, states_4,
	 "\000\100\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{261, "parameters", 0, 4, states_5,
	 "\000\000\004\000\000\000\000\000\000\000\000\000\000\000\000"},
	{262, "varargslist", 0, 5, states_6,
	 "\000\200\104\000\000\000\000\000\000\000\000\000\000\000\000"},
	{263, "fpdef", 0, 4, states_7,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\000\000\000"},
	{264, "fplist", 0, 3, states_8,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\000\000\000"},
	{265, "stmt", 0, 2, states_9,
	 "\000\300\004\000\120\341\137\100\016\040\000\000\214\224\023"},
	{266, "simple_stmt", 0, 4, states_10,
	 "\000\200\004\000\120\341\137\000\000\040\000\000\214\224\003"},
	{267, "small_stmt", 0, 2, states_11,
	 "\000\200\004\000\120\341\137\000\000\040\000\000\214\224\003"},
	{268, "expr_stmt", 0, 2, states_12,
	 "\000\200\004\000\000\000\000\000\000\040\000\000\214\224\003"},
	{269, "print_stmt", 0, 3, states_13,
	 "\000\000\000\000\020\000\000\000\000\000\000\000\000\000\000"},
	{270, "del_stmt", 0, 3, states_14,
	 "\000\000\000\000\100\000\000\000\000\000\000\000\000\000\000"},
	{271, "pass_stmt", 0, 2, states_15,
	 "\000\000\000\000\000\001\000\000\000\000\000\000\000\000\000"},
	{272, "flow_stmt", 0, 2, states_16,
	 "\000\000\000\000\000\340\001\000\000\000\000\000\000\000\000"},
	{273, "break_stmt", 0, 2, states_17,
	 "\000\000\000\000\000\040\000\000\000\000\000\000\000\000\000"},
	{274, "continue_stmt", 0, 2, states_18,
	 "\000\000\000\000\000\100\000\000\000\000\000\000\000\000\000"},
	{275, "return_stmt", 0, 3, states_19,
	 "\000\000\000\000\000\200\000\000\000\000\000\000\000\000\000"},
	{276, "raise_stmt", 0, 5, states_20,
	 "\000\000\000\000\000\000\001\000\000\000\000\000\000\000\000"},
	{277, "import_stmt", 0, 9, states_21,
	 "\000\000\000\000\000\000\006\000\000\000\000\000\000\000\000"},
	{278, "global_stmt", 0, 3, states_22,
	 "\000\000\000\000\000\000\010\000\000\000\000\000\000\000\000"},
	{279, "access_stmt", 0, 7, states_23,
	 "\000\000\000\000\000\000\020\000\000\000\000\000\000\000\000"},
	{280, "accesstype", 0, 2, states_24,
	 "\000\200\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{281, "exec_stmt", 0, 7, states_25,
	 "\000\000\000\000\000\000\100\000\000\000\000\000\000\000\000"},
	{282, "compound_stmt", 0, 2, states_26,
	 "\000\100\000\000\000\000\000\100\016\000\000\000\000\000\020"},
	{283, "if_stmt", 0, 8, states_27,
	 "\000\000\000\000\000\000\000\100\000\000\000\000\000\000\000"},
	{284, "while_stmt", 0, 8, states_28,
	 "\000\000\000\000\000\000\000\000\002\000\000\000\000\000\000"},
	{285, "for_stmt", 0, 10, states_29,
	 "\000\000\000\000\000\000\000\000\004\000\000\000\000\000\000"},
	{286, "try_stmt", 0, 10, states_30,
	 "\000\000\000\000\000\000\000\000\010\000\000\000\000\000\000"},
	{287, "except_clause", 0, 5, states_31,
	 "\000\000\000\000\000\000\000\000\100\000\000\000\000\000\000"},
	{288, "suite", 0, 5, states_32,
	 "\004\200\004\000\120\341\137\000\000\040\000\000\214\224\003"},
	{289, "test", 0, 2, states_33,
	 "\000\200\004\000\000\000\000\000\000\040\000\000\214\224\003"},
	{290, "and_test", 0, 2, states_34,
	 "\000\200\004\000\000\000\000\000\000\040\000\000\214\224\003"},
	{291, "not_test", 0, 3, states_35,
	 "\000\200\004\000\000\000\000\000\000\040\000\000\214\224\003"},
	{292, "comparison", 0, 2, states_36,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\214\224\003"},
	{293, "comp_op", 0, 4, states_37,
	 "\000\000\000\000\000\000\000\001\000\040\377\000\000\000\000"},
	{294, "expr", 0, 2, states_38,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\214\224\003"},
	{295, "xor_expr", 0, 2, states_39,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\214\224\003"},
	{296, "and_expr", 0, 2, states_40,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\214\224\003"},
	{297, "shift_expr", 0, 2, states_41,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\214\224\003"},
	{298, "arith_expr", 0, 2, states_42,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\214\224\003"},
	{299, "term", 0, 2, states_43,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\214\224\003"},
	{300, "factor", 0, 4, states_44,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\214\224\003"},
	{301, "atom", 0, 10, states_45,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\000\224\003"},
	{302, "trailer", 0, 7, states_46,
	 "\000\000\004\000\000\000\000\000\000\000\000\000\000\004\010"},
	{303, "subscript", 0, 4, states_47,
	 "\000\220\004\000\000\000\000\000\000\040\000\000\214\224\003"},
	{304, "exprlist", 0, 3, states_48,
	 "\000\200\004\000\000\000\000\000\000\000\000\000\214\224\003"},
	{305, "testlist", 0, 3, states_49,
	 "\000\200\004\000\000\000\000\000\000\040\000\000\214\224\003"},
	{306, "dictmaker", 0, 5, states_50,
	 "\000\200\004\000\000\000\000\000\000\040\000\000\214\224\003"},
	{307, "classdef", 0, 8, states_51,
	 "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\020"},
};
static label labels[117] = {
	{0, "EMPTY"},
	{256, 0},
	{4, 0},
	{266, 0},
	{282, 0},
	{257, 0},
	{265, 0},
	{0, 0},
	{258, 0},
	{305, 0},
	{259, 0},
	{262, 0},
	{11, 0},
	{260, 0},
	{1, "def"},
	{1, 0},
	{261, 0},
	{288, 0},
	{7, 0},
	{8, 0},
	{263, 0},
	{12, 0},
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
	{279, 0},
	{281, 0},
	{22, 0},
	{1, "print"},
	{289, 0},
	{1, "del"},
	{304, 0},
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
	{1, "access"},
	{280, 0},
	{1, "exec"},
	{294, 0},
	{1, "in"},
	{283, 0},
	{284, 0},
	{285, 0},
	{286, 0},
	{307, 0},
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
	{9, 0},
	{10, 0},
	{26, 0},
	{306, 0},
	{27, 0},
	{25, 0},
	{2, 0},
	{3, 0},
	{303, 0},
	{23, 0},
	{1, "class"},
};
grammar gram = {
	52,
	dfas,
	{117, labels},
	256
};
