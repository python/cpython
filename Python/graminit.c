/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

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
static arc arcs_9_0[1] = {
	{22, 1},
};
static arc arcs_9_1[2] = {
	{23, 2},
	{2, 3},
};
static arc arcs_9_2[2] = {
	{22, 1},
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
static arc arcs_10_0[6] = {
	{24, 1},
	{25, 1},
	{26, 1},
	{27, 1},
	{28, 1},
	{29, 1},
};
static arc arcs_10_1[1] = {
	{0, 1},
};
static state states_10[2] = {
	{6, arcs_10_0},
	{1, arcs_10_1},
};
static arc arcs_11_0[1] = {
	{30, 1},
};
static arc arcs_11_1[2] = {
	{31, 0},
	{0, 1},
};
static state states_11[2] = {
	{1, arcs_11_0},
	{2, arcs_11_1},
};
static arc arcs_12_0[1] = {
	{32, 1},
};
static arc arcs_12_1[2] = {
	{33, 2},
	{0, 1},
};
static arc arcs_12_2[2] = {
	{21, 1},
	{0, 2},
};
static state states_12[3] = {
	{1, arcs_12_0},
	{2, arcs_12_1},
	{2, arcs_12_2},
};
static arc arcs_13_0[1] = {
	{34, 1},
};
static arc arcs_13_1[1] = {
	{30, 2},
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
	{35, 1},
};
static arc arcs_14_1[1] = {
	{0, 1},
};
static state states_14[2] = {
	{1, arcs_14_0},
	{1, arcs_14_1},
};
static arc arcs_15_0[4] = {
	{36, 1},
	{37, 1},
	{38, 1},
	{39, 1},
};
static arc arcs_15_1[1] = {
	{0, 1},
};
static state states_15[2] = {
	{4, arcs_15_0},
	{1, arcs_15_1},
};
static arc arcs_16_0[1] = {
	{40, 1},
};
static arc arcs_16_1[1] = {
	{0, 1},
};
static state states_16[2] = {
	{1, arcs_16_0},
	{1, arcs_16_1},
};
static arc arcs_17_0[1] = {
	{41, 1},
};
static arc arcs_17_1[1] = {
	{0, 1},
};
static state states_17[2] = {
	{1, arcs_17_0},
	{1, arcs_17_1},
};
static arc arcs_18_0[1] = {
	{42, 1},
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
	{43, 1},
};
static arc arcs_19_1[1] = {
	{33, 2},
};
static arc arcs_19_2[2] = {
	{21, 3},
	{0, 2},
};
static arc arcs_19_3[1] = {
	{33, 4},
};
static arc arcs_19_4[1] = {
	{0, 4},
};
static state states_19[5] = {
	{1, arcs_19_0},
	{1, arcs_19_1},
	{2, arcs_19_2},
	{1, arcs_19_3},
	{1, arcs_19_4},
};
static arc arcs_20_0[2] = {
	{44, 1},
	{45, 2},
};
static arc arcs_20_1[1] = {
	{13, 3},
};
static arc arcs_20_2[1] = {
	{13, 4},
};
static arc arcs_20_3[2] = {
	{21, 1},
	{0, 3},
};
static arc arcs_20_4[1] = {
	{44, 5},
};
static arc arcs_20_5[2] = {
	{46, 6},
	{13, 7},
};
static arc arcs_20_6[1] = {
	{0, 6},
};
static arc arcs_20_7[2] = {
	{21, 8},
	{0, 7},
};
static arc arcs_20_8[1] = {
	{13, 7},
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
static arc arcs_21_0[6] = {
	{47, 1},
	{48, 1},
	{49, 1},
	{50, 1},
	{11, 1},
	{51, 1},
};
static arc arcs_21_1[1] = {
	{0, 1},
};
static state states_21[2] = {
	{6, arcs_21_0},
	{1, arcs_21_1},
};
static arc arcs_22_0[1] = {
	{52, 1},
};
static arc arcs_22_1[1] = {
	{33, 2},
};
static arc arcs_22_2[1] = {
	{15, 3},
};
static arc arcs_22_3[1] = {
	{16, 4},
};
static arc arcs_22_4[3] = {
	{53, 1},
	{54, 5},
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
	{3, arcs_22_4},
	{1, arcs_22_5},
	{1, arcs_22_6},
	{1, arcs_22_7},
};
static arc arcs_23_0[1] = {
	{55, 1},
};
static arc arcs_23_1[1] = {
	{33, 2},
};
static arc arcs_23_2[1] = {
	{15, 3},
};
static arc arcs_23_3[1] = {
	{16, 4},
};
static arc arcs_23_4[2] = {
	{54, 5},
	{0, 4},
};
static arc arcs_23_5[1] = {
	{15, 6},
};
static arc arcs_23_6[1] = {
	{16, 7},
};
static arc arcs_23_7[1] = {
	{0, 7},
};
static state states_23[8] = {
	{1, arcs_23_0},
	{1, arcs_23_1},
	{1, arcs_23_2},
	{1, arcs_23_3},
	{2, arcs_23_4},
	{1, arcs_23_5},
	{1, arcs_23_6},
	{1, arcs_23_7},
};
static arc arcs_24_0[1] = {
	{56, 1},
};
static arc arcs_24_1[1] = {
	{30, 2},
};
static arc arcs_24_2[1] = {
	{57, 3},
};
static arc arcs_24_3[1] = {
	{9, 4},
};
static arc arcs_24_4[1] = {
	{15, 5},
};
static arc arcs_24_5[1] = {
	{16, 6},
};
static arc arcs_24_6[2] = {
	{54, 7},
	{0, 6},
};
static arc arcs_24_7[1] = {
	{15, 8},
};
static arc arcs_24_8[1] = {
	{16, 9},
};
static arc arcs_24_9[1] = {
	{0, 9},
};
static state states_24[10] = {
	{1, arcs_24_0},
	{1, arcs_24_1},
	{1, arcs_24_2},
	{1, arcs_24_3},
	{1, arcs_24_4},
	{1, arcs_24_5},
	{2, arcs_24_6},
	{1, arcs_24_7},
	{1, arcs_24_8},
	{1, arcs_24_9},
};
static arc arcs_25_0[1] = {
	{58, 1},
};
static arc arcs_25_1[1] = {
	{15, 2},
};
static arc arcs_25_2[1] = {
	{16, 3},
};
static arc arcs_25_3[3] = {
	{59, 1},
	{60, 4},
	{0, 3},
};
static arc arcs_25_4[1] = {
	{15, 5},
};
static arc arcs_25_5[1] = {
	{16, 6},
};
static arc arcs_25_6[1] = {
	{0, 6},
};
static state states_25[7] = {
	{1, arcs_25_0},
	{1, arcs_25_1},
	{1, arcs_25_2},
	{3, arcs_25_3},
	{1, arcs_25_4},
	{1, arcs_25_5},
	{1, arcs_25_6},
};
static arc arcs_26_0[1] = {
	{61, 1},
};
static arc arcs_26_1[2] = {
	{33, 2},
	{0, 1},
};
static arc arcs_26_2[2] = {
	{21, 3},
	{0, 2},
};
static arc arcs_26_3[1] = {
	{33, 4},
};
static arc arcs_26_4[1] = {
	{0, 4},
};
static state states_26[5] = {
	{1, arcs_26_0},
	{2, arcs_26_1},
	{2, arcs_26_2},
	{1, arcs_26_3},
	{1, arcs_26_4},
};
static arc arcs_27_0[2] = {
	{3, 1},
	{2, 2},
};
static arc arcs_27_1[1] = {
	{0, 1},
};
static arc arcs_27_2[1] = {
	{62, 3},
};
static arc arcs_27_3[2] = {
	{2, 3},
	{6, 4},
};
static arc arcs_27_4[3] = {
	{6, 4},
	{2, 4},
	{63, 1},
};
static state states_27[5] = {
	{2, arcs_27_0},
	{1, arcs_27_1},
	{1, arcs_27_2},
	{2, arcs_27_3},
	{3, arcs_27_4},
};
static arc arcs_28_0[1] = {
	{64, 1},
};
static arc arcs_28_1[2] = {
	{65, 0},
	{0, 1},
};
static state states_28[2] = {
	{1, arcs_28_0},
	{2, arcs_28_1},
};
static arc arcs_29_0[1] = {
	{66, 1},
};
static arc arcs_29_1[2] = {
	{67, 0},
	{0, 1},
};
static state states_29[2] = {
	{1, arcs_29_0},
	{2, arcs_29_1},
};
static arc arcs_30_0[2] = {
	{68, 1},
	{69, 2},
};
static arc arcs_30_1[1] = {
	{66, 2},
};
static arc arcs_30_2[1] = {
	{0, 2},
};
static state states_30[3] = {
	{2, arcs_30_0},
	{1, arcs_30_1},
	{1, arcs_30_2},
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
static arc arcs_32_0[6] = {
	{72, 1},
	{73, 2},
	{31, 3},
	{57, 3},
	{68, 4},
	{74, 5},
};
static arc arcs_32_1[3] = {
	{31, 3},
	{73, 3},
	{0, 1},
};
static arc arcs_32_2[2] = {
	{31, 3},
	{0, 2},
};
static arc arcs_32_3[1] = {
	{0, 3},
};
static arc arcs_32_4[1] = {
	{57, 3},
};
static arc arcs_32_5[2] = {
	{68, 3},
	{0, 5},
};
static state states_32[6] = {
	{6, arcs_32_0},
	{3, arcs_32_1},
	{2, arcs_32_2},
	{1, arcs_32_3},
	{1, arcs_32_4},
	{2, arcs_32_5},
};
static arc arcs_33_0[1] = {
	{75, 1},
};
static arc arcs_33_1[3] = {
	{76, 0},
	{77, 0},
	{0, 1},
};
static state states_33[2] = {
	{1, arcs_33_0},
	{3, arcs_33_1},
};
static arc arcs_34_0[1] = {
	{78, 1},
};
static arc arcs_34_1[4] = {
	{46, 0},
	{79, 0},
	{80, 0},
	{0, 1},
};
static state states_34[2] = {
	{1, arcs_34_0},
	{4, arcs_34_1},
};
static arc arcs_35_0[3] = {
	{76, 1},
	{77, 1},
	{81, 2},
};
static arc arcs_35_1[1] = {
	{78, 3},
};
static arc arcs_35_2[2] = {
	{82, 2},
	{0, 2},
};
static arc arcs_35_3[1] = {
	{0, 3},
};
static state states_35[4] = {
	{3, arcs_35_0},
	{1, arcs_35_1},
	{2, arcs_35_2},
	{1, arcs_35_3},
};
static arc arcs_36_0[7] = {
	{17, 1},
	{83, 2},
	{85, 3},
	{88, 4},
	{13, 5},
	{89, 5},
	{90, 5},
};
static arc arcs_36_1[2] = {
	{9, 6},
	{19, 5},
};
static arc arcs_36_2[2] = {
	{9, 7},
	{84, 5},
};
static arc arcs_36_3[2] = {
	{86, 8},
	{87, 5},
};
static arc arcs_36_4[1] = {
	{9, 9},
};
static arc arcs_36_5[1] = {
	{0, 5},
};
static arc arcs_36_6[1] = {
	{19, 5},
};
static arc arcs_36_7[1] = {
	{84, 5},
};
static arc arcs_36_8[1] = {
	{87, 5},
};
static arc arcs_36_9[1] = {
	{88, 5},
};
static state states_36[10] = {
	{7, arcs_36_0},
	{2, arcs_36_1},
	{2, arcs_36_2},
	{2, arcs_36_3},
	{1, arcs_36_4},
	{1, arcs_36_5},
	{1, arcs_36_6},
	{1, arcs_36_7},
	{1, arcs_36_8},
	{1, arcs_36_9},
};
static arc arcs_37_0[3] = {
	{17, 1},
	{83, 2},
	{92, 3},
};
static arc arcs_37_1[2] = {
	{9, 4},
	{19, 5},
};
static arc arcs_37_2[1] = {
	{91, 6},
};
static arc arcs_37_3[1] = {
	{13, 5},
};
static arc arcs_37_4[1] = {
	{19, 5},
};
static arc arcs_37_5[1] = {
	{0, 5},
};
static arc arcs_37_6[1] = {
	{84, 5},
};
static state states_37[7] = {
	{3, arcs_37_0},
	{2, arcs_37_1},
	{1, arcs_37_2},
	{1, arcs_37_3},
	{1, arcs_37_4},
	{1, arcs_37_5},
	{1, arcs_37_6},
};
static arc arcs_38_0[2] = {
	{33, 1},
	{15, 2},
};
static arc arcs_38_1[2] = {
	{15, 2},
	{0, 1},
};
static arc arcs_38_2[2] = {
	{33, 3},
	{0, 2},
};
static arc arcs_38_3[1] = {
	{0, 3},
};
static state states_38[4] = {
	{2, arcs_38_0},
	{2, arcs_38_1},
	{2, arcs_38_2},
	{1, arcs_38_3},
};
static arc arcs_39_0[1] = {
	{70, 1},
};
static arc arcs_39_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_39_2[2] = {
	{70, 1},
	{0, 2},
};
static state states_39[3] = {
	{1, arcs_39_0},
	{2, arcs_39_1},
	{2, arcs_39_2},
};
static arc arcs_40_0[1] = {
	{33, 1},
};
static arc arcs_40_1[2] = {
	{21, 2},
	{0, 1},
};
static arc arcs_40_2[2] = {
	{33, 1},
	{0, 2},
};
static state states_40[3] = {
	{1, arcs_40_0},
	{2, arcs_40_1},
	{2, arcs_40_2},
};
static arc arcs_41_0[1] = {
	{33, 1},
};
static arc arcs_41_1[1] = {
	{15, 2},
};
static arc arcs_41_2[1] = {
	{33, 3},
};
static arc arcs_41_3[2] = {
	{21, 4},
	{0, 3},
};
static arc arcs_41_4[2] = {
	{33, 1},
	{0, 4},
};
static state states_41[5] = {
	{1, arcs_41_0},
	{1, arcs_41_1},
	{1, arcs_41_2},
	{2, arcs_41_3},
	{2, arcs_41_4},
};
static arc arcs_42_0[1] = {
	{93, 1},
};
static arc arcs_42_1[1] = {
	{13, 2},
};
static arc arcs_42_2[1] = {
	{14, 3},
};
static arc arcs_42_3[2] = {
	{31, 4},
	{15, 5},
};
static arc arcs_42_4[1] = {
	{94, 6},
};
static arc arcs_42_5[1] = {
	{16, 7},
};
static arc arcs_42_6[1] = {
	{15, 5},
};
static arc arcs_42_7[1] = {
	{0, 7},
};
static state states_42[8] = {
	{1, arcs_42_0},
	{1, arcs_42_1},
	{1, arcs_42_2},
	{2, arcs_42_3},
	{1, arcs_42_4},
	{1, arcs_42_5},
	{1, arcs_42_6},
	{1, arcs_42_7},
};
static arc arcs_43_0[1] = {
	{81, 1},
};
static arc arcs_43_1[1] = {
	{95, 2},
};
static arc arcs_43_2[2] = {
	{21, 0},
	{0, 2},
};
static state states_43[3] = {
	{1, arcs_43_0},
	{1, arcs_43_1},
	{2, arcs_43_2},
};
static arc arcs_44_0[1] = {
	{17, 1},
};
static arc arcs_44_1[2] = {
	{9, 2},
	{19, 3},
};
static arc arcs_44_2[1] = {
	{19, 3},
};
static arc arcs_44_3[1] = {
	{0, 3},
};
static state states_44[4] = {
	{1, arcs_44_0},
	{2, arcs_44_1},
	{1, arcs_44_2},
	{1, arcs_44_3},
};
static dfa dfas[45] = {
	{256, "single_input", 0, 3, states_0,
	 "\004\060\002\000\015\077\220\005\000\060\050\047"},
	{257, "file_input", 0, 2, states_1,
	 "\204\060\002\000\015\077\220\005\000\060\050\047"},
	{258, "expr_input", 0, 3, states_2,
	 "\000\040\002\000\000\000\000\000\020\060\050\007"},
	{259, "eval_input", 0, 3, states_3,
	 "\000\040\002\000\000\000\000\000\020\060\050\007"},
	{260, "funcdef", 0, 6, states_4,
	 "\000\020\000\000\000\000\000\000\000\000\000\000"},
	{261, "parameters", 0, 4, states_5,
	 "\000\000\002\000\000\000\000\000\000\000\000\000"},
	{262, "fplist", 0, 2, states_6,
	 "\000\040\002\000\000\000\000\000\000\000\000\000"},
	{263, "fpdef", 0, 4, states_7,
	 "\000\040\002\000\000\000\000\000\000\000\000\000"},
	{264, "stmt", 0, 2, states_8,
	 "\000\060\002\000\015\077\220\005\000\060\050\047"},
	{265, "simple_stmt", 0, 4, states_9,
	 "\000\040\002\000\015\077\000\000\000\060\050\007"},
	{266, "small_stmt", 0, 2, states_10,
	 "\000\040\002\000\015\077\000\000\000\060\050\007"},
	{267, "expr_stmt", 0, 2, states_11,
	 "\000\040\002\000\000\000\000\000\000\060\050\007"},
	{268, "print_stmt", 0, 3, states_12,
	 "\000\000\000\000\001\000\000\000\000\000\000\000"},
	{269, "del_stmt", 0, 3, states_13,
	 "\000\000\000\000\004\000\000\000\000\000\000\000"},
	{270, "pass_stmt", 0, 2, states_14,
	 "\000\000\000\000\010\000\000\000\000\000\000\000"},
	{271, "flow_stmt", 0, 2, states_15,
	 "\000\000\000\000\000\017\000\000\000\000\000\000"},
	{272, "break_stmt", 0, 2, states_16,
	 "\000\000\000\000\000\001\000\000\000\000\000\000"},
	{273, "continue_stmt", 0, 2, states_17,
	 "\000\000\000\000\000\002\000\000\000\000\000\000"},
	{274, "return_stmt", 0, 3, states_18,
	 "\000\000\000\000\000\004\000\000\000\000\000\000"},
	{275, "raise_stmt", 0, 5, states_19,
	 "\000\000\000\000\000\010\000\000\000\000\000\000"},
	{276, "import_stmt", 0, 9, states_20,
	 "\000\000\000\000\000\060\000\000\000\000\000\000"},
	{277, "compound_stmt", 0, 2, states_21,
	 "\000\020\000\000\000\000\220\005\000\000\000\040"},
	{278, "if_stmt", 0, 8, states_22,
	 "\000\000\000\000\000\000\020\000\000\000\000\000"},
	{279, "while_stmt", 0, 8, states_23,
	 "\000\000\000\000\000\000\200\000\000\000\000\000"},
	{280, "for_stmt", 0, 10, states_24,
	 "\000\000\000\000\000\000\000\001\000\000\000\000"},
	{281, "try_stmt", 0, 7, states_25,
	 "\000\000\000\000\000\000\000\004\000\000\000\000"},
	{282, "except_clause", 0, 5, states_26,
	 "\000\000\000\000\000\000\000\040\000\000\000\000"},
	{283, "suite", 0, 5, states_27,
	 "\004\040\002\000\015\077\000\000\000\060\050\007"},
	{284, "test", 0, 2, states_28,
	 "\000\040\002\000\000\000\000\000\020\060\050\007"},
	{285, "and_test", 0, 2, states_29,
	 "\000\040\002\000\000\000\000\000\020\060\050\007"},
	{286, "not_test", 0, 3, states_30,
	 "\000\040\002\000\000\000\000\000\020\060\050\007"},
	{287, "comparison", 0, 2, states_31,
	 "\000\040\002\000\000\000\000\000\000\060\050\007"},
	{288, "comp_op", 0, 6, states_32,
	 "\000\000\000\200\000\000\000\002\020\007\000\000"},
	{289, "expr", 0, 2, states_33,
	 "\000\040\002\000\000\000\000\000\000\060\050\007"},
	{290, "term", 0, 2, states_34,
	 "\000\040\002\000\000\000\000\000\000\060\050\007"},
	{291, "factor", 0, 4, states_35,
	 "\000\040\002\000\000\000\000\000\000\060\050\007"},
	{292, "atom", 0, 10, states_36,
	 "\000\040\002\000\000\000\000\000\000\000\050\007"},
	{293, "trailer", 0, 7, states_37,
	 "\000\000\002\000\000\000\000\000\000\000\010\020"},
	{294, "subscript", 0, 4, states_38,
	 "\000\240\002\000\000\000\000\000\020\060\050\007"},
	{295, "exprlist", 0, 3, states_39,
	 "\000\040\002\000\000\000\000\000\000\060\050\007"},
	{296, "testlist", 0, 3, states_40,
	 "\000\040\002\000\000\000\000\000\020\060\050\007"},
	{297, "dictmaker", 0, 5, states_41,
	 "\000\040\002\000\000\000\000\000\020\060\050\007"},
	{298, "classdef", 0, 8, states_42,
	 "\000\000\000\000\000\000\000\000\000\000\000\040"},
	{299, "baselist", 0, 3, states_43,
	 "\000\040\002\000\000\000\000\000\000\000\050\007"},
	{300, "arguments", 0, 4, states_44,
	 "\000\000\002\000\000\000\000\000\000\000\000\000"},
};
static label labels[96] = {
	{0, "EMPTY"},
	{256, 0},
	{4, 0},
	{265, 0},
	{277, 0},
	{257, 0},
	{264, 0},
	{0, 0},
	{258, 0},
	{296, 0},
	{259, 0},
	{260, 0},
	{1, "def"},
	{1, 0},
	{261, 0},
	{11, 0},
	{283, 0},
	{7, 0},
	{262, 0},
	{8, 0},
	{263, 0},
	{12, 0},
	{266, 0},
	{13, 0},
	{267, 0},
	{268, 0},
	{269, 0},
	{270, 0},
	{271, 0},
	{276, 0},
	{295, 0},
	{22, 0},
	{1, "print"},
	{284, 0},
	{1, "del"},
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
	{1, "from"},
	{16, 0},
	{278, 0},
	{279, 0},
	{280, 0},
	{281, 0},
	{298, 0},
	{1, "if"},
	{1, "elif"},
	{1, "else"},
	{1, "while"},
	{1, "for"},
	{1, "in"},
	{1, "try"},
	{282, 0},
	{1, "finally"},
	{1, "except"},
	{5, 0},
	{6, 0},
	{285, 0},
	{1, "or"},
	{286, 0},
	{1, "and"},
	{1, "not"},
	{287, 0},
	{289, 0},
	{288, 0},
	{20, 0},
	{21, 0},
	{1, "is"},
	{290, 0},
	{14, 0},
	{15, 0},
	{291, 0},
	{17, 0},
	{24, 0},
	{292, 0},
	{293, 0},
	{9, 0},
	{10, 0},
	{26, 0},
	{297, 0},
	{27, 0},
	{25, 0},
	{2, 0},
	{3, 0},
	{294, 0},
	{23, 0},
	{1, "class"},
	{299, 0},
	{300, 0},
};
grammar gram = {
	45,
	dfas,
	{96, labels},
	256
};
