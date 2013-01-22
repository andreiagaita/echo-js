/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99 ft=cpp:
 */

#include "ejs-llvm.h"
#include "ejs-object.h"
#include "ejs-value.h"
#include "ejs-function.h"
#include "ejs-array.h"
#include "ejs-string.h"

#include "constant.h"
#include "type.h"
#include "value.h"

ejsval _ejs_llvm_Constant_proto;
ejsval _ejs_llvm_Constant;
static ejsval
_ejs_llvm_Constant_impl (ejsval env, ejsval _this, int argc, ejsval *args)
{
    EJS_NOT_IMPLEMENTED();
}

static ejsval
_ejs_llvm_Constant_getNull (ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_TYPE_ARG (0, ty);
    return _ejs_llvm_Value_new (llvm::Constant::getNullValue(ty));
}

static ejsval
_ejs_llvm_Constant_getAggregateZero (ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_TYPE_ARG (0, ty);

    return _ejs_llvm_Value_new (llvm::ConstantAggregateZero::get(ty));
}

static ejsval
_ejs_llvm_Constant_getBoolValue (ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_BOOL_ARG (0, b);

    return _ejs_llvm_Value_new (llvm::Constant::getIntegerValue(llvm::Type::getInt8Ty(llvm::getGlobalContext()), llvm::APInt(8, b?1:0)));
}

static ejsval
_ejs_llvm_Constant_getIntegerValue (ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_TYPE_ARG (0, ty);
    REQ_INT_ARG (1, v);

    return _ejs_llvm_Value_new (llvm::Constant::getIntegerValue(ty, llvm::APInt(ty->getPrimitiveSizeInBits(), v)));
}

void
_ejs_llvm_Constant_init (ejsval exports)
{
    START_SHADOW_STACK_FRAME;

    _ejs_gc_add_named_root (_ejs_llvm_Constant_proto);
    _ejs_llvm_Constant_proto = _ejs_object_create(_ejs_Object_prototype);

    ADD_STACK_ROOT(ejsval, tmpobj, _ejs_function_new_utf8 (_ejs_null, "LLVMConstant", (EJSClosureFunc)_ejs_llvm_Constant_impl));
    _ejs_llvm_Constant = tmpobj;


#define OBJ_METHOD(x) EJS_INSTALL_FUNCTION(_ejs_llvm_Constant, EJS_STRINGIFY(x), _ejs_llvm_Constant_##x)
#define PROTO_METHOD(x) EJS_INSTALL_FUNCTION(_ejs_llvm_Constant_proto, EJS_STRINGIFY(x), _ejs_llvm_Constant_prototype_##x)

    _ejs_object_setprop (_ejs_llvm_Constant,       _ejs_atom_prototype,  _ejs_llvm_Constant_proto);

    OBJ_METHOD(getNull);
    OBJ_METHOD(getAggregateZero);
    OBJ_METHOD(getBoolValue);
    OBJ_METHOD(getIntegerValue);

#undef PROTO_METHOD
#undef OBJ_METHOD

    _ejs_object_setprop_utf8 (exports,              "Constant", _ejs_llvm_Constant);

    END_SHADOW_STACK_FRAME;
}

