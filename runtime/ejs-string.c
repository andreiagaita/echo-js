/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99 ft=cpp:
 */
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>

#include "ejs-value.h"
#include "ejs-array.h"
#include "ejs-string.h"
#include "ejs-function.h"
#include "ejs-regexp.h"
#include "ejs-ops.h"
#include "ejs-string.h"
#include "ejs-error.h"

int32_t
ucs2_strcmp (const jschar *s1, const jschar *s2)
{
    const jschar *s1p = s1;
    const jschar *s2p = s2;

    while (*s1p && *s2p && *s1p == *s2p) {
        s1p++;
        s2p++;
    }

    return ((int32_t)*s1p) - ((int32_t)*s2p);
}

jschar*
ucs2_strdup (const jschar *str)
{
    int32_t len = ucs2_strlen(str);
    jschar* result = (jschar*)calloc(sizeof(jschar), len + 1);
    memmove (result, str, len * sizeof(jschar));
    return result;
}

int32_t
ucs2_strlen (const jschar *str)
{
    int32_t rv = 0;
    while (*str++) rv++;
    return rv;
}

uint32_t
ucs2_hash (const jschar* str, int32_t hash, int length)
{
    const char* p = (const char*)str;

    while (length >= 0) {
        hash = (hash << 5) - hash * (*p++);
        hash = (hash << 5) - hash + (*p++);
        length --;
    }

    return (uint32_t)hash;
}

jschar*
ucs2_strstr (const jschar *haystack,
             const jschar *needle)
{
    const jschar *p = haystack;

    while (*p) {
        const jschar *next_candidate = NULL;

        if (*p == *needle) {
            const jschar *p2 = p+1;
            const jschar *n = needle+1;

            if (!next_candidate && *p2 == *needle)
                next_candidate = p2;

            while (*n) {
                if (*n != *p2)
                    break;
                n++;
                p2++;
                if (!next_candidate && *p2 == *needle)
                    next_candidate = p2;
            }
            if (*n == 0)
                return (jschar*)p;

            if (next_candidate)
                p = next_candidate;
            else
                p++;
            continue;
        }
        else {
            if (next_candidate)
                p = next_candidate;
            else
                p++;
        }
    }

    return NULL;
}

jschar*
ucs2_strrstr (const jschar *haystack,
              const jschar *needle)
{
    int haystack_len = ucs2_strlen(haystack);
    int needle_len = ucs2_strlen(needle);

    if (needle_len > haystack_len)
        return NULL;

    const jschar* p = haystack + haystack_len - 1;
    const jschar* needle_p = needle + needle_len - 1;

    while (p >= haystack) {
        const jschar *next_candidate = NULL;

        if (*p == *needle_p) {
            const jschar *p2 = p+1;
            const jschar *n = needle_p-1;

            if (!next_candidate && *p2 == *needle)
                next_candidate = p2;

            while (n >= needle) {
                if (*n != *p2)
                    break;
                n--;
                p2--;
                if (!next_candidate && *p2 == *needle_p)
                    next_candidate = p2;
            }
            if (needle_p < needle)
                return (jschar*)p;

            if (next_candidate)
                p = next_candidate;
            else
                p--;
            continue;
        }
        else {
            if (next_candidate)
                p = next_candidate;
            else
                p--;
        }
    }

    return NULL;
}


static jschar
utf8_to_ucs2 (const unsigned char * input, const unsigned char ** end_ptr)
{
    *end_ptr = input;
    if (input[0] == 0)
        return -1;
    if (input[0] < 0x80) {
        * end_ptr = input + 1;
        return input[0];
    }
    if ((input[0] & 0xE0) == 0xE0) {
        if (input[1] == 0 || input[2] == 0)
            return -1;
        * end_ptr = input + 3;
        return
            (input[0] & 0x0F)<<12 |
            (input[1] & 0x3F)<<6  |
            (input[2] & 0x3F);
    }
    if ((input[0] & 0xC0) == 0xC0) {
        if (input[1] == 0)
            return -1;
        * end_ptr = input + 2;
        return
            (input[0] & 0x1F)<<6  |
            (input[1] & 0x3F);
    }
    return -1;
}

static int
ucs2_to_utf8_char (jschar ucs2, char *utf8)
{
    if (ucs2 < 0x80) {
        utf8[0] = (char)ucs2;
        return 1;
    }
    if (ucs2 >= 0x80  && ucs2 < 0x800) {
        utf8[0] = (ucs2 >> 6)   | 0xC0;
        utf8[1] = (ucs2 & 0x3F) | 0x80;
        return 2;
    }
    if (ucs2 >= 0x800 && ucs2 < 0xFFFF) {
        utf8[0] = ((ucs2 >> 12)       ) | 0xE0;
        utf8[1] = ((ucs2 >> 6 ) & 0x3F) | 0x80;
        utf8[2] = ((ucs2      ) & 0x3F) | 0x80;
        return 3;
    }
    return -1;
}

char*
ucs2_to_utf8 (const jschar *str)
{
    int len = ucs2_strlen(str);
    char *conservative = (char*)calloc (sizeof(char), len * 3 + 1);

    const jschar *p = str;
    char *c = conservative;

    while (*p) {
        int adv = ucs2_to_utf8_char (*p, c);
        if (adv < 1) {
            // more here XXX
            break;
        }
        c += adv;

        p++;
        len --;
        if (len == 0)
            break;
    }

    *c = 0;
    return conservative;
}


ejsval _ejs_String EJSVAL_ALIGNMENT;
ejsval _ejs_String__proto__ EJSVAL_ALIGNMENT;
ejsval _ejs_String_prototype EJSVAL_ALIGNMENT;

static ejsval
_ejs_String_impl (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    if (EJSVAL_IS_NULL(_this) || EJSVAL_IS_UNDEFINED(_this)) {
        if (argc > 0)
            return ToString(args[0]);
        else
            return _ejs_atom_empty;
    }
    else {
        // called as a constructor
        EJSString* str = (EJSString*)EJSVAL_TO_OBJECT(_this);
        ((EJSObject*)str)->ops = &_ejs_String_specops;

        if (argc > 0) {
            str->primStr = ToString(args[0]);
        }
        else {
            str->primStr = _ejs_atom_empty;
        }
        return _this;
    }
}

static ejsval
_ejs_String_prototype_toString (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    EJSString *str = (EJSString*)EJSVAL_TO_OBJECT(_this);

    return str->primStr;
}

static ejsval
_ejs_String_prototype_replace (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    if (argc == 0)
        return _this;

    ejsval thisStr = ToString(_this);
    ejsval searchValue = args[0];
    ejsval replaceValue = argc > 1 ? args[1] : _ejs_undefined;

    if (EJSVAL_IS_REGEXP(searchValue)) {
        return _ejs_regexp_replace (thisStr, searchValue, replaceValue);
    }
    else {
        ejsval searchValueStr = ToString(searchValue);
        ejsval replaceValueStr = ToString(replaceValue);
        jschar *p = ucs2_strstr (EJSVAL_TO_FLAT_STRING(thisStr), EJSVAL_TO_FLAT_STRING(searchValueStr));
        if (p == NULL) {
            return _this;
        }
        else {
            int len1 = p - EJSVAL_TO_FLAT_STRING(thisStr);
            int len2 = EJSVAL_TO_STRLEN(replaceValueStr);
            int len3 = ucs2_strlen(p + EJSVAL_TO_STRLEN(searchValueStr));

            int new_len = len1;
            new_len += len2;
            new_len += len3;
            new_len += 1; // for the \0

            jschar* result = (jschar*)calloc(sizeof(jschar), new_len + 1);
            jschar* r = result;
            // XXX this should really use concat nodes instead
            memmove (r, EJSVAL_TO_FLAT_STRING(thisStr), len1 * sizeof(jschar)); r += len1;
            memmove (r, EJSVAL_TO_FLAT_STRING(replaceValueStr), len2 * sizeof(jschar)); r += len2;
            memmove (r, p + EJSVAL_TO_STRLEN(searchValueStr), len3 * sizeof(jschar));

            ejsval rv = _ejs_string_new_ucs2 (result);
            free (result);
            return rv;
        }
    }
}

static ejsval
_ejs_String_prototype_charAt (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    ejsval primStr;

    if (EJSVAL_IS_STRING(_this)) {
        primStr = _this;
    }
    else {
        EJSString *str = (EJSString*)EJSVAL_TO_OBJECT(_this);
        primStr = str->primStr;
    }

    int idx = 0;
    if (argc > 0 && EJSVAL_IS_NUMBER(args[0])) {
        idx = (int)EJSVAL_TO_NUMBER(args[0]);
    }

    if (idx < 0 || idx > EJSVAL_TO_STRLEN(primStr))
        return _ejs_atom_empty;

    char c[2];
    c[0] = EJSVAL_TO_FLAT_STRING(primStr)[idx];
    c[1] = '\0';
    return _ejs_string_new_utf8 (c);
}

static ejsval
_ejs_String_prototype_charCodeAt (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    ejsval primStr;

    if (EJSVAL_IS_STRING(_this)) {
        primStr = _this;
    }
    else {
        EJSString *str = (EJSString*)EJSVAL_TO_OBJECT(_this);
        primStr = str->primStr;
    }

    int idx = 0;
    if (argc > 0 && EJSVAL_IS_NUMBER(args[0])) {
        idx = (int)EJSVAL_TO_NUMBER(args[0]);
    }

    if (idx < 0 || idx >= EJSVAL_TO_STRLEN(primStr))
        return _ejs_nan;

    return NUMBER_TO_EJSVAL (EJSVAL_TO_FLAT_STRING(primStr)[idx]);
}

static ejsval
_ejs_String_prototype_concat (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    EJS_NOT_IMPLEMENTED();
}

static ejsval
_ejs_String_prototype_indexOf (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    int idx = -1;
    if (argc == 0)
        return NUMBER_TO_EJSVAL(idx);

    ejsval haystack = ToString(_this);
    jschar* haystack_cstr;
    if (EJSVAL_IS_STRING(haystack)) {
        haystack_cstr = EJSVAL_TO_FLAT_STRING(haystack);
    }
    else {
        haystack_cstr = EJSVAL_TO_FLAT_STRING(((EJSString*)EJSVAL_TO_OBJECT(haystack))->primStr);
    }

    ejsval needle = ToString(args[0]);
    jschar *needle_cstr;
    if (EJSVAL_IS_STRING(needle)) {
        needle_cstr = EJSVAL_TO_FLAT_STRING(needle);
    }
    else {
        needle_cstr = EJSVAL_TO_FLAT_STRING(((EJSString*)EJSVAL_TO_OBJECT(needle))->primStr);
    }
  
    jschar* p = ucs2_strstr(haystack_cstr, needle_cstr);
    if (p == NULL)
        return NUMBER_TO_EJSVAL(idx);

    return NUMBER_TO_EJSVAL (p - haystack_cstr);
}

static ejsval
_ejs_String_prototype_lastIndexOf (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    int idx = -1;
    if (argc == 0)
        return NUMBER_TO_EJSVAL(idx);

    ejsval haystack = ToString(_this);
    jschar* haystack_cstr;
    if (EJSVAL_IS_STRING(haystack)) {
        haystack_cstr = EJSVAL_TO_FLAT_STRING(haystack);
    }
    else {
        haystack_cstr = EJSVAL_TO_FLAT_STRING(((EJSString*)EJSVAL_TO_OBJECT(haystack))->primStr);
    }

    ejsval needle = ToString(args[0]);
    jschar *needle_cstr;
    if (EJSVAL_IS_STRING(needle)) {
        needle_cstr = EJSVAL_TO_FLAT_STRING(needle);
    }
    else {
        needle_cstr = EJSVAL_TO_FLAT_STRING(((EJSString*)EJSVAL_TO_OBJECT(needle))->primStr);
    }
  
    jschar* p = ucs2_strrstr(haystack_cstr, needle_cstr);
    if (p == NULL)
        return NUMBER_TO_EJSVAL(idx);

    return NUMBER_TO_EJSVAL (p - haystack_cstr);
}

static ejsval
_ejs_String_prototype_localeCompare (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    EJS_NOT_IMPLEMENTED();
}

static ejsval
_ejs_String_prototype_match (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    ejsval regexp;

    if (argc > 0)
        regexp = args[0];

    /* 1. Call CheckObjectCoercible passing the this value as its argument. */
    /* 2. Let S be the result of calling ToString, giving it the this value as its argument. */
    ejsval S = ToString(_this);

    ejsval rx;
    EJSObject* rxo;

    if (EJSVAL_IS_REGEXP(regexp)) {
        /* 3. If Type(regexp) is Object and the value of the [[Class]]
              internal property of regexp is "RegExp", then let rx be
              regexp; */
        rx = regexp;
    }
    else {
        /* 4. Else, let rx be a new RegExp object created as if by the
              expression new RegExp( regexp) where RegExp is the standard
              built-in constructor with that name. */
        rx = _ejs_regexp_new (regexp, _ejs_undefined/* XXX? */);
    }
    rxo = EJSVAL_TO_OBJECT(rx);
        

    /* 5. Let global be the result of calling the [[Get]] internal method of rx with argument "global". */
    ejsval global = OP(rxo,get) (rx, _ejs_atom_global);
    
    /* 6. Let exec be the standard built-in function RegExp.prototype.exec (see 15.10.6.2) */
    ejsval exec = _ejs_RegExp_prototype_exec_closure;

    /* 7. If global is not true, then */
    if (!_ejs_truthy(ToBoolean(global))) {
        /*    a. Return the result of calling the [[Call]] internal method of exec with rx as the this value and argument list containing S. */
        ejsval call_args[1];
        call_args[0] = S;
        return _ejs_invoke_closure (exec, rx, 1, call_args);
    }
    /* 8. Else, global is true */
    else {
        /*    a. Call the [[Put]] internal method of rx with arguments "lastIndex" and 0. */
        OP(rxo, put)(rx, _ejs_atom_lastIndex, NUMBER_TO_EJSVAL(0), EJS_FALSE);

        /*    b. Let A be a new array created as if by the expression new Array() where Array is the standard built-in constructor with that name. */
        ejsval A = _ejs_array_new(0, EJS_FALSE);

        /*    c. Let previousLastIndex be 0. */
        ejsval previousLastIndex = NUMBER_TO_EJSVAL(0);

        /*    d. Let n be 0. */
        int n = 0;

        /*    e. Let lastMatch be true. */
        EJSBool lastMatch = EJS_TRUE;

        /*    f. Repeat, while lastMatch is true */
        while (lastMatch) {
            /* i. Let result be the result of calling the [[Call]] internal method of exec with rx as the this value and argument list containing S. */
            ejsval call_args[1];
            call_args[0] = S;
            ejsval result = _ejs_invoke_closure (exec, rx, 1, call_args);
            /* ii. If result is null, then set lastMatch to false. */
            if (EJSVAL_IS_NULL(result)) {
                lastMatch = EJS_FALSE;
            }
            /* iii. Else, result is not null */
            else {
                /* 1. Let thisIndex be the result of calling the [[Get]] internal method of rx with argument "lastIndex". */
                ejsval thisIndex = OP(rxo, get)(rx, _ejs_atom_lastIndex);
                
                /* 2. If thisIndex = previousLastIndex then */
                if (EJSVAL_EQ(thisIndex, previousLastIndex)) {
                    /* a. Call the [[Put]] internal method of rx with arguments "lastIndex" and thisIndex+1. */
                    /* b. Set previousLastIndex to thisIndex+1. */
                    previousLastIndex = NUMBER_TO_EJSVAL (EJSVAL_TO_NUMBER(thisIndex) + 1);
                    OP(rxo, put)(rx, _ejs_atom_lastIndex, previousLastIndex, EJS_FALSE);
                }
                /* 3. Else, set previousLastIndex to thisIndex. */
                else {
                    previousLastIndex = thisIndex;
                }
                /* 4. Let matchStr be the result of calling the [[Get]] internal method of result with argument "0". */
                ejsval matchStr = OP(rxo, get)(rx, NUMBER_TO_EJSVAL(0));

                /* 5. Call the [[DefineOwnProperty]] internal method of A with arguments ToString(n), the Property Descriptor {[[Value]]: matchStr, [[Writable]]: true, [[Enumerable]]: true, [[configurable]]: true}, and false. */
                _ejs_object_setprop (A, NUMBER_TO_EJSVAL(n), matchStr);

                /* 6. Increment n. */
                n++;
            }
        }

        /*    g. If n = 0, then return null. */
        if (n == 0) return _ejs_null;

        /*    h. Return A. */
        return A;
    }
}

static ejsval
_ejs_String_prototype_search (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    EJS_NOT_IMPLEMENTED();
}

static ejsval
_ejs_String_prototype_substring (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    /* 1. Call CheckObjectCoercible passing the this value as its argument. */
    if (EJSVAL_IS_NULL_OR_UNDEFINED(_this))
        _ejs_throw_nativeerror_utf8 (EJS_TYPE_ERROR, "String.prototype.subString called on null or undefined");

    ejsval start = _ejs_undefined;
    ejsval end = _ejs_undefined;

    if (argc > 0) start = args[0];
    if (argc > 1) end = args[1];

    /* 2. Let S be the result of calling ToString, giving it the this value as its argument. */
    ejsval S = ToString(_this);

    /* 3. Let len be the number of characters in S. */
    int len = EJSVAL_TO_STRLEN(S);

    /* 4. Let intStart be ToInteger(start). */
    int32_t intStart = ToInteger(start);

    /* 5. If end is undefined, let intEnd be len; else let intEnd be ToInteger(end). */
    int32_t intEnd = (EJSVAL_IS_UNDEFINED(end)) ? len : ToInteger(end);
        
    /* 6. Let finalStart be min(max(intStart, 0), len). */
    int32_t finalStart = MIN(MAX(intStart, 0), len);

    /* 7. Let finalEnd be min(max(intEnd, 0), len). */
    int32_t finalEnd = MIN(MAX(intEnd, 0), len);

    /* 8. Let from be min(finalStart, finalEnd). */
    int32_t from = MIN(finalStart, finalEnd);

    /* 9. Let to be max(finalStart, finalEnd). */
    int32_t to = MAX(finalStart, finalEnd);

    /* 10. Return a String whose length is to - from, containing characters from S, namely the characters with indices  */
    /*     from through to-1, in ascending order. */
    return _ejs_string_new_substring (S, from, to-from);
}

// ECMA262: B.2.3
static ejsval
_ejs_String_prototype_substr (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    ejsval start = _ejs_undefined;
    ejsval length = _ejs_undefined;

    if (argc > 0) start = args[0];
    if (argc > 1) length = args[1];

    /* 1. Call ToString, giving it the this value as its argument. */
    ejsval Result1 = ToString(_this);

    /* 2. Call ToInteger(start). */
    int32_t Result2 = ToInteger(start);

    /* 3. If length is undefined, use +inf; otherwise call ToInteger(length). */
    int32_t Result3;
    if (EJSVAL_IS_UNDEFINED(length)) {
        EJS_NOT_IMPLEMENTED();
    }
    else {
        Result3 = ToInteger(length);
    }

    /* 4. Compute the number of characters in Result(1). */
    int32_t Result4 = EJSVAL_TO_STRLEN(Result1);

    /* 5. If Result(2) is positive or zero, use Result(2); else use max(Result(4)+Result(2),0). */
    int32_t Result5 = Result2 >= 0 ? Result2 : MAX(Result4+Result2, 0);
        
    /* 6. Compute min(max(Result(3),0), Result(4)–Result(5)). */
    int32_t Result6 = MIN(MAX(Result3, 0), Result4 - Result5);
    
    /* 7. If Result(6) <= 0, return the empty String "". */
    if (Result6 <= 0) return _ejs_atom_empty;

    /* 8. Return a String containing Result(6) consecutive characters from Result(1) beginning with the character at  */
    /*    position Result(5). */
    return _ejs_string_new_substring (Result1, Result5, Result6);
}

static ejsval
_ejs_String_prototype_toLowerCase (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    /* 1. Call CheckObjectCoercible passing the this value as its argument. */
    /* 2. Let S be the result of calling ToString, giving it the this value as its argument. */
    ejsval S = ToString(_this);
    char* sstr = ucs2_to_utf8(EJSVAL_TO_FLAT_STRING(S));

    /* 3. Let L be a String where each character of L is either the Unicode lowercase equivalent of the corresponding  */
    /*    character of S or the actual corresponding character of S if no Unicode lowercase equivalent exists. */
    char* p = sstr;
    while (*p) {
        *p = tolower(*p);
        p++;
    }

    ejsval L = _ejs_string_new_utf8(sstr);
    free(sstr);

    /* 4. Return L. */
    return L;
}

static ejsval
_ejs_String_prototype_toLocaleLowerCase (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    EJS_NOT_IMPLEMENTED();
}

static ejsval
_ejs_String_prototype_toUpperCase (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    /* 1. Call CheckObjectCoercible passing the this value as its argument. */
    /* 2. Let S be the result of calling ToString, giving it the this value as its argument. */
    ejsval S = ToString(_this);
    char* sstr = ucs2_to_utf8(EJSVAL_TO_FLAT_STRING(S));

    /* 3. Let L be a String where each character of L is either the Unicode lowercase equivalent of the corresponding  */
    /*    character of S or the actual corresponding character of S if no Unicode lowercase equivalent exists. */
    char* p = sstr;
    while (*p) {
        *p = toupper(*p);
        p++;
    }

    ejsval L = _ejs_string_new_utf8(sstr);
    free(sstr);

    /* 4. Return L. */
    return L;
}

static ejsval
_ejs_String_prototype_toLocaleUpperCase (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    EJS_NOT_IMPLEMENTED();
}

static ejsval
_ejs_String_prototype_trim (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    EJS_NOT_IMPLEMENTED();
}

static ejsval
_ejs_String_prototype_valueOf (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    EJS_NOT_IMPLEMENTED();
}

// ECMA262: 15.5.4.14
typedef enum {
    MATCH_RESULT_FAILURE,
    MATCH_RESULT_SUCCESS
} MatchResultType;

typedef struct {
    MatchResultType type;
    int endIndex;
    ejsval captures;
} MatchResultState;

static MatchResultState
SplitMatch(ejsval S, int q, ejsval R)
{
    /* 1. If R is a RegExp object (its [[Class]] is "RegExp"), then */
    if (EJSVAL_IS_REGEXP(R)) {
        /*    a. Call the [[Match]] internal method of R giving it the arguments S and q, and return the MatchResult  */
        /*       result. */
        EJS_NOT_IMPLEMENTED();
    }
        
    /* 2. Type(R) must be String. Let r be the number of characters in R. */
    int r = EJSVAL_TO_STRLEN(R);

    /* 3. Let s be the number of characters in S. */
    int s = EJSVAL_TO_STRLEN(S);

    /* 4. If q+r > s then return the MatchResult failure. */
    if (q + r > s) {
        MatchResultState rv = { MATCH_RESULT_FAILURE };
        return rv;
    }

    /* 5. If there exists an integer i between 0 (inclusive) and r (exclusive) such that the character at position q+i of S */
    /*    is different from the character at position i of R, then return failure. */
    jschar* sstr = EJSVAL_TO_FLAT_STRING(S);
    jschar* rstr = EJSVAL_TO_FLAT_STRING(R);
    for (int i = 0; i < r; i ++) {
        if (sstr[q+i] != rstr[i]) {
            MatchResultState rv = { MATCH_RESULT_FAILURE };
            return rv;
        }
    }

    /* 6. Let cap be an empty array of captures (see 15.10.2.1). */
    ejsval cap = _ejs_array_new(0, EJS_FALSE);
    /* 7. Return the State (q+r, cap). (see 15.10.2.1) */
    MatchResultState rv = { MATCH_RESULT_SUCCESS, q+r, cap };
    return rv;
}

static ejsval
_ejs_String_prototype_split (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    ejsval separator = _ejs_undefined;
    ejsval limit = _ejs_undefined;

    if (argc > 0) separator = args[0];
    if (argc > 1) limit = args[1];

    /* 1. Call CheckObjectCoercible passing the this value as its argument. */
    /* 2. Let S be the result of calling ToString, giving it the this value as its argument. */
    ejsval S = ToString(_this);

    /* 3. Let A be a new array created as if by the expression new Array() where Array is the standard built-in  */
    /*    constructor with that name. */
    ejsval A = _ejs_array_new(0, EJS_FALSE);

    /* 4. Let lengthA be 0. */
    int lengthA = 0;

    /* 5. If limit is undefined, let lim = 2^32–1; else let lim = ToUint32(limit). */
    uint32_t lim = (EJSVAL_IS_UNDEFINED(limit)) ? UINT32_MAX : ToUint32(limit);

    /* 6. Let s be the number of characters in S. */
    int s = EJSVAL_TO_STRLEN(S);

    /* 7. Let p = 0. */
    int p = 0;

    /* 8. If separator is a RegExp object (its [[Class]] is "RegExp"), let R = separator; otherwise let R =  */
    /*    ToString(separator). */
    ejsval R = (EJSVAL_IS_REGEXP(separator)) ? separator : ToString(separator);

    /* 9. If lim = 0, return A. */
    if (lim == 0)
        return A;

    /* 10. If separator is undefined, then */
    if (EJSVAL_IS_UNDEFINED(separator)) {
        /*     a. Call the [[DefineOwnProperty]] internal method of A with arguments "0", Property Descriptor  */
        /*        {[[Value]]: S, [[Writable]]: true, [[Enumerable]]: true, [[Configurable]]: true}, and false. */
        _ejs_array_push_dense (A, 1, &S);

        /*     b. Return A. */
        return A;
    }
    /* 11. If s = 0, then */
    if (s == 0) {
        EJS_NOT_IMPLEMENTED();
        /*     a. Call SplitMatch(S, 0, R) and let z be its MatchResult result. */
        /*     b. If z is not failure, return A. */
        /*     c. Call the [[DefineOwnProperty]] internal method of A with arguments "0", Property Descriptor  */
        /*        {[[Value]]: S, [[Writable]]: true, [[Enumerable]]: true, [[Configurable]]: true}, and false. */
        /*     d. Return A. */
        return A;
    }
    /* 12. Let q = p. */
    int q = p;

    /* 13. Repeat, while q != s */
    while (q != s) {
        /* a. Call SplitMatch(S, q, R) and let z be its MatchResult result. */
        MatchResultState z = SplitMatch(S, q, R);
        
        /* b. If z is failure, then let q = q+1. */
        if (z.type == MATCH_RESULT_FAILURE) {
            q++;
        }
        /* c. Else,  z is not failure */
        else {
            /*    i. z must be a State. Let e be z's endIndex and let cap be z's captures array. */
            int e = z.endIndex;
            ejsval cap = z.captures;

            /*    ii. If e = p, then let q = q+1. */
            if (e == p) {
                q++;
            }
            /*    iii. Else, e != p */
            else {
                /*         1. Let T be a String value equal to the substring of S consisting of the characters at  */
                /*            positions p (inclusive) through q (exclusive). */
                ejsval T = _ejs_string_new_substring (S, p, q-p);
                /*         2. Call the [[DefineOwnProperty]] internal method of A with arguments  */
                /*            ToString(lengthA), Property Descriptor {[[Value]]: T, [[Writable]]: true, */
                /*            [[Enumerable]]: true, [[Configurable]]: true}, and false. */
                _ejs_array_push_dense (A, 1, &T);

                /*         3. Increment lengthA by 1. */
                lengthA ++;
                /*         4. If lengthA = lim, return A. */
                if (lengthA == lim)
                    return A;
                /*         5. Let p = e. */
                p = e;
                /*         6. Let i = 0. */
                int i = 0;
                /*         7. Repeat, while i is not equal to the number of elements in cap. */
                while (i != EJS_ARRAY_LEN(cap)) {
                    /*            a Let i = i+1. */
                    i++;
                    /*            b Call the [[DefineOwnProperty]] internal method of A with arguments  */
                    /*              ToString(lengthA), Property Descriptor {[[Value]]: cap[i], [[Writable]]:  */
                    /*              true, [[Enumerable]]: true, [[Configurable]]: true}, and false. */
                    ejsval pushcap = EJS_DENSE_ARRAY_ELEMENTS(cap)[i];
                    _ejs_array_push_dense (A, 1, &pushcap);
                    /*            c Increment lengthA by 1. */
                    lengthA ++;
                    /*            d If lengthA = lim, return A. */
                    if (lengthA == lim)
                        return A;
                }
                /*         8. Let q = p. */
                q = p;
            }
        }
    }
    /* 14. Let T be a String value equal to the substring of S consisting of the characters at positions p (inclusive)  */
    /*     through s (exclusive). */
    ejsval T = _ejs_string_new_substring (S, p, s-p);
    /* 15. Call the [[DefineOwnProperty]] internal method of A with arguments ToString(lengthA), Property Descriptor  */
    /*     {[[Value]]: T, [[Writable]]: true, [[Enumerable]]: true, [[Configurable]]: true}, and false. */
    _ejs_array_push_dense (A, 1, &T);

    /* 16. Return A. */
    return A;
}

static ejsval
_ejs_String_prototype_slice (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    // assert argc >= 1

    ejsval start = _ejs_undefined;
    ejsval end = _ejs_undefined;

    if (argc > 0) start = args[0];
    if (argc > 1) end = args[1];

    // Call CheckObjectCoercible passing the this value as its argument.
    // Let S be the result of calling ToString, giving it the this value as its argument.
    ejsval S = ToString(_this);
    // Let len be the number of characters in S.
    int len = EJSVAL_TO_STRLEN(S);
    // Let intStart be ToInteger(start).
    int intStart = EJSVAL_IS_UNDEFINED(start) ? 0 : ToInteger(start);

    // If end is undefined, let intEnd be len; else let intEnd be ToInteger(end).
    int intEnd = EJSVAL_IS_UNDEFINED(end) ? len : ToInteger(end);

    // If intStart is negative, let from be max(len + intStart,0); else let from be min(intStart, len).
    int from = intStart < 0 ? MAX(len + intStart, 0) : MIN(intStart, len);

    // If intEnd is negative, let to be max(len + intEnd,0); else let to be min(intEnd, len).
    int to = intEnd < 0 ? MAX(len + intEnd, 0) : MIN(intEnd, len);

    // Let span be max(to – from,0).
    int span = MAX(to - from, 0);

    // Return a String containing span consecutive characters from S beginning with the character at position from.
    return _ejs_string_new_substring (S, from, span);
}

static ejsval
_ejs_String_fromCharCode (ejsval env, ejsval _this, uint32_t argc, ejsval *args)
{
    int length = argc;
    jschar* buf = (jschar*)malloc(sizeof(jschar) * (length+1));
    for (int i = 0; i < argc; i ++) {
        buf[i] = ToUint16(args[i]);
    }
    buf[length] = 0;
    ejsval rv = _ejs_string_new_ucs2(buf);
    free (buf);
    return rv;
}

static void
_ejs_string_init_proto()
{
    _ejs_gc_add_root (&_ejs_String__proto__);
    _ejs_gc_add_root (&_ejs_String_prototype);

    EJSFunction* __proto__ = _ejs_gc_new(EJSFunction);
    _ejs_init_object ((EJSObject*)__proto__, _ejs_Object_prototype, &_ejs_Function_specops);
    __proto__->func = _ejs_Function_empty;
    __proto__->env = _ejs_null;
    _ejs_String__proto__ = OBJECT_TO_EJSVAL(__proto__);

    EJSObject* prototype = _ejs_gc_new(EJSObject);
    _ejs_init_object (prototype, _ejs_null, &_ejs_String_specops);
    _ejs_String_prototype = OBJECT_TO_EJSVAL(prototype);

    _ejs_object_define_value_property (OBJECT_TO_EJSVAL(__proto__), _ejs_atom_name, _ejs_atom_empty, EJS_PROP_NOT_ENUMERABLE | EJS_PROP_NOT_CONFIGURABLE | EJS_PROP_NOT_WRITABLE);
}

void
_ejs_string_init(ejsval global)
{
    _ejs_string_init_proto();
  
    _ejs_String = _ejs_function_new_without_proto (_ejs_null, _ejs_atom_String, (EJSClosureFunc)_ejs_String_impl);
    _ejs_object_setprop (global, _ejs_atom_String, _ejs_String);

    _ejs_object_setprop (_ejs_String,       _ejs_atom_prototype,  _ejs_String_prototype);

#define OBJ_METHOD(x) EJS_INSTALL_ATOM_FUNCTION(_ejs_String, x, _ejs_String_##x)
#define PROTO_METHOD(x) EJS_INSTALL_ATOM_FUNCTION(_ejs_String_prototype, x, _ejs_String_prototype_##x)

    PROTO_METHOD(charAt);
    PROTO_METHOD(charCodeAt);
    PROTO_METHOD(concat);
    PROTO_METHOD(indexOf);
    PROTO_METHOD(lastIndexOf);
    PROTO_METHOD(localeCompare);
    PROTO_METHOD(match);
    PROTO_METHOD(replace);
    PROTO_METHOD(search);
    PROTO_METHOD(slice);
    PROTO_METHOD(split);
    PROTO_METHOD(substr);
    PROTO_METHOD(substring);
    PROTO_METHOD(toLocaleLowerCase);
    PROTO_METHOD(toLocaleUpperCase);
    PROTO_METHOD(toLowerCase);
    PROTO_METHOD(toString);
    PROTO_METHOD(toUpperCase);
    PROTO_METHOD(trim);
    PROTO_METHOD(valueOf);

    OBJ_METHOD(fromCharCode);

#undef OBJ_METHOD
#undef PROTO_METHOD
}

static ejsval
_ejs_string_specop_get (ejsval obj, ejsval propertyName)
{
    // check if propertyName is an integer, or a string that we can convert to an int
    EJSBool is_index = EJS_FALSE;
    int idx = 0;
    if (EJSVAL_IS_NUMBER(propertyName)) {
        double n = EJSVAL_TO_NUMBER(propertyName);
        if (floor(n) == n) {
            idx = (int)n;
            is_index = EJS_TRUE;
        }
    }

    EJSString* estr = (EJSString*)EJSVAL_TO_OBJECT(obj);
    if (is_index) {
        if (idx < 0 || idx > EJSVAL_TO_STRLEN(estr->primStr))
            return _ejs_undefined;
        char c[2];
        c[0] = EJSVAL_TO_FLAT_STRING(estr->primStr)[idx];
        c[1] = '\0';
        return _ejs_string_new_utf8 (c);
    }

    // we also handle the length getter here
    if (EJSVAL_IS_STRING(propertyName) && !ucs2_strcmp (_ejs_ucs2_length, EJSVAL_TO_FLAT_STRING(propertyName))) {
        return NUMBER_TO_EJSVAL (EJSVAL_TO_STRLEN(estr->primStr));
    }

    // otherwise we fallback to the object implementation
    return _ejs_Object_specops.get (obj, propertyName);
}

static EJSObject*
_ejs_string_specop_allocate()
{
    return (EJSObject*)_ejs_gc_new (EJSString);
}

static void
_ejs_string_specop_scan (EJSObject* obj, EJSValueFunc scan_func)
{
    EJSString* ejss = (EJSString*)obj;
    scan_func (ejss->primStr);
    _ejs_Object_specops.scan (obj, scan_func);
}

EJS_DEFINE_CLASS(String,
                 _ejs_string_specop_get,
                 OP_INHERIT, // get_own_property
                 OP_INHERIT, // get_property
                 OP_INHERIT, // put
                 OP_INHERIT, // can_put
                 OP_INHERIT, // has_property
                 OP_INHERIT, // delete
                 OP_INHERIT, // default_value
                 OP_INHERIT, // define_own_property
                 OP_INHERIT, // has_instance
                 _ejs_string_specop_allocate,
                 OP_INHERIT, // finalize
                 _ejs_string_specop_scan
                 )



/// EJSPrimString's

ejsval
_ejs_string_new_utf8 (const char* str)
{
    // XXX assume str is ascii for now
    int str_len = strlen(str);
    size_t value_size = sizeof(EJSPrimString) + sizeof(jschar) * (str_len + 1);

    EJSPrimString* rv = _ejs_gc_new_primstr(value_size);
    EJS_PRIMSTR_SET_TYPE(rv, EJS_STRING_FLAT);
    rv->length = str_len;
    rv->data.flat = (jschar*)((char*)rv + sizeof(EJSPrimString));
    jschar *p = rv->data.flat;
    const unsigned char *stru = (const unsigned char*)str;
    while (*stru) {
        *p++ = utf8_to_ucs2 (stru, &stru);
    }
    return STRING_TO_EJSVAL(rv);
}

ejsval
_ejs_string_new_utf8_len (const char* str, int len)
{
    size_t value_size = sizeof(EJSPrimString) + sizeof(jschar) * (len + 1);

    EJSPrimString* rv = _ejs_gc_new_primstr(value_size);
    EJS_PRIMSTR_SET_TYPE(rv, EJS_STRING_FLAT);
    rv->length = len;
    rv->data.flat = (jschar*)((char*)rv + sizeof(EJSPrimString));
    jschar *p = rv->data.flat;
    const unsigned char *stru = (const unsigned char*)str;
    while (*stru) {
        *p++ = utf8_to_ucs2 (stru, &stru);
        len--;
        if (len == 0)
            break;
    }
    return STRING_TO_EJSVAL(rv);
}

ejsval
_ejs_string_new_ucs2 (const jschar* str)
{
    int str_len = ucs2_strlen(str);
    size_t value_size = sizeof(EJSPrimString) + sizeof(jschar) * (str_len + 1);

    EJSPrimString* rv = _ejs_gc_new_primstr(value_size);
    EJS_PRIMSTR_SET_TYPE(rv, EJS_STRING_FLAT);
    rv->length = str_len;
    rv->data.flat = (jschar*)((char*)rv + sizeof(EJSPrimString));
    memmove (rv->data.flat, str, str_len * sizeof(jschar));
    return STRING_TO_EJSVAL(rv);
}

ejsval
_ejs_string_new_ucs2_len (const jschar* str, int len)
{
    size_t value_size = sizeof(EJSPrimString) + sizeof(jschar) * (len + 1);

    EJSPrimString* rv = _ejs_gc_new_primstr(value_size);
    EJS_PRIMSTR_SET_TYPE(rv, EJS_STRING_FLAT);
    rv->length = len;
    rv->data.flat = (jschar*)((char*)rv + sizeof(EJSPrimString));
    memmove (rv->data.flat, str, len * sizeof(jschar));
    return STRING_TO_EJSVAL(rv);
}

ejsval
_ejs_string_new_substring (ejsval str, int off, int len)
{
    // XXX we should probably validate off/len here..

    EJSPrimString *prim_str = EJSVAL_TO_STRING(str);
    EJSPrimString* rv = _ejs_gc_new_primstr(sizeof(EJSPrimString));
    EJS_PRIMSTR_SET_TYPE(rv, EJS_STRING_DEPENDENT);
    rv->length = len;
    rv->data.dependent.dep = prim_str;
    rv->data.dependent.off = off;
    return STRING_TO_EJSVAL(rv);
}

ejsval
_ejs_string_concat (ejsval left, ejsval right)
{
    EJSPrimString* lhs = EJSVAL_TO_STRING(left);
    EJSPrimString* rhs = EJSVAL_TO_STRING(right);
    
    EJSPrimString* rv = _ejs_gc_new_primstr (sizeof(EJSPrimString));
    EJS_PRIMSTR_SET_TYPE(rv, EJS_STRING_ROPE);
    rv->length = lhs->length + rhs->length;
    rv->data.rope.left = lhs;
    rv->data.rope.right = rhs;

    return STRING_TO_EJSVAL(rv);
}

ejsval
_ejs_string_concatv (ejsval first, ...)
{
    ejsval result = first;
    ejsval arg;

    va_list ap;

    va_start(ap, first);

    arg = va_arg(ap, ejsval);
    do {
        result = _ejs_string_concat (result, arg);
        arg = va_arg(ap, ejsval);
    } while (!EJSVAL_IS_NULL_OR_UNDEFINED(arg));

    va_end(ap);

    return result;
}

static void flatten_dep (jschar **p, EJSPrimString *n, int* off, int* len);

static void
flatten_rope (jschar **p, EJSPrimString *n)
{
    switch (EJS_PRIMSTR_GET_TYPE(n)) {
    case EJS_STRING_FLAT:
        memmove (*p, n->data.flat, n->length * sizeof(jschar));
        *p += n->length;
        break;
    case EJS_STRING_ROPE:
        flatten_rope(p, n->data.rope.left);
        flatten_rope(p, n->data.rope.right);
        break;
    case EJS_STRING_DEPENDENT: {
        int off = n->data.dependent.off;
        int len = n->length;
        flatten_dep (p, n->data.dependent.dep, &off, &len);
        break;
    }
    default:
        EJS_NOT_IMPLEMENTED();
    }
}

static void
flatten_dep (jschar **p, EJSPrimString *n, int* off, int* len)
{
    // nothing else to append
    if (*len == 0)
        return;

    // first step is to locate the node that contains the start of our characters
    if (*off >= 0) {
        switch (EJS_PRIMSTR_GET_TYPE(n)) {
        case EJS_STRING_FLAT: {
            if (*off < n->length) {
                // we handle the first append here
                int length_to_append = MIN(*len, n->length - *off);
                memmove (*p, n->data.flat + *off, length_to_append * sizeof(jschar));
                *p += length_to_append;
                *len -= length_to_append;
                *off = 0;
            }
            else {
                *off -= n->length;
            }
            break;
        }
        case EJS_STRING_ROPE:
            flatten_dep(p, n->data.rope.left, off, len);
            flatten_dep(p, n->data.rope.right, off, len);
            break;
        case EJS_STRING_DEPENDENT: {
            *off += n->data.dependent.off;
            flatten_dep(p, n->data.dependent.dep, off, len);
            break;
        }
        default:
            EJS_NOT_IMPLEMENTED();
        }
    }
    else {
        // we need to append len characters from this string into the buffer
        switch (EJS_PRIMSTR_GET_TYPE(n)) {
        case EJS_STRING_FLAT: {
            int length_to_append = MIN (*len, n->length);
            memmove (*p, n->data.flat, length_to_append * sizeof(jschar));
            *p += length_to_append;
            *len -= length_to_append;
            break;
        }
        case EJS_STRING_ROPE:
            flatten_dep(p, n->data.rope.left, off, len);
            flatten_dep(p, n->data.rope.right, off, len);
            break;
        case EJS_STRING_DEPENDENT: {
            int length_to_append = MIN (*len, n->length);
            flatten_dep (p, n->data.dependent.dep, off, &length_to_append);
            *len -= length_to_append;
            break;
        }
        default:
            EJS_NOT_IMPLEMENTED();
        }
    }
}

EJSPrimString*
_ejs_primstring_flatten (EJSPrimString* primstr)
{
    if (EJS_PRIMSTR_GET_TYPE(primstr) == EJS_STRING_FLAT)
        return primstr;

    jschar *buffer = (jschar*)calloc(sizeof(jschar), primstr->length + 1);
    jschar *p = buffer;

    switch (EJS_PRIMSTR_GET_TYPE(primstr)) {
    case EJS_STRING_DEPENDENT: {
        // modify the string in-place, switching from a dep to a flat string
        int off = 0;
        int length = primstr->length;
        flatten_dep (&p, primstr, &off, &length);
        //EJS_ASSERT (off == 0);
        //EJS_ASSERT (length == 0);
        break;
    }
    case EJS_STRING_ROPE: {
        // modify the string in-place, switching from a rope to a flat string
        flatten_rope (&p, primstr);
        break;
    }
    default:
        EJS_NOT_REACHED();
    }

    EJS_PRIMSTR_CLEAR_TYPE(primstr);
    EJS_PRIMSTR_SET_TYPE(primstr, EJS_STRING_FLAT);
    primstr->data.flat = buffer;
    return primstr;
}

EJSPrimString*
_ejs_string_flatten (ejsval str)
{
    return _ejs_primstring_flatten (EJSVAL_TO_STRING_IMPL(str));
}

static uint32_t
hash_dep (int hash, EJSPrimString* n, int* off, int* len)
{
    // nothing left to hash
    if (*len == 0)
        return hash;

    // first step is to locate the node that contains the start of our characters
    if (*off >= 0) {
        switch (EJS_PRIMSTR_GET_TYPE(n)) {
        case EJS_STRING_FLAT:
            if (*off < n->length) {
                int length_to_hash = MIN(*len, n->length - *off);
                hash = ucs2_hash (n->data.flat + *off, hash, length_to_hash);
                *len -= length_to_hash;
                *off = 0;
            }
            else {
                *off -= n->length;
            }
            break;
        case EJS_STRING_ROPE:
            hash = hash_dep (hash, n->data.rope.left, off, len);
            hash = hash_dep (hash, n->data.rope.right, off, len);
            break;
        case EJS_STRING_DEPENDENT:
            *off += n->data.dependent.off;
            hash = hash_dep(hash, n->data.dependent.dep, off, len);
            break;
        }
    }
    return hash;
}

static uint32_t
_ejs_primstring_hash_inner (EJSPrimString* primstr, int cur_hash)
{
    switch (EJS_PRIMSTR_GET_TYPE(primstr)) {
    case EJS_STRING_FLAT:
        return ucs2_hash (primstr->data.flat, cur_hash, primstr->length);
    case EJS_STRING_DEPENDENT: {
        int length = primstr->length;
        int off = 0;
        return hash_dep (cur_hash, primstr, &off, &length);
    }
    case EJS_STRING_ROPE: {
        int hash = _ejs_primstring_hash_inner (primstr->data.rope.left, cur_hash);
        return _ejs_primstring_hash_inner (primstr->data.rope.right, hash);
    }
    }
}

uint32_t
_ejs_primstring_hash (EJSPrimString* primstr)
{
    if (!EJS_PRIMSTR_HAS_HASH(primstr)) {
        primstr->hash = _ejs_primstring_hash_inner (primstr, 0);
        EJS_PRIMSTR_SET_HAS_HASH(primstr);
    }
    return primstr->hash;
}

uint32_t
_ejs_string_hash (ejsval str)
{
    EJS_ASSERT (EJSVAL_IS_STRING(str));

    return _ejs_primstring_hash (EJSVAL_TO_STRING_IMPL(str));
}

jschar
_ejs_string_char_code_at(EJSPrimString* primstr, int i)
{
    if (i >= primstr->length) {
        printf ("char_code_at error\n");
        return (jschar)-1;
    }

    switch (EJS_PRIMSTR_GET_TYPE(primstr)) {
    case EJS_STRING_FLAT:
        return primstr->data.flat[i];
    case EJS_STRING_ROPE: {
        if (i >= primstr->data.rope.left->length) {
            return _ejs_string_char_code_at (primstr->data.rope.left, i);
        }
        else {
            return _ejs_string_char_code_at (primstr->data.rope.right, i - primstr->data.rope.left->length);
        }
    }
    case EJS_STRING_DEPENDENT: {
        return _ejs_string_char_code_at (primstr->data.dependent.dep, i + primstr->data.dependent.off);
    }
    default:
        EJS_NOT_IMPLEMENTED();
    }
}

char*
_ejs_string_to_utf8(EJSPrimString* primstr)
{
    int length = primstr->length * 4+1;
    char* buf = (char*)malloc(length);
    char *p = buf;

    for (int i = 0; i < primstr->length; i ++) {
        jschar ucs2 = _ejs_string_char_code_at(primstr, i);
        int adv = ucs2_to_utf8_char (ucs2, p);
        if (adv < 1) {
            printf ("error converting ucs2 to utf8, index %d\n", i);
            free(buf);
            return NULL;
        }
        p += adv;
    }

    *p = 0;

    return buf;
}

void
_ejs_string_init_literal (const char *name, ejsval *val, EJSPrimString* str, jschar* ucs2_data, int32_t length)
{
    str->length = length;
    str->hash = 0;
    str->gc_header = (EJS_STRING_FLAT<<EJS_GC_USER_FLAGS_SHIFT);
    str->data.flat = ucs2_data;
    *val = STRING_TO_EJSVAL(str);
}

char*
_ejs_string_describe (ejsval str)
{
    return ucs2_to_utf8(EJSVAL_TO_FLAT_STRING(str));
}

char*
_ejs_string_describe_prim (EJSPrimString *str)
{
    return ucs2_to_utf8(_ejs_primstring_flatten(str)->data.flat);
}
