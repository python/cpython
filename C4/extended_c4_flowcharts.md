# CPython Extended C4 Model with Detailed Execution Flowcharts

This document extends the original C4 model with detailed flowcharts showing the complete execution flow from Python source code to the lowest level, including specific file names and function calls.

## Level 1: System Context with Execution Flow

```mermaid
graph TB
    subgraph "External Users"
        DEV[Python Developers<br/>Writing Python Code]
    end
    
    subgraph "CPython System - Complete Execution Flow"
        MAIN[Program Entry Point<br/>Programs/python.c:main()]
        PYTHONRUN[Python Runtime<br/>Python/pythonrun.c]
        PARSER[Parser System<br/>Parser/]
        COMPILER[Compiler System<br/>Python/compile.c]
        VM[Virtual Machine<br/>Python/ceval.c]
        OBJECTS[Object System<br/>Objects/]
        MEMORY[Memory Management<br/>Python/gc.c, pymem.c]
    end
    
    DEV -->|"Python Source Code"| MAIN
    MAIN -->|"Py_Main()"| PYTHONRUN
    PYTHONRUN -->|"Parse & Compile"| PARSER
    PARSER -->|"AST"| COMPILER
    COMPILER -->|"Bytecode"| VM
    VM -->|"Object Operations"| OBJECTS
    OBJECTS -->|"Memory Requests"| MEMORY
```

## Level 2: Container Diagram with File-Level Flow

```mermaid
graph TB
    subgraph "CPython Execution Pipeline"
        subgraph "Entry & Runtime"
            MAIN_FILE["Programs/python.c<br/>main() → Py_Main()"]
            RUN_FILE["Python/pythonrun.c<br/>PyRun_AnyFileObject()"]
        end
        
        subgraph "Parsing Pipeline"
            PARSE_API["Parser/peg_api.c<br/>_PyParser_ASTFromString()"]
            PEGEN["Parser/pegen.c<br/>_PyPegen_run_parser()"]
            TOKENIZER["Parser/tokenizer/<br/>Token Generation"]
            GRAMMAR["Grammar/python.gram<br/>PEG Grammar Rules"]
        end
        
        subgraph "Compilation Pipeline"
            COMPILE["Python/compile.c<br/>_PyAST_Compile()"]
            CODEGEN["Python/codegen.c<br/>Instruction Generation"]
            FLOWGRAPH["Python/flowgraph.c<br/>Control Flow Graph"]
            ASSEMBLER["Python/assemble.c<br/>Bytecode Assembly"]
        end
        
        subgraph "Execution Pipeline"
            CEVAL["Python/ceval.c<br/>_PyEval_EvalFrameDefault()"]
            OPTIMIZER["Python/optimizer.c<br/>_PyOptimizer_Optimize()"]
            BYTECODES["Python/bytecodes.c<br/>Opcode Definitions"]
        end
        
        subgraph "Object & Memory"
            OBJECTS_FILE["Objects/object.c<br/>PyObject Operations"]
            GC_FILE["Python/gc.c<br/>Garbage Collection"]
            MEM_FILE["Python/pymem.c<br/>Memory Allocation"]
        end
    end
    
    MAIN_FILE -->|"File Processing"| RUN_FILE
    RUN_FILE -->|"Source → AST"| PARSE_API
    PARSE_API -->|"Parse Tree"| PEGEN
    PEGEN -->|"Tokens"| TOKENIZER
    TOKENIZER -->|"Grammar Rules"| GRAMMAR
    PEGEN -->|"AST"| COMPILE
    COMPILE -->|"Instructions"| CODEGEN
    CODEGEN -->|"CFG"| FLOWGRAPH
    FLOWGRAPH -->|"Bytecode"| ASSEMBLER
    ASSEMBLER -->|"CodeObject"| CEVAL
    CEVAL -->|"Hot Code"| OPTIMIZER
    CEVAL -->|"Opcode Dispatch"| BYTECODES
    CEVAL -->|"Object Creation"| OBJECTS_FILE
    OBJECTS_FILE -->|"GC Triggers"| GC_FILE
    OBJECTS_FILE -->|"Memory Requests"| MEM_FILE
```

## Level 3: Detailed Execution Flowcharts

### Complete Python Code Execution Flow

```mermaid
flowchart TD
    START([Python Source Code<br/>print('Hello World')]) --> MAIN_ENTRY[Programs/python.c:main]
    
    MAIN_ENTRY --> PY_MAIN[Py_Main in Python/pythonrun.c]
    PY_MAIN --> RUN_FILE[PyRun_AnyFileObject]
    
    RUN_FILE --> CHECK_INTERACTIVE{Interactive Mode?}
    CHECK_INTERACTIVE -->|Yes| INTERACTIVE[PyRun_InteractiveLoopObject]
    CHECK_INTERACTIVE -->|No| SIMPLE_FILE[PyRun_SimpleFileObject]
    
    INTERACTIVE --> PARSE_STRING[PyRun_StringFlagsWithName]
    SIMPLE_FILE --> PARSE_STRING
    
    PARSE_STRING --> PARSE_API[Parser/peg_api.c:<br/>_PyParser_ASTFromString]
    
    PARSE_API --> TOKENIZE[Parser/tokenizer/tokenizer.c:<br/>_PyTokenizer_FromString]
    TOKENIZE --> TOKENS[Token Stream]
    
    TOKENS --> PEG_PARSE[Parser/pegen.c:<br/>_PyPegen_run_parser]
    PEG_PARSE --> GRAMMAR_CHECK[Grammar/python.gram:<br/>PEG Grammar Rules]
    GRAMMAR_CHECK --> AST_BUILD[AST Construction]
    
    AST_BUILD --> AST_VALIDATE[AST Validation]
    AST_VALIDATE --> COMPILE_ENTRY[Python/compile.c:<br/>_PyAST_Compile]
    
    COMPILE_ENTRY --> SYMTABLE[Python/symtable.c:<br/>Symbol Table Building]
    SYMTABLE --> CODEGEN_ENTRY[Python/codegen.c:<br/>Instruction Generation]
    
    CODEGEN_ENTRY --> FLOWGRAPH_BUILD[Python/flowgraph.c:<br/>Control Flow Graph]
    FLOWGRAPH_BUILD --> OPTIMIZE[Python/assemble.c:<br/>Optimize and Assemble]
    OPTIMIZE --> BYTECODE[Bytecode Generation]
    
    BYTECODE --> CODE_OBJECT[PyCodeObject Creation]
    CODE_OBJECT --> EXEC_ENTRY[Python/pythonrun.c:<br/>run_eval_code_obj]
    
    EXEC_ENTRY --> EVAL_CODE[Python/ceval.c:<br/>PyEval_EvalCode]
    EVAL_CODE --> FRAME_CREATE[Frame Creation]
    FRAME_CREATE --> EVAL_FRAME[Python/ceval.c:<br/>_PyEval_EvalFrameDefault]
    
    EVAL_FRAME --> OPCODE_LOOP[Main Opcode Loop]
    OPCODE_LOOP --> DISPATCH[Opcode Dispatch<br/>Python/bytecodes.c]
    
    DISPATCH --> CHECK_HOT{Hot Code?}
    CHECK_HOT -->|Yes| OPTIMIZER[Python/optimizer.c:<br/>_PyOptimizer_Optimize]
    CHECK_HOT -->|No| EXECUTE_OP[Execute Opcode]
    
    OPTIMIZER --> TIER2[Python/tier2_engine.md:<br/>Tier 2 Interpreter]
    TIER2 --> EXECUTE_OP
    
    EXECUTE_OP --> OBJECT_OPS[Objects/object.c:<br/>Object Operations]
    OBJECT_OPS --> MEMORY_ALLOC[Python/pymem.c:<br/>Memory Allocation]
    MEMORY_ALLOC --> GC_CHECK[Python/gc.c:<br/>Garbage Collection]
    
    GC_CHECK --> STACK_UPDATE[Stack Operations]
    STACK_UPDATE --> NEXT_OPCODE{More Opcodes?}
    NEXT_OPCODE -->|Yes| OPCODE_LOOP
    NEXT_OPCODE -->|No| RETURN_RESULT[Return Result]
    
    RETURN_RESULT --> END([Execution Complete])
```

### Parser Detailed Flow

```mermaid
flowchart TD
    SOURCE[Python Source Code] --> PARSE_API[Parser/peg_api.c:<br/>_PyParser_ASTFromString]
    
    PARSE_API --> AUDIT_CHECK[PySys_Audit Check]
    AUDIT_CHECK --> PEGEN_CALL[Parser/pegen.c:<br/>_PyPegen_run_parser_from_string]
    
    PEGEN_CALL --> TOKENIZER_CREATE[Parser/tokenizer/tokenizer.c:<br/>_PyTokenizer_FromString]
    TOKENIZER_CREATE --> TOKEN_STATE[Token State Creation]
    
    TOKEN_STATE --> PARSER_NEW[Parser/pegen.c:<br/>_PyPegen_Parser_New]
    PARSER_NEW --> PARSER_RUN[Parser/pegen.c:<br/>_PyPegen_run_parser]
    
    PARSER_RUN --> PARSE_LOOP[Parse Loop]
    PARSE_LOOP --> GRAMMAR_RULES[Grammar/python.gram:<br/>PEG Rules]
    
    GRAMMAR_RULES --> TOKEN_MATCH[Token Matching]
    TOKEN_MATCH --> AST_NODE[AST Node Creation]
    
    AST_NODE --> VALIDATE_AST[AST Validation<br/>Parser/asdl_c.py]
    VALIDATE_AST --> AST_COMPLETE[Complete AST]
    
    AST_COMPLETE --> RETURN_AST[Return AST to Compiler]
```

### Compiler Detailed Flow

```mermaid
flowchart TD
    AST_INPUT[AST from Parser] --> COMPILE_ENTRY[Python/compile.c:<br/>_PyAST_Compile]
    
    COMPILE_ENTRY --> ARENA_CREATE[PyArena Creation]
    ARENA_CREATE --> COMPILER_NEW[new_compiler]
    
    COMPILER_NEW --> FUTURE_CHECK[Python/future.c:<br/>Future Statement Check]
    FUTURE_CHECK --> SYMTABLE_BUILD[Python/symtable.c:<br/>Symbol Table Building]
    
    SYMTABLE_BUILD --> COMPILER_MOD[compiler_mod]
    COMPILER_MOD --> CODEGEN_CALL[compiler_codegen]
    
    CODEGEN_CALL --> CODEGEN_ENTRY[Python/codegen.c:<br/>_PyCodegen_Module]
    CODEGEN_ENTRY --> INSTR_GEN[Instruction Generation]
    
    INSTR_GEN --> FLOWGRAPH_BUILD[Python/flowgraph.c:<br/>CFG Building]
    FLOWGRAPH_BUILD --> OPTIMIZE_ASSEMBLE[Python/assemble.c:<br/>optimize_and_assemble]
    
    OPTIMIZE_ASSEMBLE --> BYTECODE_GEN[Bytecode Generation]
    BYTECODE_GEN --> CODE_OBJECT_CREATE[PyCodeObject Creation]
    
    CODE_OBJECT_CREATE --> RETURN_CODE[Return CodeObject]
```

### Virtual Machine Execution Flow

```mermaid
flowchart TD
    CODE_OBJECT[PyCodeObject] --> EVAL_ENTRY[Python/ceval.c:<br/>PyEval_EvalCode]
    
    EVAL_ENTRY --> FRAME_CREATE[Frame Creation<br/>Python/frame.c]
    FRAME_CREATE --> EVAL_FRAME[Python/ceval.c:<br/>_PyEval_EvalFrameDefault]
    
    EVAL_FRAME --> RECURSION_CHECK[Recursion Check]
    RECURSION_CHECK --> FRAME_SETUP[Frame Setup]
    
    FRAME_SETUP --> OPCODE_LOOP[Main Opcode Loop]
    OPCODE_LOOP --> FETCH_OPCODE[Fetch Opcode]
    
    FETCH_OPCODE --> DISPATCH[Opcode Dispatch<br/>Python/bytecodes.c]
    DISPATCH --> SPECIALIZE{Specialized?}
    
    SPECIALIZE -->|Yes| SPECIALIZED_EXEC[Specialized Execution<br/>Python/specialize.c]
    SPECIALIZE -->|No| GENERIC_EXEC[Generic Execution]
    
    SPECIALIZED_EXEC --> HOT_CHECK{Hot Code?}
    GENERIC_EXEC --> HOT_CHECK
    
    HOT_CHECK -->|Yes| OPTIMIZER_CALL[Python/optimizer.c:<br/>_PyOptimizer_Optimize]
    HOT_CHECK -->|No| EXECUTE_INSTR[Execute Instruction]
    
    OPTIMIZER_CALL --> TIER2_CREATE[Tier 2 Executor Creation]
    TIER2_CREATE --> TIER2_EXEC[Python/tier2_engine.md:<br/>Tier 2 Execution]
    TIER2_EXEC --> EXECUTE_INSTR
    
    EXECUTE_INSTR --> OBJECT_CREATE[Objects/object.c:<br/>Object Creation]
    OBJECT_CREATE --> MEMORY_ALLOC[Python/pymem.c:<br/>Memory Allocation]
    
    MEMORY_ALLOC --> GC_TRIGGER[Python/gc.c:<br/>GC Check]
    GC_TRIGGER --> STACK_OP[Stack Operations]
    
    STACK_OP --> EXCEPTION_CHECK{Exception?}
    EXCEPTION_CHECK -->|Yes| EXCEPTION_HANDLE[Exception Handling]
    EXCEPTION_CHECK -->|No| NEXT_INSTR{More Instructions?}
    
    EXCEPTION_HANDLE --> UNWIND[Stack Unwinding]
    UNWIND --> NEXT_INSTR
    
    NEXT_INSTR -->|Yes| OPCODE_LOOP
    NEXT_INSTR -->|No| RETURN_VALUE[Return Value]
```

### Memory Management Flow

```mermaid
flowchart TD
    ALLOC_REQUEST[Memory Allocation Request] --> ALLOC_ENTRY[Python/pymem.c:<br/>_PyObject_Malloc]
    
    ALLOC_ENTRY --> SIZE_CHECK{Object Size?}
    SIZE_CHECK -->|Small| SMALL_ALLOC[Small Object Allocator]
    SIZE_CHECK -->|Large| LARGE_ALLOC[Large Object Allocator]
    
    SMALL_ALLOC --> POOL_CHECK[Pool Check]
    POOL_CHECK --> FREE_LIST{Free List Available?}
    FREE_LIST -->|Yes| REUSE_OBJECT[Reuse Object from Free List]
    FREE_LIST -->|No| NEW_ALLOC[New Allocation]
    
    LARGE_ALLOC --> ARENA_ALLOC[Python/pyarena.c:<br/>Arena Allocation]
    ARENA_ALLOC --> NEW_ALLOC
    
    NEW_ALLOC --> SYSTEM_MALLOC[System malloc]
    SYSTEM_MALLOC --> OBJECT_INIT[Object Initialization]
    
    REUSE_OBJECT --> OBJECT_INIT
    OBJECT_INIT --> GC_TRACK[Python/gc.c:<br/>GC Tracking]
    
    GC_TRACK --> REF_COUNT[Reference Counting]
    REF_COUNT --> GC_CHECK{GC Needed?}
    
    GC_CHECK -->|Yes| GC_COLLECT[Python/gc.c:<br/>_PyGC_Collect]
    GC_CHECK -->|No| RETURN_OBJECT[Return Object]
    
    GC_COLLECT --> GENERATION_CHECK[Generation Check]
    GENERATION_CHECK --> CYCLE_DETECT[Cycle Detection]
    CYCLE_DETECT --> FINALIZE[Object Finalization]
    FINALIZE --> FREE_MEMORY[Free Memory]
    FREE_MEMORY --> RETURN_OBJECT
```

### Object System Flow

```mermaid
flowchart TD
    OBJECT_CREATE[Object Creation Request] --> TYPE_LOOKUP[Type Lookup<br/>Objects/typeobject.c]
    
    TYPE_LOOKUP --> ALLOC_OBJECT[Objects/object.c:<br/>_PyObject_New]
    ALLOC_OBJECT --> INIT_OBJECT[Object Initialization]
    
    INIT_OBJECT --> REF_COUNT_INIT[Reference Count = 1]
    REF_COUNT_INIT --> TYPE_SET[Set Object Type]
    
    TYPE_SET --> METHOD_RESOLUTION[Method Resolution Order<br/>Objects/typeobject.c]
    METHOD_RESOLUTION --> DESCRIPTOR_CHECK[Descriptor Protocol<br/>Objects/descrobject.c]
    
    DESCRIPTOR_CHECK --> ATTRIBUTE_ACCESS[Attribute Access]
    ATTRIBUTE_ACCESS --> OPERATION_EXEC[Operation Execution]
    
    OPERATION_EXEC --> REF_COUNT_UPDATE[Reference Count Update]
    REF_COUNT_UPDATE --> GC_CHECK{GC Object?}
    
    GC_CHECK -->|Yes| GC_TRACK[Python/gc.c:<br/>GC Tracking]
    GC_CHECK -->|No| OPERATION_COMPLETE[Operation Complete]
    
    GC_TRACK --> OPERATION_COMPLETE
    OPERATION_COMPLETE --> RETURN_RESULT[Return Result]
```

## Level 4: File-Level Code Structure with Execution Paths

### Main Entry Point Code Flow

```mermaid
graph TB
    subgraph "Programs/python.c"
        MAIN_FUNC["main()<br/>Entry Point"]
        PY_MAIN_CALL["Py_Main()<br/>Main Python Function"]
    end
    
    subgraph "Python/pythonrun.c"
        PY_MAIN_IMPL["Py_Main()<br/>Implementation"]
        RUN_ANY_FILE["PyRun_AnyFileObject()<br/>File Processing"]
        RUN_STRING["PyRun_StringFlagsWithName()<br/>String Processing"]
        RUN_MOD["run_mod()<br/>Module Execution"]
    end
    
    subgraph "Parser/peg_api.c"
        PARSE_AST["_PyParser_ASTFromString()<br/>AST Generation"]
    end
    
    subgraph "Python/compile.c"
        AST_COMPILE["_PyAST_Compile()<br/>Compilation"]
        COMPILER_MOD["compiler_mod()<br/>Module Compilation"]
    end
    
    subgraph "Python/ceval.c"
        EVAL_CODE["PyEval_EvalCode()<br/>Code Evaluation"]
        EVAL_FRAME["_PyEval_EvalFrameDefault()<br/>Frame Evaluation"]
    end
    
    MAIN_FUNC --> PY_MAIN_CALL
    PY_MAIN_CALL --> PY_MAIN_IMPL
    PY_MAIN_IMPL --> RUN_ANY_FILE
    RUN_ANY_FILE --> RUN_STRING
    RUN_STRING --> PARSE_AST
    PARSE_AST --> AST_COMPILE
    AST_COMPILE --> COMPILER_MOD
    COMPILER_MOD --> RUN_MOD
    RUN_MOD --> EVAL_CODE
    EVAL_CODE --> EVAL_FRAME
```

### Parser Code Flow

```mermaid
graph TB
    subgraph "Parser/peg_api.c"
        PARSE_STRING["_PyParser_ASTFromString()"]
        PARSE_FILE["_PyParser_ASTFromFile()"]
    end
    
    subgraph "Parser/pegen.c"
        PEGEN_RUN["_PyPegen_run_parser()"]
        PEGEN_STRING["_PyPegen_run_parser_from_string()"]
        PEGEN_FILE["_PyPegen_run_parser_from_file_pointer()"]
    end
    
    subgraph "Parser/tokenizer/"
        TOKENIZER_STRING["_PyTokenizer_FromString()"]
        TOKENIZER_FILE["_PyTokenizer_FromFile()"]
        TOKENIZER_FREE["_PyTokenizer_Free()"]
    end
    
    subgraph "Grammar/python.gram"
        GRAMMAR_RULES["PEG Grammar Rules"]
    end
    
    PARSE_STRING --> PEGEN_STRING
    PARSE_FILE --> PEGEN_FILE
    PEGEN_STRING --> TOKENIZER_STRING
    PEGEN_FILE --> TOKENIZER_FILE
    TOKENIZER_STRING --> PEGEN_RUN
    TOKENIZER_FILE --> PEGEN_RUN
    PEGEN_RUN --> GRAMMAR_RULES
    PEGEN_RUN --> TOKENIZER_FREE
```

### Compiler Code Flow

```mermaid
graph TB
    subgraph "Python/compile.c"
        AST_COMPILE["_PyAST_Compile()"]
        COMPILER_NEW["new_compiler()"]
        COMPILER_MOD["compiler_mod()"]
        COMPILER_CODEGEN["compiler_codegen()"]
    end
    
    subgraph "Python/symtable.c"
        SYMTABLE_BUILD["PySymtable_Build()"]
        SYMTABLE_ENTRY["PySTEntry_*"]
    end
    
    subgraph "Python/codegen.c"
        CODEGEN_MODULE["_PyCodegen_Module()"]
        CODEGEN_EXPR["_PyCodegen_Expression()"]
        INSTR_GEN["Instruction Generation"]
    end
    
    subgraph "Python/flowgraph.c"
        CFG_BUILD["_PyCfg_FromInstructionSequence()"]
        CFG_OPTIMIZE["CFG Optimization"]
    end
    
    subgraph "Python/assemble.c"
        OPTIMIZE_ASSEMBLE["optimize_and_assemble()"]
        BYTECODE_ASSEMBLE["Bytecode Assembly"]
    end
    
    AST_COMPILE --> COMPILER_NEW
    COMPILER_NEW --> SYMTABLE_BUILD
    SYMTABLE_BUILD --> SYMTABLE_ENTRY
    SYMTABLE_ENTRY --> COMPILER_MOD
    COMPILER_MOD --> COMPILER_CODEGEN
    COMPILER_CODEGEN --> CODEGEN_MODULE
    CODEGEN_MODULE --> INSTR_GEN
    INSTR_GEN --> CFG_BUILD
    CFG_BUILD --> CFG_OPTIMIZE
    CFG_OPTIMIZE --> OPTIMIZE_ASSEMBLE
    OPTIMIZE_ASSEMBLE --> BYTECODE_ASSEMBLE
```

### Virtual Machine Code Flow

```mermaid
graph TB
    subgraph "Python/ceval.c"
        EVAL_CODE["PyEval_EvalCode()"]
        EVAL_FRAME["_PyEval_EvalFrameDefault()"]
        OPCODE_LOOP["Main Opcode Loop"]
        DISPATCH["Opcode Dispatch"]
    end
    
    subgraph "Python/bytecodes.c"
        OPCODE_DEFS["Opcode Definitions"]
        SPECIALIZED_CASES["Specialized Cases"]
    end
    
    subgraph "Python/specialize.c"
        SPECIALIZE_OP["Specialization Logic"]
        ADAPTIVE_SPEC["Adaptive Specialization"]
    end
    
    subgraph "Python/optimizer.c"
        OPTIMIZER_OPT["_PyOptimizer_Optimize()"]
        TRACE_BUILD["Trace Building"]
        TIER2_CREATE["Tier 2 Executor Creation"]
    end
    
    subgraph "Python/tier2_engine.md"
        TIER2_EXEC["Tier 2 Execution"]
        MICRO_OPS["Micro-op Interpreter"]
    end
    
    EVAL_CODE --> EVAL_FRAME
    EVAL_FRAME --> OPCODE_LOOP
    OPCODE_LOOP --> DISPATCH
    DISPATCH --> OPCODE_DEFS
    OPCODE_DEFS --> SPECIALIZED_CASES
    SPECIALIZED_CASES --> SPECIALIZE_OP
    SPECIALIZE_OP --> ADAPTIVE_SPEC
    ADAPTIVE_SPEC --> OPTIMIZER_OPT
    OPTIMIZER_OPT --> TRACE_BUILD
    TRACE_BUILD --> TIER2_CREATE
    TIER2_CREATE --> TIER2_EXEC
    TIER2_EXEC --> MICRO_OPS
```

## Key Execution Paths Summary

### 1. **Source Code to Execution Path**
```
Programs/python.c:main()
  ↓
Python/pythonrun.c:Py_Main()
  ↓
Python/pythonrun.c:PyRun_AnyFileObject()
  ↓
Parser/peg_api.c:_PyParser_ASTFromString()
  ↓
Parser/pegen.c:_PyPegen_run_parser()
  ↓
Python/compile.c:_PyAST_Compile()
  ↓
Python/ceval.c:PyEval_EvalCode()
  ↓
Python/ceval.c:_PyEval_EvalFrameDefault()
```

### 2. **Memory Allocation Path**
```
Objects/object.c:_PyObject_New()
  ↓
Python/pymem.c:_PyObject_Malloc()
  ↓
Python/gc.c:_PyObject_GC_Link()
  ↓
Python/pyarena.c:Arena Management
```

### 3. **Optimization Path**
```
Python/ceval.c:Hot Code Detection
  ↓
Python/optimizer.c:_PyOptimizer_Optimize()
  ↓
Python/tier2_engine.md:Tier 2 Execution
  ↓
Python/jit.c:JIT Compilation (Future)
```

This comprehensive flowchart shows the complete execution path from Python source code through parsing, compilation, optimization, and execution, with specific file names and function calls at each level.
