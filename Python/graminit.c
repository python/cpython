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
static arc arcs_6_0[2] = {
	{20, 1},
	{22, 2},
};
static arc arcs_6_1[2] = {
	{21, 3},
	{0, 1},
};
static arc arcs_6_2[1] = {
	{13, 4},
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
	{13, 1},
	{17, 2},
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
static arc arcs_11_0[8] = {
	{26, 1},
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
	{8, arcs_11_0},
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
	{22, 6},
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
static arc arcs_23_0[1] = {
	{51, 1},
};
static arc arcs_23_1[2] = {
	{22, 2},
	{13, 3},
};
static arc arcs_23_2[1] = {
	{15, 4},
};
static arc arcs_23_3[2] = {
	{21, 5},
	{15, 4},
};
static arc arcs_23_4[1] = {
	{52, 6},
};
static arc arcs_23_5[1] = {
	{13, 3},
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
	{13, 1},
};
static arc arcs_24_1[2] = {
	{13, 1},
	{0, 1},
};
static state states_24[2] = {
	{1, arcs_24_0},
	{2, arcs_24_1},
};
static arc arcs_25_0[6] = {
	{53, 1},
	{54, 1},
	{55, 1},
	{56, 1},
	{11, 1},
	{57, 1},
};
static arc arcs_25_1[1] = {
	{0, 1},
};
static state states_25[2] = {
	{6, arcs_25_0},
	{1, arcs_25_1},
};
static arc arcs_26_0[1] = {
	{58, 1},
};
static arc arcs_26_1[1] = {
	{37, 2},
};
static arc arcs_26_2[1] = {
	{15, 3},
};
static arc arcs_26_3[1] = {
	{16, 4},
};
static arc arcs_26_4[3] = {
	{59, 1},
	{60, 5},
	{0, 4},
};
static arc arcs_26_5[1] = {
	{15, 6},
};
static arc arcs_26_6[1] = {
	{16, 7},
};
static arc arcs_26_7[1] = {
	{0, 7},
};
static state states_26[8] = {
	{1, arcs_26_0},
	{1, arcs_26_1},
	{1, arcs_26_2},
	{1, arcs_26_3},
	{3, arcs_26_4},
	{1, arcs_26_5},
	{1, arcs_26_6},
	{1, arcs_26_7},
};
static arc arcs_27_0[1] = {
	{61, 1},
};
static arc arcs_27_1[1] = {
	{37, 2},
};
static arc arcs_27_2[1] = {
	{15, 3},
};
static arc arcs_27_3[1] = {
	{16, 4},
};
static arc arcs_27_4[2] = {
	{60, 5},
	{0, 4},
};
static arc arcs_27_5[1] = {
	{15, 6},
};
static arc arcs_27_6[1] = {
	{16, 7},
};
static arc arcs_27_7[1] = {
	{0, 7},
};
static state states_27[8] = {
	{1, arcs_27_0},
	{1, arcs_27_1},
	{1, arcs_27_2},
	{1, arcs_27_3},
	{2, arcs_27_4},
	{1, arcs_27_5},
	{1, arcs_27_6},
	{1, arcs_27_7},
};
static arc arcs_28_0[1] = {
	{62, 1},
};
static arc arcs_28_1[1] = {
	{34, 2},
};
static arc arcs_28_2[1] = {
	{63, 3},
};
static arc arcs_28_3[1] = {
	{9, 4},
};
static arc arcs_28_4[1] = {
	{15, 5},
};
static arc arcs_28_5[1] = {
	{16, 6},
};
static arc arcs_28_6[2] = {
	{60, 7},
	{0, 6},
};
static arc arcs_28_7[1] = {
	{15, 8},
};
static arc arcs_28_8[1] = {
	{16, 9},
};
static arc arcs_28_9[1] = {
	{0, 9},
};
static state states_28[10] = {
	{1, arcs_28_0},
	{1, arcs_28_1},
	{1, arcs_28_2},
	{1, arcs_28_3},
	{1, arcs_28_4},
	{1, arcs_28_5},
	{2, arcs_28_6},
	{1, arcs_28_7},
	{1, arcs_28_8},
	{1, arcs_28_9},
};
static arc arcs_29_0[1] = {
	{64, 1},
};
static arc arcs_29_1[1] = {
	{15, 2},
};
static arc arcs_29_2[1] = {
	{16, 3},
};
static arc arcs_29_3[2] = {
	{65, 4},
	{66, 5},
};
static arc arcs_29_4[1] = {
	{15, 6},
};
static arc arcs_29_5[1] = {
	{15, 7},
};
static arc arcs_29_6[1] = {
	{16, 8},
};
static arc arcs_29_7[1] = {
	{16, 9},
};
static arc arcs_29_8[2] = {
	{65, 4},
	{0, 8},
};
static arc arcs_29_9[1] = {
	{0, 9},
};
static state states_29[10] = {
	{1, arcs_29_0},
	{1, arcs_29_1},
	{1, arcs_29_2},
	{2, arcs_29_3},
	{1, arcs_29_4},
	{1, arcs_29_5},
	{1, arcs_29_6},
	{1, arcs_29_7},
	{2, arcs_29_8},
	{1, arcs_29_9},
};
static arc arcs_30_0[1] = {
	{67, 1},
};
static arc arcs_30_1[2] = {
	{37, 2},
	{0, 1},
};
static arc arcs_30_2[2] = {
	{21, 3},
	{0, 2},
};
static arc arcs_30_3[1] = {
	{37, 4},
};
static arc arcs_30_4[1] = {
	{0, 4},
};
static state states_30[5] = {
	{1, arcs_30_0},
	{2, arcs_30_1},
	{2, arcs_30_2},
	{1, arcs_30_3},
	{1, arcs_30_4},
};
static arc arcs_31_0[2] = {
	{3, 1},
	{2, 2},
};
static arc arcs_31_1[1] = {
	{0, 1},
};
static arc arcs_31_2[1] = {
	{68, 3},
};
static arc arcs_31_3[1] = {
	{6, 4},
};
static arc arcs_31_4[2] = {
	{6, 4},
	{69, 1},
};
static state states_31[5] = {
	{2, arcs_31_0},
	{1, arcs_31_1},
	{1, arcs_31_2},
	{1, arcs_31_3},
	{2, arcs_31_4},
};
static arc arcs_32_0[1] = {
	{70, 1},
};
static arc arcs_32_1[2] = {
	{71, 0},
	{0, 1},
};
static state states_32[2] = {
	{1, arcs_32_0},
	{2, arcs_32_1},
};
static arc arcs_33_0[1] = {
	{72, 1},
};
static arc arcs_33_1[2] = {
	{73, 0},
	{0, 1},
};
static state states_33[2] = {
	{1, arcs_33_0},
	{2, arcs_33_1},
};
static arc arcs_34_0[2] = {
	{74, 1},
	{75, 2},
};
static arc arcs_34_1[1] = {
	{72, 2},
};
static arc arcs_34_2[1] = {
	{0, 2},
};
static state states_34[3] = {
	{2, arcs_34_0},
	{1, arcs_34_1},
	{1, arcs_34_2},
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
static arc arcs_36_0[10] = {
	{78, 1},
	{79, 1},
	{80, 1},
	{81, 1},
	{82, 1},
	{83, 1},
	{84, 1},
	{63, 1},
	{74, 2},
	{85, 3},
};
static arc arcs_36_1[1] = {
	{0, 1},
};
static arc arcs_36_2[1] = {
	{63, 1},
};
static arc arcs_36_3[2] = {
	{74, 1},
	{0, 3},
};
static state states_36[4] = {
	{10, arcs_36_0},
	{1, arcs_36_1},
	{1, arcs_36_2},
	{2, arcs_36_3},
};
static arc arcs_37_0[1] = {
	{86, 1},
};
static arc arcs_37_1[2] = {
	{87, 0},
	{0, 1},
};
static state states_37[2] = {
	{1, arcs_37_0},
	{2, arcs_37_1},
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
static arc arcs_40_1[3] = {
	{93, 0},
	{94, 0},
	{0, 1},
};
static state states_40[2] = {
	{1, arcs_40_0},
	{3, arcs_40_1},
};
static arc arcs_41_0[1] = {
	{95, 1},
};
static arc arcs_41_1[3] = {
	{96, 0},
	{97, 0},
	{0, 1},
};
static state states_41[2] = {
	{1, arcs_41_0},
	{3, arcs_41_1},
};
static arc arcs_42_0[1] = {
	{98, 1},
};
static arc arcs_42_1[4] = {
	{22, 0},
	{99, 0},
	{100, 0},
	{0, 1},
};
static state states_42[2] = {
	{1, arcs_42_0},
	{4, arcs_42_1},
};
static arc arcs_43_0[4] = {
	{96, 1},
	{97, 1},
	{101, 1},
	{102, 2},
};
static arc arcs_43_1[1] = {
	{98, 3},
};
static arc arcs_43_2[2] = {
	{103, 2},
	{0, 2},
};
static arc arcs_43_3[1] = {
	{0, 3},
};
static state states_43[4] = {
	{4, arcs_43_0},
	{1, arcs_43_1},
	{2, arcs_43_2},
	{1, arcs_43_3},
};
static arc arcs_44_0[7] = {
	{17, 1},
	{104, 2},
	{106, 3},
	{109, 4},
	{13, 5},
	{110, 5},
	{111, 5},
};
static arc arcs_44_1[2] = {
	{9, 6},
	{19, 5},
};
static arc arcs_44_2[2] = {
	{9, 7},
	{105, 5},
};
static arc arcs_44_3[2] = {
	{107, 8},
	{108, 5},
};
static arc arcs_44_4[1] = {
	{9, 9},
};
static arc arcs_44_5[1] = {
	{0, 5},
};
static arc arcs_44_6[1] = {
	{19, 5},
};
static arc arcs_44_7[1] = {
	{105, 5},
};
static arc arcs_44_8[1] = {
	{108, 5},
};
static arc arcs_44_9[1] = {
	{109, 5},
};
static state states_44[10] = {
	{7, arcs_44_0},
	{2, arcs_44_1},
	{2, arcs_44_2},
	{2, arcs_44_3},
	{1, arcs_44_4},
	{1, arcs_44_5},
	{1, arcs_44_6},
	{1, arcs_44_7},
	{1, arcs_44_8},
	{1, arcs_44_9},
};
static arc arcs_45_0[3] = {
	{17, 1},
	{104, 2},
	{113, 3},
};
static arc arcs_45_1[2] = {
	{9, 4},
	{19, 5},
};
static arc arcs_45_2[1] = {
	{112, 6},
};
static arc arcs_45_3[1] = {
	{13, 5},
};
static arc arcs_45_4[1] = {
	{19, 5},
};
static arc arcs_45_5[1] = {
	{0, 5},
};
static arc arcs_45_6[1] = {
	{105, 5},
};
static state states_45[7] = {
	{3, arcs_45_0},
	{2, arcs_45_1},
	{1, arcs_45_2},
	{1, arcs_45_3},
	{1, arcs_45_4},
	{1, arcs_45_5},
	{1, arcs_45_6},
};
static arc arcs_46_0[2] = {
	{37, 1},
	{15, 2},
};
static arc arcs_46_1[2] = {
	{15, 2},
	{0, 1},
};
static arc arcs_46_2[2] = {
	{37, 3},
	{0, 2},
};
static arc arcs_46_3[1] = {
	{0, 3},
};
static state states_46[4] = {
	{2, arcs_46_0},
	{2, arcs_46_1},
	{2, arcs_46_2},
	{1, arcs_46_3},
};
static arc arcs_47_0[1] = {
	{76, 1},
};
static arc arcs_47_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_47_2[2] = {
	{76, 1},
	{0, 2},
};
static state states_47[3] = {
	{1, arcs_47_0},
	{2, arcs_47_1},
	{2, arcs_47_2},
};
static arc arcs_48_0[1] = {
	{37, 1},
};
static arc arcs_48_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_48_2[2] = {
	{37, 1},
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
static arc arcs_49_1[1] = {
	{15, 2},
};
static arc arcs_49_2[1] = {
	{37, 3},
};
static arc arcs_49_3[2] = {
	{21, 4},
	{0, 3},
};
static arc arcs_49_4[2] = {
	{37, 1},
	{0, 4},
};
static state states_49[5] = {
	{1, arcs_49_0},
	{1, arcs_49_1},
	{1, arcs_49_2},
	{2, arcs_49_3},
	{2, arcs_49_4},
};
static arc arcs_50_0[1] = {
	{114, 1},
};
static arc arcs_50_1[1] = {
	{13, 2},
};
static arc arcs_50_2[2] = {
	{17, 3},
	{15, 4},
};
static arc arcs_50_3[1] = {
	{9, 5},
};
static arc arcs_50_4[1] = {
	{16, 6},
};
static arc arcs_50_5[1] = {
	{19, 7},
};
static arc arcs_50_6[1] = {
	{0, 6},
};
static arc arcs_50_7[1] = {
	{15, 4},
};
static state states_50[8] = {
	{1, arcs_50_0},
	{1, arcs_50_1},
	{2, arcs_50_2},
	{1, arcs_50_3},
	{1, arcs_50_4},
	{1, arcs_50_5},
	{1, arcs_50_6},
	{1, arcs_50_7},
};
static dfa dfas[51] = {
	{256, "single_input", 0, 3, states_0,
	 "\004\060\002\000\320\360\017\144\001\000\000\000\043\345\004"},
	{257, "file_input", 0, 2, states_1,
	 "\204\060\002\000\320\360\017\144\001\000\000\000\043\345\004"},
	{258, "expr_input", 0, 3, states_2,
	 "\000\040\002\000\000\000\000\000\000\004\000\000\043\345\000"},
	{259, "eval_input", 0, 3, states_3,
	 "\000\040\002\000\000\000\000\000\000\004\000\000\043\345\000"},
	{260, "funcdef", 0, 6, states_4,
	 "\000\020\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{261, "parameters", 0, 4, states_5,
	 "\000\000\002\000\000\000\000\000\000\000\000\000\000\000\000"},
	{262, "varargslist", 0, 5, states_6,
	 "\000\040\102\000\000\000\000\000\000\000\000\000\000\000\000"},
	{263, "fpdef", 0, 4, states_7,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\000\000\000"},
	{264, "fplist", 0, 3, states_8,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\000\000\000"},
	{265, "stmt", 0, 2, states_9,
	 "\000\060\002\000\320\360\017\144\001\000\000\000\043\345\004"},
	{266, "simple_stmt", 0, 4, states_10,
	 "\000\040\002\000\320\360\017\000\000\000\000\000\043\345\000"},
	{267, "small_stmt", 0, 2, states_11,
	 "\000\040\002\000\320\360\017\000\000\000\000\000\043\345\000"},
	{268, "expr_stmt", 0, 2, states_12,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
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
	{279, "access_stmt", 0, 7, states_23,
	 "\000\000\000\000\000\000\010\000\000\000\000\000\000\000\000"},
	{280, "accesstype", 0, 2, states_24,
	 "\000\040\000\000\000\000\000\000\000\000\000\000\000\000\000"},
	{281, "compound_stmt", 0, 2, states_25,
	 "\000\020\000\000\000\000\000\144\001\000\000\000\000\000\004"},
	{282, "if_stmt", 0, 8, states_26,
	 "\000\000\000\000\000\000\000\004\000\000\000\000\000\000\000"},
	{283, "while_stmt", 0, 8, states_27,
	 "\000\000\000\000\000\000\000\040\000\000\000\000\000\000\000"},
	{284, "for_stmt", 0, 10, states_28,
	 "\000\000\000\000\000\000\000\100\000\000\000\000\000\000\000"},
	{285, "try_stmt", 0, 10, states_29,
	 "\000\000\000\000\000\000\000\000\001\000\000\000\000\000\000"},
	{286, "except_clause", 0, 5, states_30,
	 "\000\000\000\000\000\000\000\000\010\000\000\000\000\000\000"},
	{287, "suite", 0, 5, states_31,
	 "\004\040\002\000\320\360\017\000\000\000\000\000\043\345\000"},
	{288, "test", 0, 2, states_32,
	 "\000\040\002\000\000\000\000\000\000\004\000\000\043\345\000"},
	{289, "and_test", 0, 2, states_33,
	 "\000\040\002\000\000\000\000\000\000\004\000\000\043\345\000"},
	{290, "not_test", 0, 3, states_34,
	 "\000\040\002\000\000\000\000\000\000\004\000\000\043\345\000"},
	{291, "comparison", 0, 2, states_35,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
	{292, "comp_op", 0, 4, states_36,
	 "\000\000\000\000\000\000\000\200\000\304\077\000\000\000\000"},
	{293, "expr", 0, 2, states_37,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
	{294, "xor_expr", 0, 2, states_38,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
	{295, "and_expr", 0, 2, states_39,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
	{296, "shift_expr", 0, 2, states_40,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
	{297, "arith_expr", 0, 2, states_41,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
	{298, "term", 0, 2, states_42,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
	{299, "factor", 0, 4, states_43,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
	{300, "atom", 0, 10, states_44,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\000\345\000"},
	{301, "trailer", 0, 7, states_45,
	 "\000\000\002\000\000\000\000\000\000\000\000\000\000\001\002"},
	{302, "subscript", 0, 4, states_46,
	 "\000\240\002\000\000\000\000\000\000\004\000\000\043\345\000"},
	{303, "exprlist", 0, 3, states_47,
	 "\000\040\002\000\000\000\000\000\000\000\000\000\043\345\000"},
	{304, "testlist", 0, 3, states_48,
	 "\000\040\002\000\000\000\000\000\000\004\000\000\043\345\000"},
	{305, "dictmaker", 0, 5, states_49,
	 "\000\040\002\000\000\000\000\000\000\004\000\000\043\345\000"},
	{306, "classdef", 0, 8, states_50,
	 "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\004"},
};
static label labels[115] = {
	{0, "EMPTY"},
	{256, 0},
	{4, 0},
	{266, 0},
	{281, 0},
	{257, 0},
	{265, 0},
	{0, 0},
	{258, 0},
	{304, 0},
	{259, 0},
	{260, 0},
	{1, "def"},
	{1, 0},
	{261, 0},
	{11, 0},
	{287, 0},
	{7, 0},
	{262, 0},
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
	{303, 0},
	{22, 0},
	{1, "print"},
	{288, 0},
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
	{1, "access"},
	{280, 0},
	{282, 0},
	{283, 0},
	{284, 0},
	{285, 0},
	{306, 0},
	{1, "if"},
	{1, "elif"},
	{1, "else"},
	{1, "while"},
	{1, "for"},
	{1, "in"},
	{1, "try"},
	{286, 0},
	{1, "finally"},
	{1, "except"},
	{5, 0},
	{6, 0},
	{289, 0},
	{1, "or"},
	{290, 0},
	{1, "and"},
	{1, "not"},
	{291, 0},
	{293, 0},
	{292, 0},
	{20, 0},
	{21, 0},
	{28, 0},
	{31, 0},
	{30, 0},
	{29, 0},
	{29, 0},
	{1, "is"},
	{294, 0},
	{18, 0},
	{295, 0},
	{33, 0},
	{296, 0},
	{19, 0},
	{297, 0},
	{34, 0},
	{35, 0},
	{298, 0},
	{14, 0},
	{15, 0},
	{299, 0},
	{17, 0},
	{24, 0},
	{32, 0},
	{300, 0},
	{301, 0},
	{9, 0},
	{10, 0},
	{26, 0},
	{305, 0},
	{27, 0},
	{25, 0},
	{2, 0},
	{3, 0},
	{302, 0},
	{23, 0},
	{1, "class"},
};
grammar gram = {
	51,
	dfas,
	{115, labels},
	256
};
