#include "PROTO.h"
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
static arc arcs_3_1[1] = {
	{7, 2},
};
static arc arcs_3_2[1] = {
	{0, 2},
};
static state states_3[3] = {
	{1, arcs_3_0},
	{1, arcs_3_1},
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
static arc arcs_6_0[1] = {
	{20, 1},
};
static arc arcs_6_1[2] = {
	{21, 0},
	{0, 1},
};
static state states_6[2] = {
	{1, arcs_6_0},
	{2, arcs_6_1},
};
static arc arcs_7_0[2] = {
	{13, 1},
	{17, 2},
};
static arc arcs_7_1[1] = {
	{0, 1},
};
static arc arcs_7_2[1] = {
	{18, 3},
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
static arc arcs_9_0[7] = {
	{22, 1},
	{23, 1},
	{24, 1},
	{25, 1},
	{26, 1},
	{27, 1},
	{28, 1},
};
static arc arcs_9_1[1] = {
	{0, 1},
};
static state states_9[2] = {
	{7, arcs_9_0},
	{1, arcs_9_1},
};
static arc arcs_10_0[1] = {
	{29, 1},
};
static arc arcs_10_1[2] = {
	{30, 0},
	{2, 2},
};
static arc arcs_10_2[1] = {
	{0, 2},
};
static state states_10[3] = {
	{1, arcs_10_0},
	{2, arcs_10_1},
	{1, arcs_10_2},
};
static arc arcs_11_0[1] = {
	{31, 1},
};
static arc arcs_11_1[2] = {
	{32, 2},
	{2, 3},
};
static arc arcs_11_2[2] = {
	{21, 1},
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
static arc arcs_12_0[1] = {
	{33, 1},
};
static arc arcs_12_1[1] = {
	{29, 2},
};
static arc arcs_12_2[1] = {
	{2, 3},
};
static arc arcs_12_3[1] = {
	{0, 3},
};
static state states_12[4] = {
	{1, arcs_12_0},
	{1, arcs_12_1},
	{1, arcs_12_2},
	{1, arcs_12_3},
};
static arc arcs_13_0[1] = {
	{34, 1},
};
static arc arcs_13_1[2] = {
	{35, 2},
	{2, 3},
};
static arc arcs_13_2[1] = {
	{2, 3},
};
static arc arcs_13_3[1] = {
	{0, 3},
};
static state states_13[4] = {
	{1, arcs_13_0},
	{2, arcs_13_1},
	{1, arcs_13_2},
	{1, arcs_13_3},
};
static arc arcs_14_0[1] = {
	{36, 1},
};
static arc arcs_14_1[1] = {
	{2, 2},
};
static arc arcs_14_2[1] = {
	{0, 2},
};
static state states_14[3] = {
	{1, arcs_14_0},
	{1, arcs_14_1},
	{1, arcs_14_2},
};
static arc arcs_15_0[3] = {
	{37, 1},
	{38, 1},
	{39, 1},
};
static arc arcs_15_1[1] = {
	{0, 1},
};
static state states_15[2] = {
	{3, arcs_15_0},
	{1, arcs_15_1},
};
static arc arcs_16_0[1] = {
	{40, 1},
};
static arc arcs_16_1[1] = {
	{2, 2},
};
static arc arcs_16_2[1] = {
	{0, 2},
};
static state states_16[3] = {
	{1, arcs_16_0},
	{1, arcs_16_1},
	{1, arcs_16_2},
};
static arc arcs_17_0[1] = {
	{41, 1},
};
static arc arcs_17_1[2] = {
	{9, 2},
	{2, 3},
};
static arc arcs_17_2[1] = {
	{2, 3},
};
static arc arcs_17_3[1] = {
	{0, 3},
};
static state states_17[4] = {
	{1, arcs_17_0},
	{2, arcs_17_1},
	{1, arcs_17_2},
	{1, arcs_17_3},
};
static arc arcs_18_0[1] = {
	{42, 1},
};
static arc arcs_18_1[1] = {
	{35, 2},
};
static arc arcs_18_2[2] = {
	{21, 3},
	{2, 4},
};
static arc arcs_18_3[1] = {
	{35, 5},
};
static arc arcs_18_4[1] = {
	{0, 4},
};
static arc arcs_18_5[1] = {
	{2, 4},
};
static state states_18[6] = {
	{1, arcs_18_0},
	{1, arcs_18_1},
	{2, arcs_18_2},
	{1, arcs_18_3},
	{1, arcs_18_4},
	{1, arcs_18_5},
};
static arc arcs_19_0[2] = {
	{43, 1},
	{44, 2},
};
static arc arcs_19_1[1] = {
	{13, 3},
};
static arc arcs_19_2[1] = {
	{13, 4},
};
static arc arcs_19_3[2] = {
	{21, 1},
	{2, 5},
};
static arc arcs_19_4[1] = {
	{43, 6},
};
static arc arcs_19_5[1] = {
	{0, 5},
};
static arc arcs_19_6[2] = {
	{45, 7},
	{13, 8},
};
static arc arcs_19_7[1] = {
	{2, 5},
};
static arc arcs_19_8[2] = {
	{21, 9},
	{2, 5},
};
static arc arcs_19_9[1] = {
	{13, 8},
};
static state states_19[10] = {
	{2, arcs_19_0},
	{1, arcs_19_1},
	{1, arcs_19_2},
	{2, arcs_19_3},
	{1, arcs_19_4},
	{1, arcs_19_5},
	{2, arcs_19_6},
	{1, arcs_19_7},
	{2, arcs_19_8},
	{1, arcs_19_9},
};
static arc arcs_20_0[6] = {
	{46, 1},
	{47, 1},
	{48, 1},
	{49, 1},
	{11, 1},
	{50, 1},
};
static arc arcs_20_1[1] = {
	{0, 1},
};
static state states_20[2] = {
	{6, arcs_20_0},
	{1, arcs_20_1},
};
static arc arcs_21_0[1] = {
	{51, 1},
};
static arc arcs_21_1[1] = {
	{32, 2},
};
static arc arcs_21_2[1] = {
	{15, 3},
};
static arc arcs_21_3[1] = {
	{16, 4},
};
static arc arcs_21_4[3] = {
	{52, 1},
	{53, 5},
	{0, 4},
};
static arc arcs_21_5[1] = {
	{15, 6},
};
static arc arcs_21_6[1] = {
	{16, 7},
};
static arc arcs_21_7[1] = {
	{0, 7},
};
static state states_21[8] = {
	{1, arcs_21_0},
	{1, arcs_21_1},
	{1, arcs_21_2},
	{1, arcs_21_3},
	{3, arcs_21_4},
	{1, arcs_21_5},
	{1, arcs_21_6},
	{1, arcs_21_7},
};
static arc arcs_22_0[1] = {
	{54, 1},
};
static arc arcs_22_1[1] = {
	{32, 2},
};
static arc arcs_22_2[1] = {
	{15, 3},
};
static arc arcs_22_3[1] = {
	{16, 4},
};
static arc arcs_22_4[2] = {
	{53, 5},
	{0, 4},
};
static arc arcs_22_5[1] = {
	{15, 6},
};
static arc arcs_22_6[1] = {
	{16, 7},
};
static arc arcs_22_7[1] = {
	{0, 7},
};
static state states_22[8] = {
	{1, arcs_22_0},
	{1, arcs_22_1},
	{1, arcs_22_2},
	{1, arcs_22_3},
	{2, arcs_22_4},
	{1, arcs_22_5},
	{1, arcs_22_6},
	{1, arcs_22_7},
};
static arc arcs_23_0[1] = {
	{55, 1},
};
static arc arcs_23_1[1] = {
	{29, 2},
};
static arc arcs_23_2[1] = {
	{56, 3},
};
static arc arcs_23_3[1] = {
	{29, 4},
};
static arc arcs_23_4[1] = {
	{15, 5},
};
static arc arcs_23_5[1] = {
	{16, 6},
};
static arc arcs_23_6[2] = {
	{53, 7},
	{0, 6},
};
static arc arcs_23_7[1] = {
	{15, 8},
};
static arc arcs_23_8[1] = {
	{16, 9},
};
static arc arcs_23_9[1] = {
	{0, 9},
};
static state states_23[10] = {
	{1, arcs_23_0},
	{1, arcs_23_1},
	{1, arcs_23_2},
	{1, arcs_23_3},
	{1, arcs_23_4},
	{1, arcs_23_5},
	{2, arcs_23_6},
	{1, arcs_23_7},
	{1, arcs_23_8},
	{1, arcs_23_9},
};
static arc arcs_24_0[1] = {
	{57, 1},
};
static arc arcs_24_1[1] = {
	{15, 2},
};
static arc arcs_24_2[1] = {
	{16, 3},
};
static arc arcs_24_3[3] = {
	{58, 1},
	{59, 4},
	{0, 3},
};
static arc arcs_24_4[1] = {
	{15, 5},
};
static arc arcs_24_5[1] = {
	{16, 6},
};
static arc arcs_24_6[1] = {
	{0, 6},
};
static state states_24[7] = {
	{1, arcs_24_0},
	{1, arcs_24_1},
	{1, arcs_24_2},
	{3, arcs_24_3},
	{1, arcs_24_4},
	{1, arcs_24_5},
	{1, arcs_24_6},
};
static arc arcs_25_0[1] = {
	{60, 1},
};
static arc arcs_25_1[2] = {
	{35, 2},
	{0, 1},
};
static arc arcs_25_2[2] = {
	{21, 3},
	{0, 2},
};
static arc arcs_25_3[1] = {
	{35, 4},
};
static arc arcs_25_4[1] = {
	{0, 4},
};
static state states_25[5] = {
	{1, arcs_25_0},
	{2, arcs_25_1},
	{2, arcs_25_2},
	{1, arcs_25_3},
	{1, arcs_25_4},
};
static arc arcs_26_0[2] = {
	{3, 1},
	{2, 2},
};
static arc arcs_26_1[1] = {
	{0, 1},
};
static arc arcs_26_2[1] = {
	{61, 3},
};
static arc arcs_26_3[2] = {
	{2, 3},
	{6, 4},
};
static arc arcs_26_4[3] = {
	{6, 4},
	{2, 4},
	{62, 1},
};
static state states_26[5] = {
	{2, arcs_26_0},
	{1, arcs_26_1},
	{1, arcs_26_2},
	{2, arcs_26_3},
	{3, arcs_26_4},
};
static arc arcs_27_0[1] = {
	{63, 1},
};
static arc arcs_27_1[2] = {
	{64, 0},
	{0, 1},
};
static state states_27[2] = {
	{1, arcs_27_0},
	{2, arcs_27_1},
};
static arc arcs_28_0[1] = {
	{65, 1},
};
static arc arcs_28_1[2] = {
	{66, 0},
	{0, 1},
};
static state states_28[2] = {
	{1, arcs_28_0},
	{2, arcs_28_1},
};
static arc arcs_29_0[2] = {
	{67, 1},
	{68, 2},
};
static arc arcs_29_1[1] = {
	{65, 2},
};
static arc arcs_29_2[1] = {
	{0, 2},
};
static state states_29[3] = {
	{2, arcs_29_0},
	{1, arcs_29_1},
	{1, arcs_29_2},
};
static arc arcs_30_0[1] = {
	{35, 1},
};
static arc arcs_30_1[2] = {
	{69, 0},
	{0, 1},
};
static state states_30[2] = {
	{1, arcs_30_0},
	{2, arcs_30_1},
};
static arc arcs_31_0[6] = {
	{70, 1},
	{71, 2},
	{30, 3},
	{56, 3},
	{67, 4},
	{72, 5},
};
static arc arcs_31_1[3] = {
	{30, 3},
	{71, 3},
	{0, 1},
};
static arc arcs_31_2[2] = {
	{30, 3},
	{0, 2},
};
static arc arcs_31_3[1] = {
	{0, 3},
};
static arc arcs_31_4[1] = {
	{56, 3},
};
static arc arcs_31_5[2] = {
	{67, 3},
	{0, 5},
};
static state states_31[6] = {
	{6, arcs_31_0},
	{3, arcs_31_1},
	{2, arcs_31_2},
	{1, arcs_31_3},
	{1, arcs_31_4},
	{2, arcs_31_5},
};
static arc arcs_32_0[1] = {
	{73, 1},
};
static arc arcs_32_1[3] = {
	{74, 0},
	{75, 0},
	{0, 1},
};
static state states_32[2] = {
	{1, arcs_32_0},
	{3, arcs_32_1},
};
static arc arcs_33_0[1] = {
	{76, 1},
};
static arc arcs_33_1[4] = {
	{45, 0},
	{77, 0},
	{78, 0},
	{0, 1},
};
static state states_33[2] = {
	{1, arcs_33_0},
	{4, arcs_33_1},
};
static arc arcs_34_0[3] = {
	{74, 1},
	{75, 1},
	{79, 2},
};
static arc arcs_34_1[1] = {
	{76, 3},
};
static arc arcs_34_2[2] = {
	{80, 2},
	{0, 2},
};
static arc arcs_34_3[1] = {
	{0, 3},
};
static state states_34[4] = {
	{3, arcs_34_0},
	{1, arcs_34_1},
	{2, arcs_34_2},
	{1, arcs_34_3},
};
static arc arcs_35_0[7] = {
	{17, 1},
	{81, 2},
	{83, 3},
	{85, 4},
	{13, 5},
	{86, 5},
	{87, 5},
};
static arc arcs_35_1[2] = {
	{9, 6},
	{19, 5},
};
static arc arcs_35_2[2] = {
	{9, 7},
	{82, 5},
};
static arc arcs_35_3[1] = {
	{84, 5},
};
static arc arcs_35_4[1] = {
	{9, 8},
};
static arc arcs_35_5[1] = {
	{0, 5},
};
static arc arcs_35_6[1] = {
	{19, 5},
};
static arc arcs_35_7[1] = {
	{82, 5},
};
static arc arcs_35_8[1] = {
	{85, 5},
};
static state states_35[9] = {
	{7, arcs_35_0},
	{2, arcs_35_1},
	{2, arcs_35_2},
	{1, arcs_35_3},
	{1, arcs_35_4},
	{1, arcs_35_5},
	{1, arcs_35_6},
	{1, arcs_35_7},
	{1, arcs_35_8},
};
static arc arcs_36_0[3] = {
	{17, 1},
	{81, 2},
	{89, 3},
};
static arc arcs_36_1[2] = {
	{29, 4},
	{19, 5},
};
static arc arcs_36_2[1] = {
	{88, 6},
};
static arc arcs_36_3[1] = {
	{13, 5},
};
static arc arcs_36_4[1] = {
	{19, 5},
};
static arc arcs_36_5[1] = {
	{0, 5},
};
static arc arcs_36_6[1] = {
	{82, 5},
};
static state states_36[7] = {
	{3, arcs_36_0},
	{2, arcs_36_1},
	{1, arcs_36_2},
	{1, arcs_36_3},
	{1, arcs_36_4},
	{1, arcs_36_5},
	{1, arcs_36_6},
};
static arc arcs_37_0[2] = {
	{35, 1},
	{15, 2},
};
static arc arcs_37_1[2] = {
	{15, 2},
	{0, 1},
};
static arc arcs_37_2[2] = {
	{35, 3},
	{0, 2},
};
static arc arcs_37_3[1] = {
	{0, 3},
};
static state states_37[4] = {
	{2, arcs_37_0},
	{2, arcs_37_1},
	{2, arcs_37_2},
	{1, arcs_37_3},
};
static arc arcs_38_0[1] = {
	{35, 1},
};
static arc arcs_38_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_38_2[2] = {
	{35, 1},
	{0, 2},
};
static state states_38[3] = {
	{1, arcs_38_0},
	{2, arcs_38_1},
	{2, arcs_38_2},
};
static arc arcs_39_0[1] = {
	{32, 1},
};
static arc arcs_39_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_39_2[2] = {
	{32, 1},
	{0, 2},
};
static state states_39[3] = {
	{1, arcs_39_0},
	{2, arcs_39_1},
	{2, arcs_39_2},
};
static arc arcs_40_0[1] = {
	{90, 1},
};
static arc arcs_40_1[1] = {
	{13, 2},
};
static arc arcs_40_2[1] = {
	{14, 3},
};
static arc arcs_40_3[2] = {
	{30, 4},
	{15, 5},
};
static arc arcs_40_4[1] = {
	{91, 6},
};
static arc arcs_40_5[1] = {
	{16, 7},
};
static arc arcs_40_6[1] = {
	{15, 5},
};
static arc arcs_40_7[1] = {
	{0, 7},
};
static state states_40[8] = {
	{1, arcs_40_0},
	{1, arcs_40_1},
	{1, arcs_40_2},
	{2, arcs_40_3},
	{1, arcs_40_4},
	{1, arcs_40_5},
	{1, arcs_40_6},
	{1, arcs_40_7},
};
static arc arcs_41_0[1] = {
	{79, 1},
};
static arc arcs_41_1[1] = {
	{92, 2},
};
static arc arcs_41_2[2] = {
	{21, 0},
	{0, 2},
};
static state states_41[3] = {
	{1, arcs_41_0},
	{1, arcs_41_1},
	{2, arcs_41_2},
};
static arc arcs_42_0[1] = {
	{17, 1},
};
static arc arcs_42_1[2] = {
	{9, 2},
	{19, 3},
};
static arc arcs_42_2[1] = {
	{19, 3},
};
static arc arcs_42_3[1] = {
	{0, 3},
};
static state states_42[4] = {
	{1, arcs_42_0},
	{2, arcs_42_1},
	{1, arcs_42_2},
	{1, arcs_42_3},
};
static dfa dfas[43] = {
	{256, "single_input", 0, 3, states_0,
	 "\004\060\002\200\026\037\310\002\000\014\352\004"},
	{257, "file_input", 0, 2, states_1,
	 "\204\060\002\200\026\037\310\002\000\014\352\004"},
	{258, "expr_input", 0, 3, states_2,
	 "\000\040\002\000\000\000\000\000\010\014\352\000"},
	{259, "eval_input", 0, 3, states_3,
	 "\000\040\002\000\000\000\000\000\010\014\352\000"},
	{260, "funcdef", 0, 6, states_4,
	 "\000\020\000\000\000\000\000\000\000\000\000\000"},
	{261, "parameters", 0, 4, states_5,
	 "\000\000\002\000\000\000\000\000\000\000\000\000"},
	{262, "fplist", 0, 2, states_6,
	 "\000\040\002\000\000\000\000\000\000\000\000\000"},
	{263, "fpdef", 0, 4, states_7,
	 "\000\040\002\000\000\000\000\000\000\000\000\000"},
	{264, "stmt", 0, 2, states_8,
	 "\000\060\002\200\026\037\310\002\000\014\352\004"},
	{265, "simple_stmt", 0, 2, states_9,
	 "\000\040\002\200\026\037\000\000\000\014\352\000"},
	{266, "expr_stmt", 0, 3, states_10,
	 "\000\040\002\000\000\000\000\000\000\014\352\000"},
	{267, "print_stmt", 0, 4, states_11,
	 "\000\000\000\200\000\000\000\000\000\000\000\000"},
	{268, "del_stmt", 0, 4, states_12,
	 "\000\000\000\000\002\000\000\000\000\000\000\000"},
	{269, "dir_stmt", 0, 4, states_13,
	 "\000\000\000\000\004\000\000\000\000\000\000\000"},
	{270, "pass_stmt", 0, 3, states_14,
	 "\000\000\000\000\020\000\000\000\000\000\000\000"},
	{271, "flow_stmt", 0, 2, states_15,
	 "\000\000\000\000\000\007\000\000\000\000\000\000"},
	{272, "break_stmt", 0, 3, states_16,
	 "\000\000\000\000\000\001\000\000\000\000\000\000"},
	{273, "return_stmt", 0, 4, states_17,
	 "\000\000\000\000\000\002\000\000\000\000\000\000"},
	{274, "raise_stmt", 0, 6, states_18,
	 "\000\000\000\000\000\004\000\000\000\000\000\000"},
	{275, "import_stmt", 0, 10, states_19,
	 "\000\000\000\000\000\030\000\000\000\000\000\000"},
	{276, "compound_stmt", 0, 2, states_20,
	 "\000\020\000\000\000\000\310\002\000\000\000\004"},
	{277, "if_stmt", 0, 8, states_21,
	 "\000\000\000\000\000\000\010\000\000\000\000\000"},
	{278, "while_stmt", 0, 8, states_22,
	 "\000\000\000\000\000\000\100\000\000\000\000\000"},
	{279, "for_stmt", 0, 10, states_23,
	 "\000\000\000\000\000\000\200\000\000\000\000\000"},
	{280, "try_stmt", 0, 7, states_24,
	 "\000\000\000\000\000\000\000\002\000\000\000\000"},
	{281, "except_clause", 0, 5, states_25,
	 "\000\000\000\000\000\000\000\020\000\000\000\000"},
	{282, "suite", 0, 5, states_26,
	 "\004\040\002\200\026\037\000\000\000\014\352\000"},
	{283, "test", 0, 2, states_27,
	 "\000\040\002\000\000\000\000\000\010\014\352\000"},
	{284, "and_test", 0, 2, states_28,
	 "\000\040\002\000\000\000\000\000\010\014\352\000"},
	{285, "not_test", 0, 3, states_29,
	 "\000\040\002\000\000\000\000\000\010\014\352\000"},
	{286, "comparison", 0, 2, states_30,
	 "\000\040\002\000\000\000\000\000\000\014\352\000"},
	{287, "comp_op", 0, 6, states_31,
	 "\000\000\000\100\000\000\000\001\310\001\000\000"},
	{288, "expr", 0, 2, states_32,
	 "\000\040\002\000\000\000\000\000\000\014\352\000"},
	{289, "term", 0, 2, states_33,
	 "\000\040\002\000\000\000\000\000\000\014\352\000"},
	{290, "factor", 0, 4, states_34,
	 "\000\040\002\000\000\000\000\000\000\014\352\000"},
	{291, "atom", 0, 9, states_35,
	 "\000\040\002\000\000\000\000\000\000\000\352\000"},
	{292, "trailer", 0, 7, states_36,
	 "\000\000\002\000\000\000\000\000\000\000\002\002"},
	{293, "subscript", 0, 4, states_37,
	 "\000\240\002\000\000\000\000\000\000\014\352\000"},
	{294, "exprlist", 0, 3, states_38,
	 "\000\040\002\000\000\000\000\000\000\014\352\000"},
	{295, "testlist", 0, 3, states_39,
	 "\000\040\002\000\000\000\000\000\010\014\352\000"},
	{296, "classdef", 0, 8, states_40,
	 "\000\000\000\000\000\000\000\000\000\000\000\004"},
	{297, "baselist", 0, 3, states_41,
	 "\000\040\002\000\000\000\000\000\000\000\352\000"},
	{298, "arguments", 0, 4, states_42,
	 "\000\000\002\000\000\000\000\000\000\000\000\000"},
};
static label labels[93] = {
	{0, "EMPTY"},
	{256, 0},
	{4, 0},
	{265, 0},
	{276, 0},
	{257, 0},
	{264, 0},
	{0, 0},
	{258, 0},
	{295, 0},
	{259, 0},
	{260, 0},
	{1, "def"},
	{1, 0},
	{261, 0},
	{11, 0},
	{282, 0},
	{7, 0},
	{262, 0},
	{8, 0},
	{263, 0},
	{12, 0},
	{266, 0},
	{267, 0},
	{270, 0},
	{268, 0},
	{269, 0},
	{271, 0},
	{275, 0},
	{294, 0},
	{22, 0},
	{1, "print"},
	{283, 0},
	{1, "del"},
	{1, "dir"},
	{288, 0},
	{1, "pass"},
	{272, 0},
	{273, 0},
	{274, 0},
	{1, "break"},
	{1, "return"},
	{1, "raise"},
	{1, "import"},
	{1, "from"},
	{16, 0},
	{277, 0},
	{278, 0},
	{279, 0},
	{280, 0},
	{296, 0},
	{1, "if"},
	{1, "elif"},
	{1, "else"},
	{1, "while"},
	{1, "for"},
	{1, "in"},
	{1, "try"},
	{281, 0},
	{1, "finally"},
	{1, "except"},
	{5, 0},
	{6, 0},
	{284, 0},
	{1, "or"},
	{285, 0},
	{1, "and"},
	{1, "not"},
	{286, 0},
	{287, 0},
	{20, 0},
	{21, 0},
	{1, "is"},
	{289, 0},
	{14, 0},
	{15, 0},
	{290, 0},
	{17, 0},
	{24, 0},
	{291, 0},
	{292, 0},
	{9, 0},
	{10, 0},
	{26, 0},
	{27, 0},
	{25, 0},
	{2, 0},
	{3, 0},
	{293, 0},
	{23, 0},
	{1, "class"},
	{297, 0},
	{298, 0},
};
grammar gram = {
	43,
	dfas,
	{93, labels},
	256
};
