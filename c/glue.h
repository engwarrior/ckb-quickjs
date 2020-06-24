#ifndef CKB_QUICKJS_GLUE_H
#define CKB_QUICKJS_GLUE_H

#include "ckb_syscalls.h"
#include "quickjs.h"
#include "quickjs-libc.h"

double dummy_get_now(void) {
    /*
     * Return a fixed time here as a dummy value since CKB does not support
     * fetching current timestamp
     */
    return -11504520000.0;
}

/*
 * Check if v can fit in duk_int_t, if so, push it to quickjs stack, otherwise
 * throw an error.
 */
static void push_checked_integer(JSContext *ctx, uint64_t v) {
    if (v == ((uint64_t)((duk_int_t)v))) {
        JS_DupValue(ctx, JS_NewBigUint64(ctx, v));
    } else {
        JS_ThrowInternalError(ctx, "Integer %lu is overflowed!", v);
    }
}

static void check_ckb_syscall_ret(JSContext *ctx, int ret) {
    if (ret != 0) {
        JS_ThrowInternalError(ctx, "Invalid CKB syscall response: %d", ret);
    }
}

static size_t extract_source(JSContext *ctx, JSValueConst v) {
    size_t source = 0xFFFFFFFFFFFFFFFF;
    if (JS_ToBool(JS_IsString(ctx, v)) == 1) {
        const char *str = JS_ToCString(ctx, v);
        source = strtol(str, NULL, 10);
    } else if (JS_ToBool(JS_IsNumber(ctx, v) == 1) {
        source = (size_t)JS_ToInt32(ctx, v);
    } else {
        JS_ThrowInternalError(ctx, "Invalid source type!");
    }
    return source;
}

static int quick_ckb_debug(duk_context *ctx) {
    JS_DupValue(ctx, JS_NewString(ctx, " "));
    JS_DupValue(ctx, JS_NewInt32(ctx, 0));
    JS_DupValue(ctx, JS_NewInt32(ctx, -1));
    ckb_debug("-1");
    return 0;
}

typedef int (*load_hash_function)(void *, uint64_t *, size_t);
typedef int (*load_single_function)(void *, uint64_t *, size_t);
typedef int (*load_function)(void *, uint64_t *, size_t, size_t,
                             size_t);
typedef int (*load_by_field_function)(void *, uint64_t *, size_t,
                                      size_t, size_t, size_t);

static int quick_ckb_load_hash(JSContext *ctx, load_hash_function f) {
    uint64_t len = 32;
    uint8_t *buf;
    JSFreeArrayBufferDataFunc *free_func;
    JSValue array = JS_NewArrayBuffer(ctx, buf, len, free_func, JS_GetContextOpaque(ctx), JS_BOOL(0));
    JS_DupValue(ctx, array);
    check_ckb_syscall_ret(ctx, f(buf, &len, 0));
    if (len != 32) {
        return JS_ToInt32(ctx, JS_ThrowInternalError(ctx, "Invalid CKB hash length: %ld", len));
    }
    /* Create an ArrayBuffer for ease of handling at JS side */
    JS_DupValue(ctx, array);
    JS_FreeValue(ctx, array)

    return 1;
}

static int quick_ckb_raw_load_single(JSContext *ctx, load_single_function f) {
    if (JS_ToBool(JS_IsNumber(ctx, v)) != 1) {
        return JS_ToInt32(ctx, JS_ThrowInternalError(ctx, "Invalid arguments"));
    }

    size_t buffer_size = 0;
    uint8_t *buf;
    JSFreeArrayBufferDataFunc *free_func;
    JSValue array = JS_NewArrayBuffer(ctx, buf, len, free_func, JS_GetContextOpaque(ctx), JS_BOOL(0));
    JS_DupValue(ctx, array);
    check_ckb_syscall_ret(ctx, f(buf, &len, 0));

    uint64_t len = buffer_size;
    int ret = f(buffer, &len, offset);

    if (ret != 0) {
        JS_DupValue(ctx, JS_NewInt32(ctx, -ret));
    } else {
        push_checked_integer(ctx, len);
    }

    return 1;
}

void ckb_init(JSContext *ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);

    JS_NewCFunction(ctx, quick_ckb_debug, "debug", 0);
}

#endif //CKB_QUICKJS_GLUE_H

