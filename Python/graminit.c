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
	{9, 3},
};
static arc arcs_4_3[2] = {
	{2, 3},
	{7, 4},
};
static arc arcs_4_4[1] = {
	{0, 4},
};
static state states_4[5] = {
	{1, arcs_4_0},
	{1, arcs_4_1},
	{1, arcs_4_2},
	{2, arcs_4_3},
	{1, arcs_4_4},
};
static arc arcs_5_0[1] = {
	{15, 1},
};
static arc arcs_5_1[1] = {
	{16, 2},
};
static arc arcs_5_2[1] = {
	{17, 3},
};
static arc arcs_5_3[1] = {
	{13, 4},
};
static arc arcs_5_4[1] = {
	{18, 5},
};
static arc arcs_5_5[1] = {
	{0, 5},
};
static state states_5[6] = {
	{1, arcs_5_0},
	{1, arcs_5_1},
	{1, arcs_5_2},
	{1, arcs_5_3},
	{1, arcs_5_4},
	{1, arcs_5_5},
};
static arc arcs_6_0[1] = {
	{19, 1},
};
static arc arcs_6_1[2] = {
	{12, 2},
	{20, 3},
};
static arc arcs_6_2[1] = {
	{20, 3},
};
static arc arcs_6_3[1] = {
	{0, 3},
};
static state states_6[4] = {
	{1, arcs_6_0},
	{2, arcs_6_1},
	{1, arcs_6_2},
	{1, arcs_6_3},
};
static arc arcs_7_0[2] = {
	{21, 1},
	{23, 2},
};
static arc arcs_7_1[2] = {
	{22, 3},
	{0, 1},
};
static arc arcs_7_2[1] = {
	{16, 4},
};
static arc arcs_7_3[3] = {
	{21, 1},
	{23, 2},
	{0, 3},
};
static arc arcs_7_4[1] = {
	{0, 4},
};
static state states_7[5] = {
	{2, arcs_7_0},
	{2, arcs_7_1},
	{1, arcs_7_2},
	{3, arcs_7_3},
	{1, arcs_7_4},
};
static arc arcs_8_0[2] = {
	{16, 1},
	{19, 2},
};
static arc arcs_8_1[1] = {
	{0, 1},
};
static arc arcs_8_2[1] = {
	{24, 3},
};
static arc arcs_8_3[1] = {
	{20, 1},
};
static state states_8[4] = {
	{2, arcs_8_0},
	{1, arcs_8_1},
	{1, arcs_8_2},
	{1, arcs_8_3},
};
static arc arcs_9_0[1] = {
	{21, 1},
};
static arc arcs_9_1[2] = {
	{22, 2},
	{0, 1},
};
static arc arcs_9_2[2] = {
	{21, 1},
	{0, 2},
};
static state states_9[3] = {
	{1, arcs_9_0},
	{2, arcs_9_1},
	{2, arcs_9_2},
};
static arc arcs_10_0[2] = {
	{3, 1},
	{4, 1},
};
static arc arcs_10_1[1] = {
	{0, 1},
};
static state states_10[2] = {
	{2, arcs_10_0},
	{1, arcs_10_1},
};
static arc arcs_11_0[1] = {
	{25, 1},
};
static arc arcs_11_1[2] = {
	{26, 2},
	{2, 3},
};
static arc arcs_11_2[2] = {
	{25, 1},
	{2, 3},
};
static arc arcs_11_3[1] = {
	{0, 3},
};
static state states_11[4] = {
	{1, arcs_11_0},
	{2, arcs_11_1},
	{2, arcs_11_2},
	{1, arcs_11_3},
};
static arc arcs_12_0[9] = {
	{27, 1},
	{28, 1},
	{29, 1},
	{30, 1},
	{31, 1},
	{32, 1},
	{33, 1},
	{34, 1},
	{35, 1},
};
static arc arcs_12_1[1] = {
	{0, 1},
};
static state states_12[2] = {
	{9, arcs_12_0},
	{1, arcs_12_1},
};
static arc arcs_13_0[1] = {
	{9, 1},
};
static arc arcs_13_1[2] = {
	{36, 0},
	{0, 1},
};
static state states_13[2] = {
	{1, arcs_13_0},
	{2, arcs_13_1},
};
static arc arcs_14_0[1] = {
	{37, 1},
};
static arc arcs_14_1[2] = {
	{38, 2},
	{0, 1},
};
static arc arcs_14_2[2] = {
	{22, 1},
	{0, 2},
};
static state states_14[3] = {
	{1, arcs_14_0},
	{2, arcs_14_1},
	{2, arcs_14_2},
};
static arc arcs_15_0[1] = {
	{39, 1},
};
static arc arcs_15_1[1] = {
	{40, 2},
};
static arc arcs_15_2[1] = {
	{0, 2},
};
static state states_15[3] = {
	{1, arcs_15_0},
	{1, arcs_15_1},
	{1, arcs_15_2},
};
static arc arcs_16_0[1] = {
	{41, 1},
};
static arc arcs_16_1[1] = {
	{0, 1},
};
static state states_16[2] = {
	{1, arcs_16_0},
	{1, arcs_16_1},
};
static arc arcs_17_0[4] = {
	{42, 1},
	{43, 1},
	{44, 1},
	{45, 1},
};
static arc arcs_17_1[1] = {
	{0, 1},
};
static state states_17[2] = {
	{4, arcs_17_0},
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
static arc arcs_19_1[1] = {
	{0, 1},
};
static state states_19[2] = {
	{1, arcs_19_0},
	{1, arcs_19_1},
};
static arc arcs_20_0[1] = {
	{48, 1},
};
static arc arcs_20_1[2] = {
	{9, 2},
	{0, 1},
};
static arc arcs_20_2[1] = {
	{0, 2},
};
static state states_20[3] = {
	{1, arcs_20_0},
	{2, arcs_20_1},
	{1, arcs_20_2},
};
static arc arcs_21_0[1] = {
	{49, 1},
};
static arc arcs_21_1[1] = {
	{38, 2},
};
static arc arcs_21_2[2] = {
	{22, 3},
	{0, 2},
};
static arc arcs_21_3[1] = {
	{38, 4},
};
static arc arcs_21_4[1] = {
	{0, 4},
};
static state states_21[5] = {
	{1, arcs_21_0},
	{1, arcs_21_1},
	{2, arcs_21_2},
	{1, arcs_21_3},
	{1, arcs_21_4},
};
static arc arcs_22_0[2] = {
	{50, 1},
	{51, 2},
};
static arc arcs_22_1[1] = {
	{16, 3},
};
static arc arcs_22_2[1] = {
	{16, 4},
};
static arc arcs_22_3[2] = {
	{22, 1},
	{0, 3},
};
static arc arcs_22_4[1] = {
	{50, 5},
};
static arc arcs_22_5[2] = {
	{23, 6},
	{16, 7},
};
static arc arcs_22_6[1] = {
	{0, 6},
};
static arc arcs_22_7[2] = {
	{22, 8},
	{0, 7},
};
static arc arcs_22_8[1] = {
	{16, 7},
};
static state states_22[9] = {
	{2, arcs_22_0},
	{1, arcs_22_1},
	{1, arcs_22_2},
	{2, arcs_22_3},
	{1, arcs_22_4},
	{2, arcs_22_5},
	{1, arcs_22_6},
	{2, arcs_22_7},
	{1, arcs_22_8},
};
static arc arcs_23_0[1] = {
	{52, 1},
};
static arc arcs_23_1[1] = {
	{16, 2},
};
static arc arcs_23_2[2] = {
	{22, 1},
	{0, 2},
};
static state states_23[3] = {
	{1, arcs_23_0},
	{1, arcs_23_1},
	{2, arcs_23_2},
};
static arc arcs_24_0[1] = {
	{53, 1},
};
static arc arcs_24_1[2] = {
	{23, 2},
	{16, 3},
};
static arc arcs_24_2[1] = {
	{13, 4},
};
static arc arcs_24_3[2] = {
	{22, 5},
	{13, 4},
};
static arc arcs_24_4[1] = {
	{54, 6},
};
static arc arcs_24_5[1] = {
	{16, 3},
};
static arc arcs_24_6[2] = {
	{22, 4},
	{0, 6},
};
static state states_24[7] = {
	{1, arcs_24_0},
	{2, arcs_24_1},
	{1, arcs_24_2},
	{2, arcs_24_3},
	{1, arcs_24_4},
	{1, arcs_24_5},
	{2, arcs_24_6},
};
static arc arcs_25_0[1] = {
	{16, 1},
};
static arc arcs_25_1[2] = {
	{16, 1},
	{0, 1},
};
static state states_25[2] = {
	{1, arcs_25_0},
	{2, arcs_25_1},
};
static arc arcs_26_0[1] = {
	{55, 1},
};
static arc arcs_26_1[1] = {
	{56, 2},
};
static arc arcs_26_2[2] = {
	{57, 3},
	{0, 2},
};
static arc arcs_26_3[1] = {
	{38, 4},
};
static arc arcs_26_4[2] = {
	{22, 5},
	{0, 4},
};
static arc arcs_26_5[1] = {
	{38, 6},
};
static arc arcs_26_6[1] = {
	{0, 6},
};
static state states_26[7] = {
	{1, arcs_26_0},
	{1, arcs_26_1},
	{2, arcs_26_2},
	{1, arcs_26_3},
	{2, arcs_26_4},
	{1, arcs_26_5},
	{1, arcs_26_6},
};
static arc arcs_27_0[6] = {
	{58, 1},
	{59, 1},
	{60, 1},
	{61, 1},
	{14, 1},
	{62, 1},
};
static arc arcs_27_1[1] = {
	{0, 1},
};
static state states_27[2] = {
	{6, arcs_27_0},
	{1, arcs_27_1},
};
static arc arcs_28_0[1] = {
	{63, 1},
};
static arc arcs_28_1[1] = {
	{38, 2},
};
static arc arcs_28_2[1] = {
	{13, 3},
};
static arc arcs_28_3[1] = {
	{18, 4},
};
static arc arcs_28_4[3] = {
	{64, 1},
	{65, 5},
	{0, 4},
};
static arc arcs_28_5[1] = {
	{13, 6},
};
static arc arcs_28_6[1] = {
	{18, 7},
};
static arc arcs_28_7[1] = {
	{0, 7},
};
static state states_28[8] = {
	{1, arcs_28_0},
	{1, arcs_28_1},
	{1, arcs_28_2},
	{1, arcs_28_3},
	{3, arcs_28_4},
	{1, arcs_28_5},
	{1, arcs_28_6},
	{1, arcs_28_7},
};
static arc arcs_29_0[1] = {
	{66, 1},
};
static arc arcs_29_1[1] = {
	{38, 2},
};
static arc arcs_29_2[1] = {
	{13, 3},
};
static arc arcs_29_3[1] = {
	{18, 4},
};
static arc arcs_29_4[2] = {
	{65, 5},
	{0, 4},
};
static arc arcs_29_5[1] = {
	{13, 6},
};
static arc arcs_29_6[1] = {
	{18, 7},
};
static arc arcs_29_7[1] = {
	{0, 7},
};
static state states_29[8] = {
	{1, arcs_29_0},
	{1, arcs_29_1},
	{1, arcs_29_2},
	{1, arcs_29_3},
	{2, arcs_29_4},
	{1, arcs_29_5},
	{1, arcs_29_6},
	{1, arcs_29_7},
};
static arc arcs_30_0[1] = {
	{67, 1},
};
static arc arcs_30_1[1] = {
	{40, 2},
};
static arc arcs_30_2[1] = {
	{57, 3},
};
static arc arcs_30_3[1] = {
	{9, 4},
};
static arc arcs_30_4[1] = {
	{13, 5},
};
static arc arcs_30_5[1] = {
	{18, 6},
};
static arc arcs_30_6[2] = {
	{65, 7},
	{0, 6},
};
static arc arcs_30_7[1] = {
	{13, 8},
};
static arc arcs_30_8[1] = {
	{18, 9},
};
static arc arcs_30_9[1] = {
	{0, 9},
};
static state states_30[10] = {
	{1, arcs_30_0},
	{1, arcs_30_1},
	{1, arcs_30_2},
	{1, arcs_30_3},
	{1, arcs_30_4},
	{1, arcs_30_5},
	{2, arcs_30_6},
	{1, arcs_30_7},
	{1, arcs_30_8},
	{1, arcs_30_9},
};
static arc arcs_31_0[1] = {
	{68, 1},
};
static arc arcs_31_1[1] = {
	{13, 2},
};
static arc arcs_31_2[1] = {
	{18, 3},
};
static arc arcs_31_3[2] = {
	{69, 4},
	{70, 5},
};
static arc arcs_31_4[1] = {
	{13, 6},
};
static arc arcs_31_5[1] = {
	{13, 7},
};
static arc arcs_31_6[1] = {
	{18, 8},
};
static arc arcs_31_7[1] = {
	{18, 9},
};
static arc arcs_31_8[2] = {
	{69, 4},
	{0, 8},
};
static arc arcs_31_9[1] = {
	{0, 9},
};
static state states_31[10] = {
	{1, arcs_31_0},
	{1, arcs_31_1},
	{1, arcs_31_2},
	{2, arcs_31_3},
	{1, arcs_31_4},
	{1, arcs_31_5},
	{1, arcs_31_6},
	{1, arcs_31_7},
	{2, arcs_31_8},
	{1, arcs_31_9},
};
static arc arcs_32_0[1] = {
	{71, 1},
};
static arc arcs_32_1[2] = {
	{38, 2},
	{0, 1},
};
static arc arcs_32_2[2] = {
	{22, 3},
	{0, 2},
};
static arc arcs_32_3[1] = {
	{38, 4},
};
static arc arcs_32_4[1] = {
	{0, 4},
};
static state states_32[5] = {
	{1, arcs_32_0},
	{2, arcs_32_1},
	{2, arcs_32_2},
	{1, arcs_32_3},
	{1, arcs_32_4},
};
static arc arcs_33_0[2] = {
	{3, 1},
	{2, 2},
};
static arc arcs_33_1[1] = {
	{0, 1},
};
static arc arcs_33_2[1] = {
	{72, 3},
};
static arc arcs_33_3[1] = {
	{6, 4},
};
static arc arcs_33_4[2] = {
	{6, 4},
	{73, 1},
};
static state states_33[5] = {
	{2, arcs_33_0},
	{1, arcs_33_1},
	{1, arcs_33_2},
	{1, arcs_33_3},
	{2, arcs_33_4},
};
static arc arcs_34_0[1] = {
	{74, 1},
};
static arc arcs_34_1[2] = {
	{75, 0},
	{0, 1},
};
static state states_34[2] = {
	{1, arcs_34_0},
	{2, arcs_34_1},
};
static arc arcs_35_0[1] = {
	{76, 1},
};
static arc arcs_35_1[2] = {
	{77, 0},
	{0, 1},
};
static state states_35[2] = {
	{1, arcs_35_0},
	{2, arcs_35_1},
};
static arc arcs_36_0[2] = {
	{78, 1},
	{79, 2},
};
static arc arcs_36_1[1] = {
	{76, 2},
};
static arc arcs_36_2[1] = {
	{0, 2},
};
static state states_36[3] = {
	{2, arcs_36_0},
	{1, arcs_36_1},
	{1, arcs_36_2},
};
static arc arcs_37_0[1] = {
	{56, 1},
};
static arc arcs_37_1[2] = {
	{80, 0},
	{0, 1},
};
static state states_37[2] = {
	{1, arcs_37_0},
	{2, arcs_37_1},
};
static arc arcs_38_0[10] = {
	{81, 1},
	{82, 1},
	{83, 1},
	{84, 1},
	{85, 1},
	{86, 1},
	{87, 1},
	{57, 1},
	{78, 2},
	{88, 3},
};
static arc arcs_38_1[1] = {
	{0, 1},
};
static arc arcs_38_2[1] = {
	{57, 1},
};
static arc arcs_38_3[2] = {
	{78, 1},
	{0, 3},
};
static state states_38[4] = {
	{10, arcs_38_0},
	{1, arcs_38_1},
	{1, arcs_38_2},
	{2, arcs_38_3},
};
static arc arcs_39_0[1] = {
	{89, 1},
};
static arc arcs_39_1[2] = {
	{90, 0},
	{0, 1},
};
static state states_39[2] = {
	{1, arcs_39_0},
	{2, arcs_39_1},
};
static arc arcs_40_0[1] = {
	{91, 1},
};
static arc arcs_40_1[2] = {
	{92, 0},
	{0, 1},
};
static state states_40[2] = {
	{1, arcs_40_0},
	{2, arcs_40_1},
};
static arc arcs_41_0[1] = {
	{93, 1},
};
static arc arcs_41_1[2] = {
	{94, 0},
	{0, 1},
};
static state states_41[2] = {
	{1, arcs_41_0},
	{2, arcs_41_1},
};
static arc arcs_42_0[1] = {
	{95, 1},
};
static arc arcs_42_1[3] = {
	{96, 0},
	{97, 0},
	{0, 1},
};
static state states_42[2] = {
	{1, arcs_42_0},
	{3, arcs_42_1},
};
static arc arcs_43_0[1] = {
	{98, 1},
};
static arc arcs_43_1[3] = {
	{99, 0},
	{100, 0},
	{0, 1},
};
static state states_43[2] = {
	{1, arcs_43_0},
	{3, arcs_43_1},
};
static arc arcs_44_0[1] = {
	{101, 1},
};
static arc arcs_44_1[4] = {
	{23, 0},
	{102, 0},
	{103, 0},
	{0, 1},
};
static state states_44[2] = {
	{1, arcs_44_0},
	{4, arcs_44_1},
};
static arc arcs_45_0[4] = {
	{99, 1},
	{100, 1},
	{104, 1},
	{105, 2},
};
static arc arcs_45_1[1] = {
	{101, 3},
};
static arc arcs_45_2[2] = {
	{106, 2},
	{0, 2},
};
static arc arcs_45_3[1] = {
	{0, 3},
};
static state states_45[4] = {
	{4, arcs_45_0},
	{1, arcs_45_1},
	{2, arcs_45_2},
	{1, arcs_45_3},
};
static arc arcs_46_0[7] = {
	{19, 1},
	{107, 2},
	{109, 3},
	{112, 4},
	{16, 5},
	{113, 5},
	{114, 5},
};
static arc arcs_46_1[2] = {
	{9, 6},
	{20, 5},
};
static arc arcs_46_2[2] = {
	{9, 7},
	{108, 5},
};
static arc arcs_46_3[2] = {
	{110, 8},
	{111, 5},
};
static arc arcs_46_4[1] = {
	{9, 9},
};
static arc arcs_46_5[1] = {
	{0, 5},
};
static arc arcs_46_6[1] = {
	{20, 5},
};
static arc arcs_46_7[1] = {
	{108, 5},
};
static arc arcs_46_8[1] = {
	{111, 5},
};
static arc arcs_46_9[1] = {
	{112, 5},
};
static state states_46[10] = {
	{7, arcs_46_0},
	{2, arcs_46_1},
	{2, arcs_46_2},
	{2, arcs_46_3},
	{1, arcs_46_4},
	{1, arcs_46_5},
	{1, arcs_46_6},
	{1, arcs_46_7},
	{1, arcs_46_8},
	{1, arcs_46_9},
};
static arc arcs_47_0[3] = {
	{19, 1},
	{107, 2},
	{116, 3},
};
static arc arcs_47_1[2] = {
	{9, 4},
	{20, 5},
};
static arc arcs_47_2[1] = {
	{115, 6},
};
static arc arcs_47_3[1] = {
	{16, 5},
};
static arc arcs_47_4[1] = {
	{20, 5},
};
static arc arcs_47_5[1] = {
	{0, 5},
};
static arc arcs_47_6[1] = {
	{108, 5},
};
static state states_47[7] = {
	{3, arcs_47_0},
	{2, arcs_47_1},
	{1, arcs_47_2},
	{1, arcs_47_3},
	{1, arcs_47_4},
	{1, arcs_47_5},
	{1, arcs_47_6},
};
static arc arcs_48_0[2] = {
	{38, 1},
	{13, 2},
};
static arc arcs_48_1[2] = {
	{13, 2},
	{0, 1},
};
static arc arcs_48_2[2] = {
	{38, 3},
	{0, 2},
};
static arc arcs_48_3[1] = {
	{0, 3},
};
static state states_48[4] = {
	{2, arcs_48_0},
	{2, arcs_48_1},
	{2, arcs_48_2},
	{1, arcs_48_3},
};
static arc arcs_49_0[1] = {
	{56, 1},
};
static arc arcs_49_1[2] = {
	{22, 2},
	{0, 1},
};
static arc arcs_49_2[2] = {
	{56, 1},
	{0, 2},
};
static state states_49[3] = {
	{1, arcs_49_0},
	{2, arcs_49_1},
	{2, arcs_49_2},
};
static arc arcs_50_0[1] = {
	{38, 1},
};
static arc arcs_50_1[2] = {
	{22, 2},
	{0, 1},
};
static arc arcs_50_2[2] = {
	{38, 1},
	{0, 2},
};
static state states_50[3] = {
	{1, arcs_50_0},
	{2, arcs_50_1},
	{2, arcs_50_2},
};
static arc arcs_51_0[1] = {
	{38, 1},
};
static arc arcs_51_1[1] = {
	{13, 2},
};
static arc arcs_51_2[1] = {
	{38, 3},
};
static arc arcs_51_3[2] = {
	{22, 4},
	{0, 3},
};
static arc arcs_51_4[2] = {
	{38, 1},
	{0, 4},
};
static state states_51[5] = {
	{1, arcs_51_0},
	{1, arcs_51_1},
	{1, arcs_51_2},
	{2, arcs_51_3},
	{2, arcs_51_4},
};
static arc arcs_52_0[1] = {
	{117, 1},
};
static arc arcs_52_1[1] = {
	{16, 2},
};
static arc arcs_52_2[2] = {
	{19, 3},
	{13, 4},
};
static arc arcs_52_3[1] = {
	{9, 5},
};
static arc arcs_52_4[1] = {
	{18, 6},
};
static arc arcs_52_5[1] = {
	{20, 7},
};
static arc arcs_52_6[1] = {
	{0, 6},
};
static arc arcs_52_7[1] = {
	{13, 4},
};
static state states_52[8] = {
	{1, arcs_52_0},
	{1, arcs_52_1},
	{2, arcs_52_2},
	{1, arcs_52_3},
	{1, arcs_52_4},
	{1, arcs_52_5},
	{1, arcs_52_6},
	{1, arcs_52_7},
};
static dfa dfas[53] = {
	{256, "single_input", 0, 3, states_0,
	 "\004\200\011\000\240\302\277\200\034\100\000\000\030\051\047"},
	{257, "file_input", 0, 2, states_1,
	 "\204\200\011\000\240\302\277\200\034\100\000\000\030\051\047"},
	{258, "expr_input", 0, 3, states_2,
	 "\000\000\011\000\000\000\000\000\000\100\000\000\030\051\007"},
	{259, "eval_input", 0, 3, states_3,
	 "\000\000\011\000\000\000\000\000\000\100\000\000\030\051\007"},
	{260, "exprfunc_input", 0, 5, states_4,
	 "\000\000\211\000\000\000\000\000\000\000\000\000\000\000\000"},
	{261, "funcdef", 0, 6, states_5,
	 "\000\200\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{262, "parameters", 0, 4, states_6,
	 "\000\000\010\000\000\000\000\000\000\000\000\000\000\000\000"},
	{263, "varargslist", 0, 5, states_7,
	 "\000\000\211\000\000\000\000\000\000\000\000\000\000\000\000"},
	{264, "fpdef", 0, 4, states_8,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\000\000\000"},
	{265, "fplist", 0, 3, states_9,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\000\000\000"},
	{266, "stmt", 0, 2, states_10,
	 "\000\200\011\000\240\302\277\200\034\100\000\000\030\051\047"},
	{267, "simple_stmt", 0, 4, states_11,
	 "\000\000\011\000\240\302\277\000\000\100\000\000\030\051\007"},
	{268, "small_stmt", 0, 2, states_12,
	 "\000\000\011\000\240\302\277\000\000\100\000\000\030\051\007"},
	{269, "expr_stmt", 0, 2, states_13,
	 "\000\000\011\000\000\000\000\000\000\100\000\000\030\051\007"},
	{270, "print_stmt", 0, 3, states_14,
	 "\000\000\000\000\040\000\000\000\000\000\000\000\000\000\000"},
	{271, "del_stmt", 0, 3, states_15,
	 "\000\000\000\000\200\000\000\000\000\000\000\000\000\000\000"},
	{272, "pass_stmt", 0, 2, states_16,
	 "\000\000\000\000\000\002\000\000\000\000\000\000\000\000\000"},
	{273, "flow_stmt", 0, 2, states_17,
	 "\000\000\000\000\000\300\003\000\000\000\000\000\000\000\000"},
	{274, "break_stmt", 0, 2, states_18,
	 "\000\000\000\000\000\100\000\000\000\000\000\000\000\000\000"},
	{275, "continue_stmt", 0, 2, states_19,
	 "\000\000\000\000\000\200\000\000\000\000\000\000\000\000\000"},
	{276, "return_stmt", 0, 3, states_20,
	 "\000\000\000\000\000\000\001\000\000\000\000\000\000\000\000"},
	{277, "raise_stmt", 0, 5, states_21,
	 "\000\000\000\000\000\000\002\000\000\000\000\000\000\000\000"},
	{278, "import_stmt", 0, 9, states_22,
	 "\000\000\000\000\000\000\014\000\000\000\000\000\000\000\000"},
	{279, "global_stmt", 0, 3, states_23,
	 "\000\000\000\000\000\000\020\000\000\000\000\000\000\000\000"},
	{280, "access_stmt", 0, 7, states_24,
	 "\000\000\000\000\000\000\040\000\000\000\000\000\000\000\000"},
	{281, "accesstype", 0, 2, states_25,
	 "\000\000\001\000\000\000\000\000\000\000\000\000\000\000\000"},
	{282, "exec_stmt", 0, 7, states_26,
	 "\000\000\000\000\000\000\200\000\000\000\000\000\000\000\000"},
	{283, "compound_stmt", 0, 2, states_27,
	 "\000\200\000\000\000\000\000\200\034\000\000\000\000\000\040"},
	{284, "if_stmt", 0, 8, states_28,
	 "\000\000\000\000\000\000\000\200\000\000\000\000\000\000\000"},
	{285, "while_stmt", 0, 8, states_29,
	 "\000\000\000\000\000\000\000\000\004\000\000\000\000\000\000"},
	{286, "for_stmt", 0, 10, states_30,
	 "\000\000\000\000\000\000\000\000\010\000\000\000\000\000\000"},
	{287, "try_stmt", 0, 10, states_31,
	 "\000\000\000\000\000\000\000\000\020\000\000\000\000\000\000"},
	{288, "except_clause", 0, 5, states_32,
	 "\000\000\000\000\000\000\000\000\200\000\000\000\000\000\000"},
	{289, "suite", 0, 5, states_33,
	 "\004\000\011\000\240\302\277\000\000\100\000\000\030\051\007"},
	{290, "test", 0, 2, states_34,
	 "\000\000\011\000\000\000\000\000\000\100\000\000\030\051\007"},
	{291, "and_test", 0, 2, states_35,
	 "\000\000\011\000\000\000\000\000\000\100\000\000\030\051\007"},
	{292, "not_test", 0, 3, states_36,
	 "\000\000\011\000\000\000\000\000\000\100\000\000\030\051\007"},
	{293, "comparison", 0, 2, states_37,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\030\051\007"},
	{294, "comp_op", 0, 4, states_38,
	 "\000\000\000\000\000\000\000\002\000\100\376\001\000\000\000"},
	{295, "expr", 0, 2, states_39,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\030\051\007"},
	{296, "xor_expr", 0, 2, states_40,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\030\051\007"},
	{297, "and_expr", 0, 2, states_41,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\030\051\007"},
	{298, "shift_expr", 0, 2, states_42,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\030\051\007"},
	{299, "arith_expr", 0, 2, states_43,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\030\051\007"},
	{300, "term", 0, 2, states_44,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\030\051\007"},
	{301, "factor", 0, 4, states_45,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\030\051\007"},
	{302, "atom", 0, 10, states_46,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\000\050\007"},
	{303, "trailer", 0, 7, states_47,
	 "\000\000\010\000\000\000\000\000\000\000\000\000\000\010\020"},
	{304, "subscript", 0, 4, states_48,
	 "\000\040\011\000\000\000\000\000\000\100\000\000\030\051\007"},
	{305, "exprlist", 0, 3, states_49,
	 "\000\000\011\000\000\000\000\000\000\000\000\000\030\051\007"},
	{306, "testlist", 0, 3, states_50,
	 "\000\000\011\000\000\000\000\000\000\100\000\000\030\051\007"},
	{307, "dictmaker", 0, 5, states_51,
	 "\000\000\011\000\000\000\000\000\000\100\000\000\030\051\007"},
	{308, "classdef", 0, 8, states_52,
	 "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\040"},
};
static label labels[118] = {
	{0, "EMPTY"},
	{256, 0},
	{4, 0},
	{267, 0},
	{283, 0},
	{257, 0},
	{266, 0},
	{0, 0},
	{258, 0},
	{306, 0},
	{259, 0},
	{260, 0},
	{263, 0},
	{11, 0},
	{261, 0},
	{1, "def"},
	{1, 0},
	{262, 0},
	{289, 0},
	{7, 0},
	{8, 0},
	{264, 0},
	{12, 0},
	{16, 0},
	{265, 0},
	{268, 0},
	{13, 0},
	{269, 0},
	{270, 0},
	{271, 0},
	{272, 0},
	{273, 0},
	{278, 0},
	{279, 0},
	{280, 0},
	{282, 0},
	{22, 0},
	{1, "print"},
	{290, 0},
	{1, "del"},
	{305, 0},
	{1, "pass"},
	{274, 0},
	{275, 0},
	{276, 0},
	{277, 0},
	{1, "break"},
	{1, "continue"},
	{1, "return"},
	{1, "raise"},
	{1, "import"},
	{1, "from"},
	{1, "global"},
	{1, "access"},
	{281, 0},
	{1, "exec"},
	{295, 0},
	{1, "in"},
	{284, 0},
	{285, 0},
	{286, 0},
	{287, 0},
	{308, 0},
	{1, "if"},
	{1, "elif"},
	{1, "else"},
	{1, "while"},
	{1, "for"},
	{1, "try"},
	{288, 0},
	{1, "finally"},
	{1, "except"},
	{5, 0},
	{6, 0},
	{291, 0},
	{1, "or"},
	{292, 0},
	{1, "and"},
	{1, "not"},
	{293, 0},
	{294, 0},
	{20, 0},
	{21, 0},
	{28, 0},
	{31, 0},
	{30, 0},
	{29, 0},
	{29, 0},
	{1, "is"},
	{296, 0},
	{18, 0},
	{297, 0},
	{33, 0},
	{298, 0},
	{19, 0},
	{299, 0},
	{34, 0},
	{35, 0},
	{300, 0},
	{14, 0},
	{15, 0},
	{301, 0},
	{17, 0},
	{24, 0},
	{32, 0},
	{302, 0},
	{303, 0},
	{9, 0},
	{10, 0},
	{26, 0},
	{307, 0},
	{27, 0},
	{25, 0},
	{2, 0},
	{3, 0},
	{304, 0},
	{23, 0},
	{1, "class"},
};
grammar gram = {
	53,
	dfas,
	{118, labels},
	256
};
