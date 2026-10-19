/* Check that an Emscripten callback cannot exit a suspended main(). */

#include <emscripten.h>
#include <stdio.h>

EM_JS(__externref_t, callback_promise, (void), {
    return new Promise(resolve => { Module.testMainResume = resolve; });
})

EM_JS(void, await_callback, (__externref_t promise), {
    return promise;
}
await_callback = new WebAssembly.Suspending(await_callback);
)

EM_JS(void, callback, (void *arg), {
    Module.testMainResume();
})

int
main(int argc, char **argv)
{
    __externref_t promise = callback_promise();
    emscripten_async_call(callback, NULL, 0);
    await_callback(promise);
    puts("web_embed_test: main resumed");
    return 7;
}
