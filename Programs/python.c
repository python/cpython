#include "Python.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <ctype.h>
#include "llama.h"

// ----------------------------------------------------------------------------
// Phase 3: Hash Generation & Verification
// ----------------------------------------------------------------------------
int compute_sha256(const char *path, char *output_hash) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), f)) != 0) {
        SHA256_Update(&ctx, buffer, bytes);
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);
    fclose(f);
    
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output_hash + (i * 2), "%02x", hash[i]);
    }
    output_hash[64] = '\0';
    return 0;
}

char* read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *string = malloc(fsize + 1);
    if (!string) { fclose(f); return NULL; }
    fread(string, fsize, 1, f);
    fclose(f);
    
    string[fsize] = '\0';
    return string;
}

// ----------------------------------------------------------------------------
// Phase 4: Embedded AI Inference (llama.cpp)
// ----------------------------------------------------------------------------
int is_chatbot_prompt(const char* code) {
    char lower[256] = {0};
    strncpy(lower, code, 255);
    for(int i = 0; lower[i]; i++){
        lower[i] = tolower((unsigned char)lower[i]);
    }
    
    if (strstr(lower, "write a script") || 
        strstr(lower, "create a script") || 
        strstr(lower, "make a python") || 
        strstr(lower, "write a program") || 
        strstr(lower, "create a program") || 
        strstr(lower, "can you write") || 
        strstr(lower, "generate a script") ||
        strstr(lower, "make a script")) {
        return 1;
    }
    return 0;
}

char* run_llama_transpilation(const char *woma_code) {
    // 1. Initialize llama.cpp
    llama_backend_init();
    
    struct llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 99; // Offload to RTX 4050 (CUDA) if LLAMA_CUBLAS=1
    
    extern unsigned char _binary_model_gguf_start[];
    extern unsigned char _binary_model_gguf_end[];
    size_t model_size = _binary_model_gguf_end - _binary_model_gguf_start;

    const char *tmp_model_path = "/tmp/.woma_model.gguf";
    struct stat st;
    if (stat(tmp_model_path, &st) != 0 || st.st_size != model_size) {
        FILE *f = fopen(tmp_model_path, "wb");
        if (f) {
            fwrite(_binary_model_gguf_start, 1, model_size, f);
            fclose(f);
        } else {
            fprintf(stderr, "FATAL: Failed to write embedded AI model to %s\n", tmp_model_path);
            exit(1);
        }
    }

    struct llama_model *model = llama_model_load_from_file(tmp_model_path, model_params);
    if (!model) {
        fprintf(stderr, "FATAL: Failed to load AI model\n");
        exit(1);
    }
    
    struct llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 4096; // 4K context for code translation
    struct llama_context *ctx = llama_init_from_model(model, ctx_params);
    
    // 2. Prepare Prompt (ChatML format for Qwen2.5-Coder)
    const struct llama_vocab *vocab = llama_model_get_vocab(model);
    
    const char *system_prompt = "<|im_start|>system\nYou are WomaPython, a polyglot compiler. Translate the given pseudocode into a valid Python 3 script. Output ONLY raw Python code. Do not use markdown. If the input is a conversational AI prompt (like 'Create a script that pings google' or 'Write a python script to do X'), output exactly: WOMA_COMPILER_ERROR_REJECTED. Otherwise, translate the logic faithfully.<|im_end|>\n<|im_start|>user\n";
    const char *prompt_suffix = "<|im_end|>\n<|im_start|>assistant\n";
    size_t prompt_len = strlen(system_prompt) + strlen(woma_code) + strlen(prompt_suffix) + 1;
    char *full_prompt = malloc(prompt_len);
    snprintf(full_prompt, prompt_len, "%s%s%s", system_prompt, woma_code, prompt_suffix);
    
    // Tokenization and Evaluation
    int n_tokens = prompt_len + 1024;
    llama_token *tokens = malloc(n_tokens * sizeof(llama_token));
    int n_prompt_tokens = llama_tokenize(vocab, full_prompt, strlen(full_prompt), tokens, n_tokens, true, true);
    
    // Evaluate prompt
    struct llama_batch batch = llama_batch_init(n_prompt_tokens, 0, 1);
    batch.n_tokens = n_prompt_tokens;
    for (int i = 0; i < n_prompt_tokens; i++) {
        batch.token[i] = tokens[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = (i == n_prompt_tokens - 1);
    }
    
    if (llama_decode(ctx, batch)) {
        fprintf(stderr, "FATAL: Failed to decode prompt\n");
        exit(1);
    }

    // Allocate buffer for output Python code
    size_t out_cap = 8192;
    char *out_code = malloc(out_cap);
    out_code[0] = '\0';
    size_t out_len = 0;
    
    // Token generation loop
    int n_cur = n_prompt_tokens;
    while (n_cur <= ctx_params.n_ctx) {
        float *logits = llama_get_logits_ith(ctx, -1);
        int n_vocab = llama_vocab_n_tokens(vocab);
        
        llama_token new_token_id = 0;
        float max_logit = logits[0];
        for (int i = 1; i < n_vocab; i++) {
            if (logits[i] > max_logit) { max_logit = logits[i]; new_token_id = i; }
        }
        
        if (llama_vocab_is_eog(vocab, new_token_id)) {
            break; // Finished generating (reached EOS or <|im_end|>)
        }
        
        // Append token to string
        char buf[256];
        int piece_len = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf) - 1, 0, true);
        if (piece_len > 0) {
            buf[piece_len] = '\0';
            strncat(out_code, buf, out_cap - out_len - 1);
            out_len += piece_len;
        }
        
        batch.n_tokens = 1;
        batch.token[0] = new_token_id;
        batch.pos[0] = n_cur;
        batch.n_seq_id[0] = 1;
        batch.seq_id[0][0] = 0;
        batch.logits[0] = true;
        
        llama_decode(ctx, batch);
        n_cur++;
    }
    
    // Cleanup
    llama_batch_free(batch);
    free(full_prompt);
    free(tokens);
    llama_free(ctx);
    llama_model_free(model);
    llama_backend_free();
    
    // Post-process to remove markdown code blocks
    char *start = strstr(out_code, "```python");
    if (start) {
        start += 9;
        while (*start == '\n' || *start == '\r') start++;
        char *end = strstr(start, "```");
        if (end) {
            *end = '\0';
        }
        memmove(out_code, start, strlen(start) + 1);
    } else {
        start = strstr(out_code, "```");
        if (start) {
            start += 3;
            while (*start == '\n' || *start == '\r') start++;
            char *end = strstr(start, "```");
            if (end) {
                *end = '\0';
            }
            memmove(out_code, start, strlen(start) + 1);
        }
    }
    
    return out_code;
}

// ----------------------------------------------------------------------------
// Phase 5: Dynamic Dependency Injection
// ----------------------------------------------------------------------------
void inject_dependencies(const char *py_code) {
    // Run an embedded Python snippet that uses the AST module to parse the code,
    // find imports, check if they exist, and pip install if missing.
    const char *resolver_script = 
        "import ast\n"
        "import sys\n"
        "import os\n"
        "import importlib.util\n"
        "sys.path.insert(0, os.getcwd())\n"
        "code = sys.argv[-1]\n"
        "tree = ast.parse(code)\n"
        "imports = set()\n"
        "for node in ast.walk(tree):\n"
        "    if isinstance(node, ast.Import):\n"
        "        for alias in node.names:\n"
        "            imports.add(alias.name.split('.')[0])\n"
        "    elif isinstance(node, ast.ImportFrom):\n"
        "        if node.module:\n"
        "            imports.add(node.module.split('.')[0])\n"
        "\n"
        "for mod in imports:\n"
        "    if not importlib.util.find_spec(mod):\n"
        "        print(f'[*] WomaPython: Auto-installing missing dependency: {mod}')\n"
        "        os.system(f'env -u PYTHONHOME pip install -q --break-system-packages -t . {mod}')\n";
    
    // We pass the code string as the last argument in sys.argv temporarily
    PyObject *sys_module = PyImport_ImportModule("sys");
    PyObject *sys_argv = PyObject_GetAttrString(sys_module, "argv");
    PyObject *py_code_str = PyUnicode_FromString(py_code);
    PyList_Append(sys_argv, py_code_str);
    
    PyRun_SimpleString(resolver_script);
    
    // Remove the temporary code string from sys.argv to clean up
    // (We use a simple Python command for cleanup here for brevity)
    PyRun_SimpleString("sys.argv.pop()");
    
    Py_DECREF(py_code_str);
    Py_DECREF(sys_argv);
    Py_DECREF(sys_module);
}

// ----------------------------------------------------------------------------
// Main Entrypoint
// ----------------------------------------------------------------------------
int main(int argc, char **argv) {
    if (argc < 2) {
        return Py_BytesMain(argc, argv); // Normal Python REPL
    }
    char exe_path[1024];
    ssize_t link_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (link_len != -1) {
        exe_path[link_len] = '\0';
        char *dir = dirname(exe_path);
        setenv("PYTHONHOME", dir, 1);
    }
    const char *input_file = argv[1];
    size_t len = strlen(input_file);
    
    if (len > 5 && strcmp(input_file + len - 5, ".woma") == 0) {
        // Phase 3: Check cache
        char hash[65];
        if (compute_sha256(input_file, hash) != 0) {
            fprintf(stderr, "Error: Could not read .woma file\n");
            return 1;
        }
        
        char py_file[1024];
        snprintf(py_file, sizeof(py_file), "%.*s.py", (int)(len - 5), input_file);
        
        int cache_hit = 0;
        FILE *pf = fopen(py_file, "r");
        if (pf) {
            char line[256];
            if (fgets(line, sizeof(line), pf)) {
                char expected_header[128];
                snprintf(expected_header, sizeof(expected_header), "# WOMA_HASH: %s\n", hash);
                if (strcmp(line, expected_header) == 0) {
                    cache_hit = 1;
                }
            }
            fclose(pf);
        }
        
        char *python_code = NULL;
        
        if (cache_hit) {
            // Cache Hit: Read the existing Python code
            python_code = read_file(py_file);
        } else {
            // Cache Miss: Phase 4
            printf("[*] WomaPython: Cache miss. Initializing AI transpiler...\n");
            char *woma_code = read_file(input_file);
            if (woma_code && is_chatbot_prompt(woma_code)) {
                fprintf(stderr, "SyntaxError: Woma is a strict polyglot compiler. Conversational prompts are rejected. Please provide explicit line-by-line logic.\n");
                free(woma_code);
                return 1;
            }
            python_code = run_llama_transpilation(woma_code);
            free(woma_code);
            
            if (python_code && strstr(python_code, "WOMA_COMPILER_ERROR_REJECTED")) {
                fprintf(stderr, "SyntaxError: Woma is a strict polyglot compiler. The AI rejected the conversational prompt. Please provide explicit line-by-line logic.\n");
                free(python_code);
                return 1;
            }
            
            // File Write: Save the transpiled code
            FILE *out = fopen(py_file, "w");
            if (out) {
                fprintf(out, "# WOMA_HASH: %s\n", hash);
                fprintf(out, "%s\n", python_code);
                fclose(out);
            }
        }
        
        // Initialize CPython interpreter explicitly before running dynamic dependency injection
        Py_Initialize();
        
        // Phase 5: Dynamic Dependency Injection
        if (python_code) {
            inject_dependencies(python_code);
        }
        
        // Phase 6: Final Execution & Memory Safety
        PyRun_SimpleString("import sys, os\nsys.path.insert(0, os.getcwd())\n");
        FILE *run_file = fopen(py_file, "r");
        if (run_file) {
            PyRun_SimpleFile(run_file, py_file);
            fclose(run_file);
        } else {
            fprintf(stderr, "Error: Could not execute cached %s\n", py_file);
        }
        
        // Cleanup C memory and Python runtime
        free(python_code);
        Py_Finalize();
        return 0;
    }
    
    // Normal Python behavior if not a .woma file
    return Py_BytesMain(argc, argv);
}
