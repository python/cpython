/* $XConsortium: keysymdef.h,v 1.15 93/04/02 10:57:36 rws Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#define XK_VoidSymbol		0xFFFFFF	/* void symbol */

#ifdef XK_MISCELLANY
/*
 * TTY Functions, cleverly chosen to map to ascii, for convenience of
 * programming, but could have been arbitrary (at the cost of lookup
 * tables in client code.
 */

#define XK_BackSpace		0xFF08	/* back space, back char */
#define XK_Tab			0xFF09
#define XK_Linefeed		0xFF0A	/* Linefeed, LF */
#define XK_Clear		0xFF0B
#define XK_Return		0xFF0D	/* Return, enter */
#define XK_Pause		0xFF13	/* Pause, hold */
#define XK_Scroll_Lock		0xFF14
#define XK_Sys_Req		0xFF15
#define XK_Escape		0xFF1B
#define XK_Delete		0xFFFF	/* Delete, rubout */



/* International & multi-key character composition */

#define XK_Multi_key		0xFF20  /* Multi-key character compose */

/* Japanese keyboard support */

#define XK_Kanji		0xFF21	/* Kanji, Kanji convert */
#define XK_Muhenkan		0xFF22  /* Cancel Conversion */
#define XK_Henkan_Mode		0xFF23  /* Start/Stop Conversion */
#define XK_Henkan		0xFF23  /* Alias for Henkan_Mode */
#define XK_Romaji		0xFF24  /* to Romaji */
#define XK_Hiragana		0xFF25  /* to Hiragana */
#define XK_Katakana		0xFF26  /* to Katakana */
#define XK_Hiragana_Katakana	0xFF27  /* Hiragana/Katakana toggle */
#define XK_Zenkaku		0xFF28  /* to Zenkaku */
#define XK_Hankaku		0xFF29  /* to Hankaku */
#define XK_Zenkaku_Hankaku	0xFF2A  /* Zenkaku/Hankaku toggle */
#define XK_Touroku		0xFF2B  /* Add to Dictionary */
#define XK_Massyo		0xFF2C  /* Delete from Dictionary */
#define XK_Kana_Lock		0xFF2D  /* Kana Lock */
#define XK_Kana_Shift		0xFF2E  /* Kana Shift */
#define XK_Eisu_Shift		0xFF2F  /* Alphanumeric Shift */
#define XK_Eisu_toggle		0xFF30  /* Alphanumeric toggle */

/* Cursor control & motion */

#define XK_Home			0xFF50
#define XK_Left			0xFF51	/* Move left, left arrow */
#define XK_Up			0xFF52	/* Move up, up arrow */
#define XK_Right		0xFF53	/* Move right, right arrow */
#define XK_Down			0xFF54	/* Move down, down arrow */
#define XK_Prior		0xFF55	/* Prior, previous */
#define XK_Page_Up		0xFF55
#define XK_Next			0xFF56	/* Next */
#define XK_Page_Down		0xFF56
#define XK_End			0xFF57	/* EOL */
#define XK_Begin		0xFF58	/* BOL */

/* Special Windows keyboard keys */

#define XK_Win_L		0xFF5B	/* Left-hand Windows */
#define XK_Win_R		0xFF5C	/* Right-hand Windows */
#define XK_App			0xFF5D	/* Menu key */

/* Misc Functions */

#define XK_Select		0xFF60	/* Select, mark */
#define XK_Print		0xFF61
#define XK_Execute		0xFF62	/* Execute, run, do */
#define XK_Insert		0xFF63	/* Insert, insert here */
#define XK_Undo			0xFF65	/* Undo, oops */
#define XK_Redo			0xFF66	/* redo, again */
#define XK_Menu			0xFF67
#define XK_Find			0xFF68	/* Find, search */
#define XK_Cancel		0xFF69	/* Cancel, stop, abort, exit */
#define XK_Help			0xFF6A	/* Help, ? */
#define XK_Break		0xFF6B
#define XK_Mode_switch		0xFF7E	/* Character set switch */
#define XK_script_switch        0xFF7E  /* Alias for mode_switch */
#define XK_Num_Lock		0xFF7F

/* Keypad Functions, keypad numbers cleverly chosen to map to ascii */

#define XK_KP_Space		0xFF80	/* space */
#define XK_KP_Tab		0xFF89
#define XK_KP_Enter		0xFF8D	/* enter */
#define XK_KP_F1		0xFF91	/* PF1, KP_A, ... */
#define XK_KP_F2		0xFF92
#define XK_KP_F3		0xFF93
#define XK_KP_F4		0xFF94
#define XK_KP_Home		0xFF95
#define XK_KP_Left		0xFF96
#define XK_KP_Up		0xFF97
#define XK_KP_Right		0xFF98
#define XK_KP_Down		0xFF99
#define XK_KP_Prior		0xFF9A
#define XK_KP_Page_Up		0xFF9A
#define XK_KP_Next		0xFF9B
#define XK_KP_Page_Down		0xFF9B
#define XK_KP_End		0xFF9C
#define XK_KP_Begin		0xFF9D
#define XK_KP_Insert		0xFF9E
#define XK_KP_Delete		0xFF9F
#define XK_KP_Equal		0xFFBD	/* equals */
#define XK_KP_Multiply		0xFFAA
#define XK_KP_Add		0xFFAB
#define XK_KP_Separator		0xFFAC	/* separator, often comma */
#define XK_KP_Subtract		0xFFAD
#define XK_KP_Decimal		0xFFAE
#define XK_KP_Divide		0xFFAF

#define XK_KP_0			0xFFB0
#define XK_KP_1			0xFFB1
#define XK_KP_2			0xFFB2
#define XK_KP_3			0xFFB3
#define XK_KP_4			0xFFB4
#define XK_KP_5			0xFFB5
#define XK_KP_6			0xFFB6
#define XK_KP_7			0xFFB7
#define XK_KP_8			0xFFB8
#define XK_KP_9			0xFFB9



/*
 * Auxilliary Functions; note the duplicate definitions for left and right
 * function keys;  Sun keyboards and a few other manufactures have such
 * function key groups on the left and/or right sides of the keyboard.
 * We've not found a keyboard with more than 35 function keys total.
 */

#define XK_F1			0xFFBE
#define XK_F2			0xFFBF
#define XK_F3			0xFFC0
#define XK_F4			0xFFC1
#define XK_F5			0xFFC2
#define XK_F6			0xFFC3
#define XK_F7			0xFFC4
#define XK_F8			0xFFC5
#define XK_F9			0xFFC6
#define XK_F10			0xFFC7
#define XK_F11			0xFFC8
#define XK_L1			0xFFC8
#define XK_F12			0xFFC9
#define XK_L2			0xFFC9
#define XK_F13			0xFFCA
#define XK_L3			0xFFCA
#define XK_F14			0xFFCB
#define XK_L4			0xFFCB
#define XK_F15			0xFFCC
#define XK_L5			0xFFCC
#define XK_F16			0xFFCD
#define XK_L6			0xFFCD
#define XK_F17			0xFFCE
#define XK_L7			0xFFCE
#define XK_F18			0xFFCF
#define XK_L8			0xFFCF
#define XK_F19			0xFFD0
#define XK_L9			0xFFD0
#define XK_F20			0xFFD1
#define XK_L10			0xFFD1
#define XK_F21			0xFFD2
#define XK_R1			0xFFD2
#define XK_F22			0xFFD3
#define XK_R2			0xFFD3
#define XK_F23			0xFFD4
#define XK_R3			0xFFD4
#define XK_F24			0xFFD5
#define XK_R4			0xFFD5
#define XK_F25			0xFFD6
#define XK_R5			0xFFD6
#define XK_F26			0xFFD7
#define XK_R6			0xFFD7
#define XK_F27			0xFFD8
#define XK_R7			0xFFD8
#define XK_F28			0xFFD9
#define XK_R8			0xFFD9
#define XK_F29			0xFFDA
#define XK_R9			0xFFDA
#define XK_F30			0xFFDB
#define XK_R10			0xFFDB
#define XK_F31			0xFFDC
#define XK_R11			0xFFDC
#define XK_F32			0xFFDD
#define XK_R12			0xFFDD
#define XK_F33			0xFFDE
#define XK_R13			0xFFDE
#define XK_F34			0xFFDF
#define XK_R14			0xFFDF
#define XK_F35			0xFFE0
#define XK_R15			0xFFE0

/* Modifiers */

#define XK_Shift_L		0xFFE1	/* Left shift */
#define XK_Shift_R		0xFFE2	/* Right shift */
#define XK_Control_L		0xFFE3	/* Left control */
#define XK_Control_R		0xFFE4	/* Right control */
#define XK_Caps_Lock		0xFFE5	/* Caps lock */
#define XK_Shift_Lock		0xFFE6	/* Shift lock */

#define XK_Meta_L		0xFFE7	/* Left meta */
#define XK_Meta_R		0xFFE8	/* Right meta */
#define XK_Alt_L		0xFFE9	/* Left alt */
#define XK_Alt_R		0xFFEA	/* Right alt */
#define XK_Super_L		0xFFEB	/* Left super */
#define XK_Super_R		0xFFEC	/* Right super */
#define XK_Hyper_L		0xFFED	/* Left hyper */
#define XK_Hyper_R		0xFFEE	/* Right hyper */
#endif /* XK_MISCELLANY */

/*
 *  Latin 1
 *  Byte 3 = 0
 */
#ifdef XK_LATIN1
#define XK_space               0x020
#define XK_exclam              0x021
#define XK_quotedbl            0x022
#define XK_numbersign          0x023
#define XK_dollar              0x024
#define XK_percent             0x025
#define XK_ampersand           0x026
#define XK_apostrophe          0x027
#define XK_quoteright          0x027	/* deprecated */
#define XK_parenleft           0x028
#define XK_parenright          0x029
#define XK_asterisk            0x02a
#define XK_plus                0x02b
#define XK_comma               0x02c
#define XK_minus               0x02d
#define XK_period              0x02e
#define XK_slash               0x02f
#define XK_0                   0x030
#define XK_1                   0x031
#define XK_2                   0x032
#define XK_3                   0x033
#define XK_4                   0x034
#define XK_5                   0x035
#define XK_6                   0x036
#define XK_7                   0x037
#define XK_8                   0x038
#define XK_9                   0x039
#define XK_colon               0x03a
#define XK_semicolon           0x03b
#define XK_less                0x03c
#define XK_equal               0x03d
#define XK_greater             0x03e
#define XK_question            0x03f
#define XK_at                  0x040
#define XK_A                   0x041
#define XK_B                   0x042
#define XK_C                   0x043
#define XK_D                   0x044
#define XK_E                   0x045
#define XK_F                   0x046
#define XK_G                   0x047
#define XK_H                   0x048
#define XK_I                   0x049
#define XK_J                   0x04a
#define XK_K                   0x04b
#define XK_L                   0x04c
#define XK_M                   0x04d
#define XK_N                   0x04e
#define XK_O                   0x04f
#define XK_P                   0x050
#define XK_Q                   0x051
#define XK_R                   0x052
#define XK_S                   0x053
#define XK_T                   0x054
#define XK_U                   0x055
#define XK_V                   0x056
#define XK_W                   0x057
#define XK_X                   0x058
#define XK_Y                   0x059
#define XK_Z                   0x05a
#define XK_bracketleft         0x05b
#define XK_backslash           0x05c
#define XK_bracketright        0x05d
#define XK_asciicircum         0x05e
#define XK_underscore          0x05f
#define XK_grave               0x060
#define XK_quoteleft           0x060	/* deprecated */
#define XK_a                   0x061
#define XK_b                   0x062
#define XK_c                   0x063
#define XK_d                   0x064
#define XK_e                   0x065
#define XK_f                   0x066
#define XK_g                   0x067
#define XK_h                   0x068
#define XK_i                   0x069
#define XK_j                   0x06a
#define XK_k                   0x06b
#define XK_l                   0x06c
#define XK_m                   0x06d
#define XK_n                   0x06e
#define XK_o                   0x06f
#define XK_p                   0x070
#define XK_q                   0x071
#define XK_r                   0x072
#define XK_s                   0x073
#define XK_t                   0x074
#define XK_u                   0x075
#define XK_v                   0x076
#define XK_w                   0x077
#define XK_x                   0x078
#define XK_y                   0x079
#define XK_z                   0x07a
#define XK_braceleft           0x07b
#define XK_bar                 0x07c
#define XK_braceright          0x07d
#define XK_asciitilde          0x07e

#define XK_nobreakspace        0x0a0
#define XK_exclamdown          0x0a1
#define XK_cent        	       0x0a2
#define XK_sterling            0x0a3
#define XK_currency            0x0a4
#define XK_yen                 0x0a5
#define XK_brokenbar           0x0a6
#define XK_section             0x0a7
#define XK_diaeresis           0x0a8
#define XK_copyright           0x0a9
#define XK_ordfeminine         0x0aa
#define XK_guillemotleft       0x0ab	/* left angle quotation mark */
#define XK_notsign             0x0ac
#define XK_hyphen              0x0ad
#define XK_registered          0x0ae
#define XK_macron              0x0af
#define XK_degree              0x0b0
#define XK_plusminus           0x0b1
#define XK_twosuperior         0x0b2
#define XK_threesuperior       0x0b3
#define XK_acute               0x0b4
#define XK_mu                  0x0b5
#define XK_paragraph           0x0b6
#define XK_periodcentered      0x0b7
#define XK_cedilla             0x0b8
#define XK_onesuperior         0x0b9
#define XK_masculine           0x0ba
#define XK_guillemotright      0x0bb	/* right angle quotation mark */
#define XK_onequarter          0x0bc
#define XK_onehalf             0x0bd
#define XK_threequarters       0x0be
#define XK_questiondown        0x0bf
#define XK_Agrave              0x0c0
#define XK_Aacute              0x0c1
#define XK_Acircumflex         0x0c2
#define XK_Atilde              0x0c3
#define XK_Adiaeresis          0x0c4
#define XK_Aring               0x0c5
#define XK_AE                  0x0c6
#define XK_Ccedilla            0x0c7
#define XK_Egrave              0x0c8
#define XK_Eacute              0x0c9
#define XK_Ecircumflex         0x0ca
#define XK_Ediaeresis          0x0cb
#define XK_Igrave              0x0cc
#define XK_Iacute              0x0cd
#define XK_Icircumflex         0x0ce
#define XK_Idiaeresis          0x0cf
#define XK_ETH                 0x0d0
#define XK_Eth                 0x0d0	/* deprecated */
#define XK_Ntilde              0x0d1
#define XK_Ograve              0x0d2
#define XK_Oacute              0x0d3
#define XK_Ocircumflex         0x0d4
#define XK_Otilde              0x0d5
#define XK_Odiaeresis          0x0d6
#define XK_multiply            0x0d7
#define XK_Oslash              0x0d8
#define XK_Ooblique            0x0d8
#define XK_Ugrave              0x0d9
#define XK_Uacute              0x0da
#define XK_Ucircumflex         0x0db
#define XK_Udiaeresis          0x0dc
#define XK_Yacute              0x0dd
#define XK_THORN               0x0de
#define XK_Thorn               0x0de	/* deprecated */
#define XK_ssharp              0x0df
#define XK_agrave              0x0e0
#define XK_aacute              0x0e1
#define XK_acircumflex         0x0e2
#define XK_atilde              0x0e3
#define XK_adiaeresis          0x0e4
#define XK_aring               0x0e5
#define XK_ae                  0x0e6
#define XK_ccedilla            0x0e7
#define XK_egrave              0x0e8
#define XK_eacute              0x0e9
#define XK_ecircumflex         0x0ea
#define XK_ediaeresis          0x0eb
#define XK_igrave              0x0ec
#define XK_iacute              0x0ed
#define XK_icircumflex         0x0ee
#define XK_idiaeresis          0x0ef
#define XK_eth                 0x0f0
#define XK_ntilde              0x0f1
#define XK_ograve              0x0f2
#define XK_oacute              0x0f3
#define XK_ocircumflex         0x0f4
#define XK_otilde              0x0f5
#define XK_odiaeresis          0x0f6
#define XK_division            0x0f7
#define XK_oslash              0x0f8
#define XK_ugrave              0x0f9
#define XK_uacute              0x0fa
#define XK_ucircumflex         0x0fb
#define XK_udiaeresis          0x0fc
#define XK_yacute              0x0fd
#define XK_thorn               0x0fe
#define XK_ydiaeresis          0x0ff
#endif /* XK_LATIN1 */

/*
 *   Latin 2
 *   Byte 3 = 1
 */

#ifdef XK_LATIN2
#define XK_Aogonek             0x1a1
#define XK_breve               0x1a2
#define XK_Lstroke             0x1a3
#define XK_Lcaron              0x1a5
#define XK_Sacute              0x1a6
#define XK_Scaron              0x1a9
#define XK_Scedilla            0x1aa
#define XK_Tcaron              0x1ab
#define XK_Zacute              0x1ac
#define XK_Zcaron              0x1ae
#define XK_Zabovedot           0x1af
#define XK_aogonek             0x1b1
#define XK_ogonek              0x1b2
#define XK_lstroke             0x1b3
#define XK_lcaron              0x1b5
#define XK_sacute              0x1b6
#define XK_caron               0x1b7
#define XK_scaron              0x1b9
#define XK_scedilla            0x1ba
#define XK_tcaron              0x1bb
#define XK_zacute              0x1bc
#define XK_doubleacute         0x1bd
#define XK_zcaron              0x1be
#define XK_zabovedot           0x1bf
#define XK_Racute              0x1c0
#define XK_Abreve              0x1c3
#define XK_Lacute              0x1c5
#define XK_Cacute              0x1c6
#define XK_Ccaron              0x1c8
#define XK_Eogonek             0x1ca
#define XK_Ecaron              0x1cc
#define XK_Dcaron              0x1cf
#define XK_Dstroke             0x1d0
#define XK_Nacute              0x1d1
#define XK_Ncaron              0x1d2
#define XK_Odoubleacute        0x1d5
#define XK_Rcaron              0x1d8
#define XK_Uring               0x1d9
#define XK_Udoubleacute        0x1db
#define XK_Tcedilla            0x1de
#define XK_racute              0x1e0
#define XK_abreve              0x1e3
#define XK_lacute              0x1e5
#define XK_cacute              0x1e6
#define XK_ccaron              0x1e8
#define XK_eogonek             0x1ea
#define XK_ecaron              0x1ec
#define XK_dcaron              0x1ef
#define XK_dstroke             0x1f0
#define XK_nacute              0x1f1
#define XK_ncaron              0x1f2
#define XK_odoubleacute        0x1f5
#define XK_udoubleacute        0x1fb
#define XK_rcaron              0x1f8
#define XK_uring               0x1f9
#define XK_tcedilla            0x1fe
#define XK_abovedot            0x1ff
#endif /* XK_LATIN2 */

/*
 *   Latin 3
 *   Byte 3 = 2
 */

#ifdef XK_LATIN3
#define XK_Hstroke             0x2a1
#define XK_Hcircumflex         0x2a6
#define XK_Iabovedot           0x2a9
#define XK_Gbreve              0x2ab
#define XK_Jcircumflex         0x2ac
#define XK_hstroke             0x2b1
#define XK_hcircumflex         0x2b6
#define XK_idotless            0x2b9
#define XK_gbreve              0x2bb
#define XK_jcircumflex         0x2bc
#define XK_Cabovedot           0x2c5
#define XK_Ccircumflex         0x2c6
#define XK_Gabovedot           0x2d5
#define XK_Gcircumflex         0x2d8
#define XK_Ubreve              0x2dd
#define XK_Scircumflex         0x2de
#define XK_cabovedot           0x2e5
#define XK_ccircumflex         0x2e6
#define XK_gabovedot           0x2f5
#define XK_gcircumflex         0x2f8
#define XK_ubreve              0x2fd
#define XK_scircumflex         0x2fe
#endif /* XK_LATIN3 */


/*
 *   Latin 4
 *   Byte 3 = 3
 */

#ifdef XK_LATIN4
#define XK_kra                 0x3a2
#define XK_kappa               0x3a2	/* deprecated */
#define XK_Rcedilla            0x3a3
#define XK_Itilde              0x3a5
#define XK_Lcedilla            0x3a6
#define XK_Emacron             0x3aa
#define XK_Gcedilla            0x3ab
#define XK_Tslash              0x3ac
#define XK_rcedilla            0x3b3
#define XK_itilde              0x3b5
#define XK_lcedilla            0x3b6
#define XK_emacron             0x3ba
#define XK_gcedilla            0x3bb
#define XK_tslash              0x3bc
#define XK_ENG                 0x3bd
#define XK_eng                 0x3bf
#define XK_Amacron             0x3c0
#define XK_Iogonek             0x3c7
#define XK_Eabovedot           0x3cc
#define XK_Imacron             0x3cf
#define XK_Ncedilla            0x3d1
#define XK_Omacron             0x3d2
#define XK_Kcedilla            0x3d3
#define XK_Uogonek             0x3d9
#define XK_Utilde              0x3dd
#define XK_Umacron             0x3de
#define XK_amacron             0x3e0
#define XK_iogonek             0x3e7
#define XK_eabovedot           0x3ec
#define XK_imacron             0x3ef
#define XK_ncedilla            0x3f1
#define XK_omacron             0x3f2
#define XK_kcedilla            0x3f3
#define XK_uogonek             0x3f9
#define XK_utilde              0x3fd
#define XK_umacron             0x3fe
#endif /* XK_LATIN4 */

/*
 * Katakana
 * Byte 3 = 4
 */

#ifdef XK_KATAKANA
#define XK_overline				       0x47e
#define XK_kana_fullstop                               0x4a1
#define XK_kana_openingbracket                         0x4a2
#define XK_kana_closingbracket                         0x4a3
#define XK_kana_comma                                  0x4a4
#define XK_kana_conjunctive                            0x4a5
#define XK_kana_middledot                              0x4a5  /* deprecated */
#define XK_kana_WO                                     0x4a6
#define XK_kana_a                                      0x4a7
#define XK_kana_i                                      0x4a8
#define XK_kana_u                                      0x4a9
#define XK_kana_e                                      0x4aa
#define XK_kana_o                                      0x4ab
#define XK_kana_ya                                     0x4ac
#define XK_kana_yu                                     0x4ad
#define XK_kana_yo                                     0x4ae
#define XK_kana_tsu                                    0x4af
#define XK_kana_tu                                     0x4af  /* deprecated */
#define XK_prolongedsound                              0x4b0
#define XK_kana_A                                      0x4b1
#define XK_kana_I                                      0x4b2
#define XK_kana_U                                      0x4b3
#define XK_kana_E                                      0x4b4
#define XK_kana_O                                      0x4b5
#define XK_kana_KA                                     0x4b6
#define XK_kana_KI                                     0x4b7
#define XK_kana_KU                                     0x4b8
#define XK_kana_KE                                     0x4b9
#define XK_kana_KO                                     0x4ba
#define XK_kana_SA                                     0x4bb
#define XK_kana_SHI                                    0x4bc
#define XK_kana_SU                                     0x4bd
#define XK_kana_SE                                     0x4be
#define XK_kana_SO                                     0x4bf
#define XK_kana_TA                                     0x4c0
#define XK_kana_CHI                                    0x4c1
#define XK_kana_TI                                     0x4c1  /* deprecated */
#define XK_kana_TSU                                    0x4c2
#define XK_kana_TU                                     0x4c2  /* deprecated */
#define XK_kana_TE                                     0x4c3
#define XK_kana_TO                                     0x4c4
#define XK_kana_NA                                     0x4c5
#define XK_kana_NI                                     0x4c6
#define XK_kana_NU                                     0x4c7
#define XK_kana_NE                                     0x4c8
#define XK_kana_NO                                     0x4c9
#define XK_kana_HA                                     0x4ca
#define XK_kana_HI                                     0x4cb
#define XK_kana_FU                                     0x4cc
#define XK_kana_HU                                     0x4cc  /* deprecated */
#define XK_kana_HE                                     0x4cd
#define XK_kana_HO                                     0x4ce
#define XK_kana_MA                                     0x4cf
#define XK_kana_MI                                     0x4d0
#define XK_kana_MU                                     0x4d1
#define XK_kana_ME                                     0x4d2
#define XK_kana_MO                                     0x4d3
#define XK_kana_YA                                     0x4d4
#define XK_kana_YU                                     0x4d5
#define XK_kana_YO                                     0x4d6
#define XK_kana_RA                                     0x4d7
#define XK_kana_RI                                     0x4d8
#define XK_kana_RU                                     0x4d9
#define XK_kana_RE                                     0x4da
#define XK_kana_RO                                     0x4db
#define XK_kana_WA                                     0x4dc
#define XK_kana_N                                      0x4dd
#define XK_voicedsound                                 0x4de
#define XK_semivoicedsound                             0x4df
#define XK_kana_switch          0xFF7E  /* Alias for mode_switch */
#endif /* XK_KATAKANA */

/*
 *  Arabic
 *  Byte 3 = 5
 */

#ifdef XK_ARABIC
#define XK_Arabic_comma                                0x5ac
#define XK_Arabic_semicolon                            0x5bb
#define XK_Arabic_question_mark                        0x5bf
#define XK_Arabic_hamza                                0x5c1
#define XK_Arabic_maddaonalef                          0x5c2
#define XK_Arabic_hamzaonalef                          0x5c3
#define XK_Arabic_hamzaonwaw                           0x5c4
#define XK_Arabic_hamzaunderalef                       0x5c5
#define XK_Arabic_hamzaonyeh                           0x5c6
#define XK_Arabic_alef                                 0x5c7
#define XK_Arabic_beh                                  0x5c8
#define XK_Arabic_tehmarbuta                           0x5c9
#define XK_Arabic_teh                                  0x5ca
#define XK_Arabic_theh                                 0x5cb
#define XK_Arabic_jeem                                 0x5cc
#define XK_Arabic_hah                                  0x5cd
#define XK_Arabic_khah                                 0x5ce
#define XK_Arabic_dal                                  0x5cf
#define XK_Arabic_thal                                 0x5d0
#define XK_Arabic_ra                                   0x5d1
#define XK_Arabic_zain                                 0x5d2
#define XK_Arabic_seen                                 0x5d3
#define XK_Arabic_sheen                                0x5d4
#define XK_Arabic_sad                                  0x5d5
#define XK_Arabic_dad                                  0x5d6
#define XK_Arabic_tah                                  0x5d7
#define XK_Arabic_zah                                  0x5d8
#define XK_Arabic_ain                                  0x5d9
#define XK_Arabic_ghain                                0x5da
#define XK_Arabic_tatweel                              0x5e0
#define XK_Arabic_feh                                  0x5e1
#define XK_Arabic_qaf                                  0x5e2
#define XK_Arabic_kaf                                  0x5e3
#define XK_Arabic_lam                                  0x5e4
#define XK_Arabic_meem                                 0x5e5
#define XK_Arabic_noon                                 0x5e6
#define XK_Arabic_ha                                   0x5e7
#define XK_Arabic_heh                                  0x5e7  /* deprecated */
#define XK_Arabic_waw                                  0x5e8
#define XK_Arabic_alefmaksura                          0x5e9
#define XK_Arabic_yeh                                  0x5ea
#define XK_Arabic_fathatan                             0x5eb
#define XK_Arabic_dammatan                             0x5ec
#define XK_Arabic_kasratan                             0x5ed
#define XK_Arabic_fatha                                0x5ee
#define XK_Arabic_damma                                0x5ef
#define XK_Arabic_kasra                                0x5f0
#define XK_Arabic_shadda                               0x5f1
#define XK_Arabic_sukun                                0x5f2
#define XK_Arabic_switch        0xFF7E  /* Alias for mode_switch */
#endif /* XK_ARABIC */

/*
 * Cyrillic
 * Byte 3 = 6
 */
#ifdef XK_CYRILLIC
#define XK_Serbian_dje                                 0x6a1
#define XK_Macedonia_gje                               0x6a2
#define XK_Cyrillic_io                                 0x6a3
#define XK_Ukrainian_ie                                0x6a4
#define XK_Ukranian_je                                 0x6a4  /* deprecated */
#define XK_Macedonia_dse                               0x6a5
#define XK_Ukrainian_i                                 0x6a6
#define XK_Ukranian_i                                  0x6a6  /* deprecated */
#define XK_Ukrainian_yi                                0x6a7
#define XK_Ukranian_yi                                 0x6a7  /* deprecated */
#define XK_Cyrillic_je                                 0x6a8
#define XK_Serbian_je                                  0x6a8  /* deprecated */
#define XK_Cyrillic_lje                                0x6a9
#define XK_Serbian_lje                                 0x6a9  /* deprecated */
#define XK_Cyrillic_nje                                0x6aa
#define XK_Serbian_nje                                 0x6aa  /* deprecated */
#define XK_Serbian_tshe                                0x6ab
#define XK_Macedonia_kje                               0x6ac
#define XK_Byelorussian_shortu                         0x6ae
#define XK_Cyrillic_dzhe                               0x6af
#define XK_Serbian_dze                                 0x6af  /* deprecated */
#define XK_numerosign                                  0x6b0
#define XK_Serbian_DJE                                 0x6b1
#define XK_Macedonia_GJE                               0x6b2
#define XK_Cyrillic_IO                                 0x6b3
#define XK_Ukrainian_IE                                0x6b4
#define XK_Ukranian_JE                                 0x6b4  /* deprecated */
#define XK_Macedonia_DSE                               0x6b5
#define XK_Ukrainian_I                                 0x6b6
#define XK_Ukranian_I                                  0x6b6  /* deprecated */
#define XK_Ukrainian_YI                                0x6b7
#define XK_Ukranian_YI                                 0x6b7  /* deprecated */
#define XK_Cyrillic_JE                                 0x6b8
#define XK_Serbian_JE                                  0x6b8  /* deprecated */
#define XK_Cyrillic_LJE                                0x6b9
#define XK_Serbian_LJE                                 0x6b9  /* deprecated */
#define XK_Cyrillic_NJE                                0x6ba
#define XK_Serbian_NJE                                 0x6ba  /* deprecated */
#define XK_Serbian_TSHE                                0x6bb
#define XK_Macedonia_KJE                               0x6bc
#define XK_Byelorussian_SHORTU                         0x6be
#define XK_Cyrillic_DZHE                               0x6bf
#define XK_Serbian_DZE                                 0x6bf  /* deprecated */
#define XK_Cyrillic_yu                                 0x6c0
#define XK_Cyrillic_a                                  0x6c1
#define XK_Cyrillic_be                                 0x6c2
#define XK_Cyrillic_tse                                0x6c3
#define XK_Cyrillic_de                                 0x6c4
#define XK_Cyrillic_ie                                 0x6c5
#define XK_Cyrillic_ef                                 0x6c6
#define XK_Cyrillic_ghe                                0x6c7
#define XK_Cyrillic_ha                                 0x6c8
#define XK_Cyrillic_i                                  0x6c9
#define XK_Cyrillic_shorti                             0x6ca
#define XK_Cyrillic_ka                                 0x6cb
#define XK_Cyrillic_el                                 0x6cc
#define XK_Cyrillic_em                                 0x6cd
#define XK_Cyrillic_en                                 0x6ce
#define XK_Cyrillic_o                                  0x6cf
#define XK_Cyrillic_pe                                 0x6d0
#define XK_Cyrillic_ya                                 0x6d1
#define XK_Cyrillic_er                                 0x6d2
#define XK_Cyrillic_es                                 0x6d3
#define XK_Cyrillic_te                                 0x6d4
#define XK_Cyrillic_u                                  0x6d5
#define XK_Cyrillic_zhe                                0x6d6
#define XK_Cyrillic_ve                                 0x6d7
#define XK_Cyrillic_softsign                           0x6d8
#define XK_Cyrillic_yeru                               0x6d9
#define XK_Cyrillic_ze                                 0x6da
#define XK_Cyrillic_sha                                0x6db
#define XK_Cyrillic_e                                  0x6dc
#define XK_Cyrillic_shcha                              0x6dd
#define XK_Cyrillic_che                                0x6de
#define XK_Cyrillic_hardsign                           0x6df
#define XK_Cyrillic_YU                                 0x6e0
#define XK_Cyrillic_A                                  0x6e1
#define XK_Cyrillic_BE                                 0x6e2
#define XK_Cyrillic_TSE                                0x6e3
#define XK_Cyrillic_DE                                 0x6e4
#define XK_Cyrillic_IE                                 0x6e5
#define XK_Cyrillic_EF                                 0x6e6
#define XK_Cyrillic_GHE                                0x6e7
#define XK_Cyrillic_HA                                 0x6e8
#define XK_Cyrillic_I                                  0x6e9
#define XK_Cyrillic_SHORTI                             0x6ea
#define XK_Cyrillic_KA                                 0x6eb
#define XK_Cyrillic_EL                                 0x6ec
#define XK_Cyrillic_EM                                 0x6ed
#define XK_Cyrillic_EN                                 0x6ee
#define XK_Cyrillic_O                                  0x6ef
#define XK_Cyrillic_PE                                 0x6f0
#define XK_Cyrillic_YA                                 0x6f1
#define XK_Cyrillic_ER                                 0x6f2
#define XK_Cyrillic_ES                                 0x6f3
#define XK_Cyrillic_TE                                 0x6f4
#define XK_Cyrillic_U                                  0x6f5
#define XK_Cyrillic_ZHE                                0x6f6
#define XK_Cyrillic_VE                                 0x6f7
#define XK_Cyrillic_SOFTSIGN                           0x6f8
#define XK_Cyrillic_YERU                               0x6f9
#define XK_Cyrillic_ZE                                 0x6fa
#define XK_Cyrillic_SHA                                0x6fb
#define XK_Cyrillic_E                                  0x6fc
#define XK_Cyrillic_SHCHA                              0x6fd
#define XK_Cyrillic_CHE                                0x6fe
#define XK_Cyrillic_HARDSIGN                           0x6ff
#endif /* XK_CYRILLIC */

/*
 * Greek
 * Byte 3 = 7
 */

#ifdef XK_GREEK
#define XK_Greek_ALPHAaccent                           0x7a1
#define XK_Greek_EPSILONaccent                         0x7a2
#define XK_Greek_ETAaccent                             0x7a3
#define XK_Greek_IOTAaccent                            0x7a4
#define XK_Greek_IOTAdieresis                          0x7a5
#define XK_Greek_IOTAdiaeresis                         0x7a5
#define XK_Greek_OMICRONaccent                         0x7a7
#define XK_Greek_UPSILONaccent                         0x7a8
#define XK_Greek_UPSILONdieresis                       0x7a9
#define XK_Greek_OMEGAaccent                           0x7ab
#define XK_Greek_accentdieresis                        0x7ae
#define XK_Greek_horizbar                              0x7af
#define XK_Greek_alphaaccent                           0x7b1
#define XK_Greek_epsilonaccent                         0x7b2
#define XK_Greek_etaaccent                             0x7b3
#define XK_Greek_iotaaccent                            0x7b4
#define XK_Greek_iotadieresis                          0x7b5
#define XK_Greek_iotaaccentdieresis                    0x7b6
#define XK_Greek_omicronaccent                         0x7b7
#define XK_Greek_upsilonaccent                         0x7b8
#define XK_Greek_upsilondieresis                       0x7b9
#define XK_Greek_upsilonaccentdieresis                 0x7ba
#define XK_Greek_omegaaccent                           0x7bb
#define XK_Greek_ALPHA                                 0x7c1
#define XK_Greek_BETA                                  0x7c2
#define XK_Greek_GAMMA                                 0x7c3
#define XK_Greek_DELTA                                 0x7c4
#define XK_Greek_EPSILON                               0x7c5
#define XK_Greek_ZETA                                  0x7c6
#define XK_Greek_ETA                                   0x7c7
#define XK_Greek_THETA                                 0x7c8
#define XK_Greek_IOTA                                  0x7c9
#define XK_Greek_KAPPA                                 0x7ca
#define XK_Greek_LAMDA                                 0x7cb
#define XK_Greek_LAMBDA                                0x7cb
#define XK_Greek_MU                                    0x7cc
#define XK_Greek_NU                                    0x7cd
#define XK_Greek_XI                                    0x7ce
#define XK_Greek_OMICRON                               0x7cf
#define XK_Greek_PI                                    0x7d0
#define XK_Greek_RHO                                   0x7d1
#define XK_Greek_SIGMA                                 0x7d2
#define XK_Greek_TAU                                   0x7d4
#define XK_Greek_UPSILON                               0x7d5
#define XK_Greek_PHI                                   0x7d6
#define XK_Greek_CHI                                   0x7d7
#define XK_Greek_PSI                                   0x7d8
#define XK_Greek_OMEGA                                 0x7d9
#define XK_Greek_alpha                                 0x7e1
#define XK_Greek_beta                                  0x7e2
#define XK_Greek_gamma                                 0x7e3
#define XK_Greek_delta                                 0x7e4
#define XK_Greek_epsilon                               0x7e5
#define XK_Greek_zeta                                  0x7e6
#define XK_Greek_eta                                   0x7e7
#define XK_Greek_theta                                 0x7e8
#define XK_Greek_iota                                  0x7e9
#define XK_Greek_kappa                                 0x7ea
#define XK_Greek_lamda                                 0x7eb
#define XK_Greek_lambda                                0x7eb
#define XK_Greek_mu                                    0x7ec
#define XK_Greek_nu                                    0x7ed
#define XK_Greek_xi                                    0x7ee
#define XK_Greek_omicron                               0x7ef
#define XK_Greek_pi                                    0x7f0
#define XK_Greek_rho                                   0x7f1
#define XK_Greek_sigma                                 0x7f2
#define XK_Greek_finalsmallsigma                       0x7f3
#define XK_Greek_tau                                   0x7f4
#define XK_Greek_upsilon                               0x7f5
#define XK_Greek_phi                                   0x7f6
#define XK_Greek_chi                                   0x7f7
#define XK_Greek_psi                                   0x7f8
#define XK_Greek_omega                                 0x7f9
#define XK_Greek_switch         0xFF7E  /* Alias for mode_switch */
#endif /* XK_GREEK */

/*
 * Technical
 * Byte 3 = 8
 */

#ifdef XK_TECHNICAL
#define XK_leftradical                                 0x8a1
#define XK_topleftradical                              0x8a2
#define XK_horizconnector                              0x8a3
#define XK_topintegral                                 0x8a4
#define XK_botintegral                                 0x8a5
#define XK_vertconnector                               0x8a6
#define XK_topleftsqbracket                            0x8a7
#define XK_botleftsqbracket                            0x8a8
#define XK_toprightsqbracket                           0x8a9
#define XK_botrightsqbracket                           0x8aa
#define XK_topleftparens                               0x8ab
#define XK_botleftparens                               0x8ac
#define XK_toprightparens                              0x8ad
#define XK_botrightparens                              0x8ae
#define XK_leftmiddlecurlybrace                        0x8af
#define XK_rightmiddlecurlybrace                       0x8b0
#define XK_topleftsummation                            0x8b1
#define XK_botleftsummation                            0x8b2
#define XK_topvertsummationconnector                   0x8b3
#define XK_botvertsummationconnector                   0x8b4
#define XK_toprightsummation                           0x8b5
#define XK_botrightsummation                           0x8b6
#define XK_rightmiddlesummation                        0x8b7
#define XK_lessthanequal                               0x8bc
#define XK_notequal                                    0x8bd
#define XK_greaterthanequal                            0x8be
#define XK_integral                                    0x8bf
#define XK_therefore                                   0x8c0
#define XK_variation                                   0x8c1
#define XK_infinity                                    0x8c2
#define XK_nabla                                       0x8c5
#define XK_approximate                                 0x8c8
#define XK_similarequal                                0x8c9
#define XK_ifonlyif                                    0x8cd
#define XK_implies                                     0x8ce
#define XK_identical                                   0x8cf
#define XK_radical                                     0x8d6
#define XK_includedin                                  0x8da
#define XK_includes                                    0x8db
#define XK_intersection                                0x8dc
#define XK_union                                       0x8dd
#define XK_logicaland                                  0x8de
#define XK_logicalor                                   0x8df
#define XK_partialderivative                           0x8ef
#define XK_function                                    0x8f6
#define XK_leftarrow                                   0x8fb
#define XK_uparrow                                     0x8fc
#define XK_rightarrow                                  0x8fd
#define XK_downarrow                                   0x8fe
#endif /* XK_TECHNICAL */

/*
 *  Special
 *  Byte 3 = 9
 */

#ifdef XK_SPECIAL
#define XK_blank                                       0x9df
#define XK_soliddiamond                                0x9e0
#define XK_checkerboard                                0x9e1
#define XK_ht                                          0x9e2
#define XK_ff                                          0x9e3
#define XK_cr                                          0x9e4
#define XK_lf                                          0x9e5
#define XK_nl                                          0x9e8
#define XK_vt                                          0x9e9
#define XK_lowrightcorner                              0x9ea
#define XK_uprightcorner                               0x9eb
#define XK_upleftcorner                                0x9ec
#define XK_lowleftcorner                               0x9ed
#define XK_crossinglines                               0x9ee
#define XK_horizlinescan1                              0x9ef
#define XK_horizlinescan3                              0x9f0
#define XK_horizlinescan5                              0x9f1
#define XK_horizlinescan7                              0x9f2
#define XK_horizlinescan9                              0x9f3
#define XK_leftt                                       0x9f4
#define XK_rightt                                      0x9f5
#define XK_bott                                        0x9f6
#define XK_topt                                        0x9f7
#define XK_vertbar                                     0x9f8
#endif /* XK_SPECIAL */

/*
 *  Publishing
 *  Byte 3 = a
 */

#ifdef XK_PUBLISHING
#define XK_emspace                                     0xaa1
#define XK_enspace                                     0xaa2
#define XK_em3space                                    0xaa3
#define XK_em4space                                    0xaa4
#define XK_digitspace                                  0xaa5
#define XK_punctspace                                  0xaa6
#define XK_thinspace                                   0xaa7
#define XK_hairspace                                   0xaa8
#define XK_emdash                                      0xaa9
#define XK_endash                                      0xaaa
#define XK_signifblank                                 0xaac
#define XK_ellipsis                                    0xaae
#define XK_doubbaselinedot                             0xaaf
#define XK_onethird                                    0xab0
#define XK_twothirds                                   0xab1
#define XK_onefifth                                    0xab2
#define XK_twofifths                                   0xab3
#define XK_threefifths                                 0xab4
#define XK_fourfifths                                  0xab5
#define XK_onesixth                                    0xab6
#define XK_fivesixths                                  0xab7
#define XK_careof                                      0xab8
#define XK_figdash                                     0xabb
#define XK_leftanglebracket                            0xabc
#define XK_decimalpoint                                0xabd
#define XK_rightanglebracket                           0xabe
#define XK_marker                                      0xabf
#define XK_oneeighth                                   0xac3
#define XK_threeeighths                                0xac4
#define XK_fiveeighths                                 0xac5
#define XK_seveneighths                                0xac6
#define XK_trademark                                   0xac9
#define XK_signaturemark                               0xaca
#define XK_trademarkincircle                           0xacb
#define XK_leftopentriangle                            0xacc
#define XK_rightopentriangle                           0xacd
#define XK_emopencircle                                0xace
#define XK_emopenrectangle                             0xacf
#define XK_leftsinglequotemark                         0xad0
#define XK_rightsinglequotemark                        0xad1
#define XK_leftdoublequotemark                         0xad2
#define XK_rightdoublequotemark                        0xad3
#define XK_prescription                                0xad4
#define XK_minutes                                     0xad6
#define XK_seconds                                     0xad7
#define XK_latincross                                  0xad9
#define XK_hexagram                                    0xada
#define XK_filledrectbullet                            0xadb
#define XK_filledlefttribullet                         0xadc
#define XK_filledrighttribullet                        0xadd
#define XK_emfilledcircle                              0xade
#define XK_emfilledrect                                0xadf
#define XK_enopencircbullet                            0xae0
#define XK_enopensquarebullet                          0xae1
#define XK_openrectbullet                              0xae2
#define XK_opentribulletup                             0xae3
#define XK_opentribulletdown                           0xae4
#define XK_openstar                                    0xae5
#define XK_enfilledcircbullet                          0xae6
#define XK_enfilledsqbullet                            0xae7
#define XK_filledtribulletup                           0xae8
#define XK_filledtribulletdown                         0xae9
#define XK_leftpointer                                 0xaea
#define XK_rightpointer                                0xaeb
#define XK_club                                        0xaec
#define XK_diamond                                     0xaed
#define XK_heart                                       0xaee
#define XK_maltesecross                                0xaf0
#define XK_dagger                                      0xaf1
#define XK_doubledagger                                0xaf2
#define XK_checkmark                                   0xaf3
#define XK_ballotcross                                 0xaf4
#define XK_musicalsharp                                0xaf5
#define XK_musicalflat                                 0xaf6
#define XK_malesymbol                                  0xaf7
#define XK_femalesymbol                                0xaf8
#define XK_telephone                                   0xaf9
#define XK_telephonerecorder                           0xafa
#define XK_phonographcopyright                         0xafb
#define XK_caret                                       0xafc
#define XK_singlelowquotemark                          0xafd
#define XK_doublelowquotemark                          0xafe
#define XK_cursor                                      0xaff
#endif /* XK_PUBLISHING */

/*
 *  APL
 *  Byte 3 = b
 */

#ifdef XK_APL
#define XK_leftcaret                                   0xba3
#define XK_rightcaret                                  0xba6
#define XK_downcaret                                   0xba8
#define XK_upcaret                                     0xba9
#define XK_overbar                                     0xbc0
#define XK_downtack                                    0xbc2
#define XK_upshoe                                      0xbc3
#define XK_downstile                                   0xbc4
#define XK_underbar                                    0xbc6
#define XK_jot                                         0xbca
#define XK_quad                                        0xbcc
#define XK_uptack                                      0xbce
#define XK_circle                                      0xbcf
#define XK_upstile                                     0xbd3
#define XK_downshoe                                    0xbd6
#define XK_rightshoe                                   0xbd8
#define XK_leftshoe                                    0xbda
#define XK_lefttack                                    0xbdc
#define XK_righttack                                   0xbfc
#endif /* XK_APL */

/*
 * Hebrew
 * Byte 3 = c
 */

#ifdef XK_HEBREW
#define XK_hebrew_doublelowline                        0xcdf
#define XK_hebrew_aleph                                0xce0
#define XK_hebrew_bet                                  0xce1
#define XK_hebrew_beth                                 0xce1  /* deprecated */
#define XK_hebrew_gimel                                0xce2
#define XK_hebrew_gimmel                               0xce2  /* deprecated */
#define XK_hebrew_dalet                                0xce3
#define XK_hebrew_daleth                               0xce3  /* deprecated */
#define XK_hebrew_he                                   0xce4
#define XK_hebrew_waw                                  0xce5
#define XK_hebrew_zain                                 0xce6
#define XK_hebrew_zayin                                0xce6  /* deprecated */
#define XK_hebrew_chet                                 0xce7
#define XK_hebrew_het                                  0xce7  /* deprecated */
#define XK_hebrew_tet                                  0xce8
#define XK_hebrew_teth                                 0xce8  /* deprecated */
#define XK_hebrew_yod                                  0xce9
#define XK_hebrew_finalkaph                            0xcea
#define XK_hebrew_kaph                                 0xceb
#define XK_hebrew_lamed                                0xcec
#define XK_hebrew_finalmem                             0xced
#define XK_hebrew_mem                                  0xcee
#define XK_hebrew_finalnun                             0xcef
#define XK_hebrew_nun                                  0xcf0
#define XK_hebrew_samech                               0xcf1
#define XK_hebrew_samekh                               0xcf1  /* deprecated */
#define XK_hebrew_ayin                                 0xcf2
#define XK_hebrew_finalpe                              0xcf3
#define XK_hebrew_pe                                   0xcf4
#define XK_hebrew_finalzade                            0xcf5
#define XK_hebrew_finalzadi                            0xcf5  /* deprecated */
#define XK_hebrew_zade                                 0xcf6
#define XK_hebrew_zadi                                 0xcf6  /* deprecated */
#define XK_hebrew_qoph                                 0xcf7
#define XK_hebrew_kuf                                  0xcf7  /* deprecated */
#define XK_hebrew_resh                                 0xcf8
#define XK_hebrew_shin                                 0xcf9
#define XK_hebrew_taw                                  0xcfa
#define XK_hebrew_taf                                  0xcfa  /* deprecated */
#define XK_Hebrew_switch        0xFF7E  /* Alias for mode_switch */
#endif /* XK_HEBREW */

/* Multimedia keys, defined same as on Linux
 * /usr/include/pkg/libxkbcommon/xkbcommon/xkbcommon-keysyms.h
 */

#define XK_XF86AudioLowerVolume	0x1008FF11   /* Volume control down        */
#define XK_XF86AudioMute	0x1008FF12   /* Mute sound from the system */
#define XK_XF86AudioRaiseVolume	0x1008FF13   /* Volume control up          */
#define XK_XF86AudioPlay	0x1008FF14   /* Start playing of audio >   */
#define XK_XF86AudioStop	0x1008FF15   /* Stop playing audio         */
#define XK_XF86AudioPrev	0x1008FF16   /* Previous track             */
#define XK_XF86AudioNext	0x1008FF17   /* Next track                 */

