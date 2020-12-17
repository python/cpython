/*
 * This file is (mostly) automatically generated from ttk.decls.
 */

#ifndef _TTKDECLS
#define _TTKDECLS

#if defined(USE_TTK_STUBS)

extern const char *TtkInitializeStubs(
	Tcl_Interp *, const char *version, int epoch, int revision);
#define Ttk_InitStubs(interp) TtkInitializeStubs( \
	interp, TTK_VERSION, TTK_STUBS_EPOCH, TTK_STUBS_REVISION)
#else

#define Ttk_InitStubs(interp) Tcl_PkgRequireEx(interp, "Ttk", TTK_VERSION, 0, NULL)

#endif


/* !BEGIN!: Do not edit below this line. */

#define TTK_STUBS_EPOCH 0
#define TTK_STUBS_REVISION 31

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Exported function declarations:
 */

/* 0 */
TTKAPI Ttk_Theme	Ttk_GetTheme(Tcl_Interp *interp, const char *name);
/* 1 */
TTKAPI Ttk_Theme	Ttk_GetDefaultTheme(Tcl_Interp *interp);
/* 2 */
TTKAPI Ttk_Theme	Ttk_GetCurrentTheme(Tcl_Interp *interp);
/* 3 */
TTKAPI Ttk_Theme	Ttk_CreateTheme(Tcl_Interp *interp, const char *name,
				Ttk_Theme parent);
/* 4 */
TTKAPI void		Ttk_RegisterCleanup(Tcl_Interp *interp,
				void *deleteData,
				Ttk_CleanupProc *cleanupProc);
/* 5 */
TTKAPI int		Ttk_RegisterElementSpec(Ttk_Theme theme,
				const char *elementName,
				Ttk_ElementSpec *elementSpec,
				void *clientData);
/* 6 */
TTKAPI Ttk_ElementClass * Ttk_RegisterElement(Tcl_Interp *interp,
				Ttk_Theme theme, const char *elementName,
				Ttk_ElementSpec *elementSpec,
				void *clientData);
/* 7 */
TTKAPI int		Ttk_RegisterElementFactory(Tcl_Interp *interp,
				const char *name,
				Ttk_ElementFactory factoryProc,
				void *clientData);
/* 8 */
TTKAPI void		Ttk_RegisterLayout(Ttk_Theme theme,
				const char *className,
				Ttk_LayoutSpec layoutSpec);
/* Slot 9 is reserved */
/* 10 */
TTKAPI int		Ttk_GetStateSpecFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Ttk_StateSpec *spec_rtn);
/* 11 */
TTKAPI Tcl_Obj *	Ttk_NewStateSpecObj(unsigned int onbits,
				unsigned int offbits);
/* 12 */
TTKAPI Ttk_StateMap	Ttk_GetStateMapFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr);
/* 13 */
TTKAPI Tcl_Obj *	Ttk_StateMapLookup(Tcl_Interp *interp,
				Ttk_StateMap map, Ttk_State state);
/* 14 */
TTKAPI int		Ttk_StateTableLookup(Ttk_StateTable map[],
				Ttk_State state);
/* Slot 15 is reserved */
/* Slot 16 is reserved */
/* Slot 17 is reserved */
/* Slot 18 is reserved */
/* Slot 19 is reserved */
/* 20 */
TTKAPI int		Ttk_GetPaddingFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr,
				Ttk_Padding *pad_rtn);
/* 21 */
TTKAPI int		Ttk_GetBorderFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Ttk_Padding *pad_rtn);
/* 22 */
TTKAPI int		Ttk_GetStickyFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Ttk_Sticky *sticky_rtn);
/* 23 */
TTKAPI Ttk_Padding	Ttk_MakePadding(short l, short t, short r, short b);
/* 24 */
TTKAPI Ttk_Padding	Ttk_UniformPadding(short borderWidth);
/* 25 */
TTKAPI Ttk_Padding	Ttk_AddPadding(Ttk_Padding pad1, Ttk_Padding pad2);
/* 26 */
TTKAPI Ttk_Padding	Ttk_RelievePadding(Ttk_Padding padding, int relief,
				int n);
/* 27 */
TTKAPI Ttk_Box		Ttk_MakeBox(int x, int y, int width, int height);
/* 28 */
TTKAPI int		Ttk_BoxContains(Ttk_Box box, int x, int y);
/* 29 */
TTKAPI Ttk_Box		Ttk_PackBox(Ttk_Box *cavity, int w, int h,
				Ttk_Side side);
/* 30 */
TTKAPI Ttk_Box		Ttk_StickBox(Ttk_Box parcel, int w, int h,
				Ttk_Sticky sticky);
/* 31 */
TTKAPI Ttk_Box		Ttk_AnchorBox(Ttk_Box parcel, int w, int h,
				Tk_Anchor anchor);
/* 32 */
TTKAPI Ttk_Box		Ttk_PadBox(Ttk_Box b, Ttk_Padding p);
/* 33 */
TTKAPI Ttk_Box		Ttk_ExpandBox(Ttk_Box b, Ttk_Padding p);
/* 34 */
TTKAPI Ttk_Box		Ttk_PlaceBox(Ttk_Box *cavity, int w, int h,
				Ttk_Side side, Ttk_Sticky sticky);
/* 35 */
TTKAPI Tcl_Obj *	Ttk_NewBoxObj(Ttk_Box box);
/* Slot 36 is reserved */
/* Slot 37 is reserved */
/* Slot 38 is reserved */
/* Slot 39 is reserved */
/* 40 */
TTKAPI int		Ttk_GetOrientFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *orient);

typedef struct TtkStubs {
    int magic;
    int epoch;
    int revision;
    void *hooks;

    Ttk_Theme (*ttk_GetTheme) (Tcl_Interp *interp, const char *name); /* 0 */
    Ttk_Theme (*ttk_GetDefaultTheme) (Tcl_Interp *interp); /* 1 */
    Ttk_Theme (*ttk_GetCurrentTheme) (Tcl_Interp *interp); /* 2 */
    Ttk_Theme (*ttk_CreateTheme) (Tcl_Interp *interp, const char *name, Ttk_Theme parent); /* 3 */
    void (*ttk_RegisterCleanup) (Tcl_Interp *interp, void *deleteData, Ttk_CleanupProc *cleanupProc); /* 4 */
    int (*ttk_RegisterElementSpec) (Ttk_Theme theme, const char *elementName, Ttk_ElementSpec *elementSpec, void *clientData); /* 5 */
    Ttk_ElementClass * (*ttk_RegisterElement) (Tcl_Interp *interp, Ttk_Theme theme, const char *elementName, Ttk_ElementSpec *elementSpec, void *clientData); /* 6 */
    int (*ttk_RegisterElementFactory) (Tcl_Interp *interp, const char *name, Ttk_ElementFactory factoryProc, void *clientData); /* 7 */
    void (*ttk_RegisterLayout) (Ttk_Theme theme, const char *className, Ttk_LayoutSpec layoutSpec); /* 8 */
    void (*reserved9)(void);
    int (*ttk_GetStateSpecFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Ttk_StateSpec *spec_rtn); /* 10 */
    Tcl_Obj * (*ttk_NewStateSpecObj) (unsigned int onbits, unsigned int offbits); /* 11 */
    Ttk_StateMap (*ttk_GetStateMapFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr); /* 12 */
    Tcl_Obj * (*ttk_StateMapLookup) (Tcl_Interp *interp, Ttk_StateMap map, Ttk_State state); /* 13 */
    int (*ttk_StateTableLookup) (Ttk_StateTable map[], Ttk_State state); /* 14 */
    void (*reserved15)(void);
    void (*reserved16)(void);
    void (*reserved17)(void);
    void (*reserved18)(void);
    void (*reserved19)(void);
    int (*ttk_GetPaddingFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, Ttk_Padding *pad_rtn); /* 20 */
    int (*ttk_GetBorderFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Ttk_Padding *pad_rtn); /* 21 */
    int (*ttk_GetStickyFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Ttk_Sticky *sticky_rtn); /* 22 */
    Ttk_Padding (*ttk_MakePadding) (short l, short t, short r, short b); /* 23 */
    Ttk_Padding (*ttk_UniformPadding) (short borderWidth); /* 24 */
    Ttk_Padding (*ttk_AddPadding) (Ttk_Padding pad1, Ttk_Padding pad2); /* 25 */
    Ttk_Padding (*ttk_RelievePadding) (Ttk_Padding padding, int relief, int n); /* 26 */
    Ttk_Box (*ttk_MakeBox) (int x, int y, int width, int height); /* 27 */
    int (*ttk_BoxContains) (Ttk_Box box, int x, int y); /* 28 */
    Ttk_Box (*ttk_PackBox) (Ttk_Box *cavity, int w, int h, Ttk_Side side); /* 29 */
    Ttk_Box (*ttk_StickBox) (Ttk_Box parcel, int w, int h, Ttk_Sticky sticky); /* 30 */
    Ttk_Box (*ttk_AnchorBox) (Ttk_Box parcel, int w, int h, Tk_Anchor anchor); /* 31 */
    Ttk_Box (*ttk_PadBox) (Ttk_Box b, Ttk_Padding p); /* 32 */
    Ttk_Box (*ttk_ExpandBox) (Ttk_Box b, Ttk_Padding p); /* 33 */
    Ttk_Box (*ttk_PlaceBox) (Ttk_Box *cavity, int w, int h, Ttk_Side side, Ttk_Sticky sticky); /* 34 */
    Tcl_Obj * (*ttk_NewBoxObj) (Ttk_Box box); /* 35 */
    void (*reserved36)(void);
    void (*reserved37)(void);
    void (*reserved38)(void);
    void (*reserved39)(void);
    int (*ttk_GetOrientFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *orient); /* 40 */
} TtkStubs;

extern const TtkStubs *ttkStubsPtr;

#ifdef __cplusplus
}
#endif

#if defined(USE_TTK_STUBS)

/*
 * Inline function declarations:
 */

#define Ttk_GetTheme \
	(ttkStubsPtr->ttk_GetTheme) /* 0 */
#define Ttk_GetDefaultTheme \
	(ttkStubsPtr->ttk_GetDefaultTheme) /* 1 */
#define Ttk_GetCurrentTheme \
	(ttkStubsPtr->ttk_GetCurrentTheme) /* 2 */
#define Ttk_CreateTheme \
	(ttkStubsPtr->ttk_CreateTheme) /* 3 */
#define Ttk_RegisterCleanup \
	(ttkStubsPtr->ttk_RegisterCleanup) /* 4 */
#define Ttk_RegisterElementSpec \
	(ttkStubsPtr->ttk_RegisterElementSpec) /* 5 */
#define Ttk_RegisterElement \
	(ttkStubsPtr->ttk_RegisterElement) /* 6 */
#define Ttk_RegisterElementFactory \
	(ttkStubsPtr->ttk_RegisterElementFactory) /* 7 */
#define Ttk_RegisterLayout \
	(ttkStubsPtr->ttk_RegisterLayout) /* 8 */
/* Slot 9 is reserved */
#define Ttk_GetStateSpecFromObj \
	(ttkStubsPtr->ttk_GetStateSpecFromObj) /* 10 */
#define Ttk_NewStateSpecObj \
	(ttkStubsPtr->ttk_NewStateSpecObj) /* 11 */
#define Ttk_GetStateMapFromObj \
	(ttkStubsPtr->ttk_GetStateMapFromObj) /* 12 */
#define Ttk_StateMapLookup \
	(ttkStubsPtr->ttk_StateMapLookup) /* 13 */
#define Ttk_StateTableLookup \
	(ttkStubsPtr->ttk_StateTableLookup) /* 14 */
/* Slot 15 is reserved */
/* Slot 16 is reserved */
/* Slot 17 is reserved */
/* Slot 18 is reserved */
/* Slot 19 is reserved */
#define Ttk_GetPaddingFromObj \
	(ttkStubsPtr->ttk_GetPaddingFromObj) /* 20 */
#define Ttk_GetBorderFromObj \
	(ttkStubsPtr->ttk_GetBorderFromObj) /* 21 */
#define Ttk_GetStickyFromObj \
	(ttkStubsPtr->ttk_GetStickyFromObj) /* 22 */
#define Ttk_MakePadding \
	(ttkStubsPtr->ttk_MakePadding) /* 23 */
#define Ttk_UniformPadding \
	(ttkStubsPtr->ttk_UniformPadding) /* 24 */
#define Ttk_AddPadding \
	(ttkStubsPtr->ttk_AddPadding) /* 25 */
#define Ttk_RelievePadding \
	(ttkStubsPtr->ttk_RelievePadding) /* 26 */
#define Ttk_MakeBox \
	(ttkStubsPtr->ttk_MakeBox) /* 27 */
#define Ttk_BoxContains \
	(ttkStubsPtr->ttk_BoxContains) /* 28 */
#define Ttk_PackBox \
	(ttkStubsPtr->ttk_PackBox) /* 29 */
#define Ttk_StickBox \
	(ttkStubsPtr->ttk_StickBox) /* 30 */
#define Ttk_AnchorBox \
	(ttkStubsPtr->ttk_AnchorBox) /* 31 */
#define Ttk_PadBox \
	(ttkStubsPtr->ttk_PadBox) /* 32 */
#define Ttk_ExpandBox \
	(ttkStubsPtr->ttk_ExpandBox) /* 33 */
#define Ttk_PlaceBox \
	(ttkStubsPtr->ttk_PlaceBox) /* 34 */
#define Ttk_NewBoxObj \
	(ttkStubsPtr->ttk_NewBoxObj) /* 35 */
/* Slot 36 is reserved */
/* Slot 37 is reserved */
/* Slot 38 is reserved */
/* Slot 39 is reserved */
#define Ttk_GetOrientFromObj \
	(ttkStubsPtr->ttk_GetOrientFromObj) /* 40 */

#endif /* defined(USE_TTK_STUBS) */

/* !END!: Do not edit above this line. */

#endif /* _TTKDECLS */
