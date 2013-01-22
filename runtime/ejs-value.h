/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99 ft=cpp:
 */

#ifndef _ejs_value_h
#define _ejs_value_h

#include "ejs.h"
#include "ejs-gc.h"

typedef double EJSPrimNumber;

#define EJSVAL_IS_PRIMITIVE(v) (EJSVAL_IS_NUMBER(v) || EJSVAL_IS_STRING(v) || EJSVAL_IS_BOOLEAN(v) || EJSVAL_IS_UNDEFINED(v))

#define EJSVAL_IS_OBJECT(v)    EJSVAL_IS_OBJECT_IMPL(v)
#define EJSVAL_IS_ARRAY(v)     (EJSVAL_IS_OBJECT(v) && (EJSVAL_TO_OBJECT(v)->ops == &_ejs_array_specops || EJSVAL_TO_OBJECT(v)->ops == &_ejs_sparsearray_specops))
#define EJSVAL_IS_FUNCTION(v)  (EJSVAL_IS_OBJECT(v) && (EJSVAL_TO_OBJECT(v)->ops == &_ejs_function_specops))
#define EJSVAL_IS_DATE(v)      (EJSVAL_IS_OBJECT(v) && (EJSVAL_TO_OBJECT(v)->ops == &_ejs_date_specops))
#define EJSVAL_IS_REGEXP(v)    (EJSVAL_IS_OBJECT(v) && (EJSVAL_TO_OBJECT(v)->ops == &_ejs_regexp_specops))
#define EJSVAL_IS_NUMBER_OBJECT(v) (EJSVAL_IS_OBJECT(v) && (EJSVAL_TO_OBJECT(v)->ops == &_ejs_number_specops))
#define EJSVAL_IS_STRING_OBJECT(v) (EJSVAL_IS_OBJECT(v) && (EJSVAL_TO_OBJECT(v)->ops == &_ejs_string_specops))
#define EJSVAL_IS_BOOLEAN_OBJECT(v) (EJSVAL_IS_OBJECT(v) && (EJSVAL_TO_OBJECT(v)->ops == &_ejs_boolean_specops))
#define EJSVAL_IS_NUMBER(v)    EJSVAL_IS_DOUBLE_IMPL(v)
#define EJSVAL_IS_STRING(v)    EJSVAL_IS_STRING_IMPL(v)
#define EJSVAL_IS_BOOLEAN(v)   EJSVAL_IS_BOOLEAN_IMPL(v)
#define EJSVAL_IS_UNDEFINED(v) EJSVAL_IS_UNDEFINED_IMPL(v)
#define EJSVAL_IS_NULL(v)      EJSVAL_IS_NULL_IMPL(v)
#define EJSVAL_IS_OBJECT_OR_NULL(v) EJSVAL_IS_OBJECT_OR_NULL_IMPL(v)

#define EJSVAL_TO_OBJECT(v)       EJSVAL_TO_OBJECT_IMPL(v)
#define EJSVAL_TO_NUMBER(v)       v.asDouble
#define EJSVAL_TO_BOOLEAN(v)      EJSVAL_TO_BOOLEAN_IMPL(v)
#define EJSVAL_TO_FUNC(v)         ((EJSFunction*)EJSVAL_TO_OBJECT_IMPL(v))->func
#define EJSVAL_TO_ENV(v)          ((EJSFunction*)EJSVAL_TO_OBJECT_IMPL(v))->env

#define OBJECT_TO_EJSVAL(v)       OBJECT_TO_EJSVAL_IMPL(v)
#define BOOLEAN_TO_EJSVAL(v)      BOOLEAN_TO_EJSVAL_IMPL(v)
#define NUMBER_TO_EJSVAL(v)       DOUBLE_TO_EJSVAL_IMPL(v)
#define STRING_TO_EJSVAL(v)       STRING_TO_EJSVAL_IMPL(v)

#define EJSVAL_EQ(v1,v2)          ((v1).asBits == (v2).asBits)

#define EJS_NUMBER_FORMAT "%g"

EJS_BEGIN_DECLS

void _ejs_dump_value (ejsval val);

ejsval _ejs_number_new (double value);

void _ejs_value_finalize(ejsval val);

typedef void (*EJSValueFunc)(ejsval value);

EJS_END_DECLS

#endif /* _ejs_value_h */
