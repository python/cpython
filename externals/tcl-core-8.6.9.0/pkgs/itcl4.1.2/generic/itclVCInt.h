#define ITCL_RESOLVE_DATA "ITCL_Resolve_Info"

typedef struct ItclResolvngInfo {
    Tcl_Interp *interp;
    Tcl_HashTable resolveVars;    /* all possible names for variables in
                                   * this class (e.g., x, foo::x, etc.) */
    Tcl_HashTable resolveCmds;    /* all possible names for functions in
                                   * this class (e.g., x, foo::x, etc.) */
    ItclCheckClassProtection *varProtFcn;
    ItclCheckClassProtection *cmdProtFcn;
    Tcl_HashTable objectVarsTables;
    Tcl_HashTable objectCmdsTables;
} ItclResolvingInfo;

typedef struct ObjectVarInfo {
    ClientData clientData;
    ItclObject *ioPtr;
    Tcl_Var varPtr;
} ObjectVarInfo;

typedef struct ObjectVarTableInfo {
    Tcl_HashTable varInfos;
    TclVarHashTable *tablePtr;
} ObjectVarTableInfo;

typedef struct ObjectCmdInfo {
    ClientData clientData;
    ItclObject *ioPtr;
    Tcl_Command cmdPtr;
} ObjectCmdInfo;

typedef struct ObjectCmdTableInfo {
    Tcl_HashTable cmdInfos;
    Tcl_HashTable *tablePtr;
} ObjectCmdTableInfo;


