/****************************************************************************\
Copyright (c) 2002, NVIDIA Corporation.

NVIDIA Corporation("NVIDIA") supplies this software to you in
consideration of your agreement to the following terms, and your use,
installation, modification or redistribution of this NVIDIA software
constitutes acceptance of these terms.  If you do not agree with these
terms, please do not use, install, modify or redistribute this NVIDIA
software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, NVIDIA grants you a personal, non-exclusive
license, under NVIDIA's copyrights in this original NVIDIA software (the
"NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from the
NVIDIA Software without specific prior written permission from NVIDIA.
Except as expressly stated in this notice, no other rights or licenses
express or implied, are granted by NVIDIA herein, including but not
limited to any patent rights that may be infringed by your derivative
works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder. 

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY
OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE
NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT,
TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// symbols.c
//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "cg_stdlib.h"

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Symbol Table Variables: ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

Scope *ScopeList = NULL;
Scope *CurrentScope = NULL;
Scope *GlobalScope = NULL;
int NextFunctionIndex = 0;
static int NextSymbolSourceOrdinal = 1;

Type *UndefinedType = NULL;
Type *CFloatType = NULL;
Type *CIntType = NULL;
Type *VoidType = NULL;
Type *FloatType = NULL;
Type *IntType = NULL;
Type *BooleanType = NULL;

Type *CFloat1Type = NULL;
Type *CFloat2Type = NULL;
Type *CFloat3Type = NULL;
Type *CFloat4Type = NULL;

Type *CInt1Type = NULL;
Type *CInt2Type = NULL;
Type *CInt3Type = NULL;
Type *CInt4Type = NULL;

Type *Float1Type = NULL;
Type *Float2Type = NULL;
Type *Float3Type = NULL;
Type *Float4Type = NULL;

Type *Int1Type = NULL;
Type *Int2Type = NULL;
Type *Int3Type = NULL;
Type *Int4Type = NULL;

Type *Boolean1Type = NULL;
Type *Boolean2Type = NULL;
Type *Boolean3Type = NULL;
Type *Boolean4Type = NULL;

Symbol *FalseSymb = NULL;
Symbol *TrueSymb = NULL;

static int baseTypeNames[TYPE_BASE_LAST_USER + 1] = { 0 };
static Type *baseTypes[TYPE_BASE_LAST_USER + 1] = { NULL };

// Canonical kinds with user-declarable spellings, in registration order:

static const CgScalarKind declarationKinds[] = {
    CG_SCALAR_CHAR, CG_SCALAR_UCHAR, CG_SCALAR_SHORT, CG_SCALAR_USHORT,
    CG_SCALAR_INT, CG_SCALAR_UINT, CG_SCALAR_LONG, CG_SCALAR_ULONG,
    CG_SCALAR_FIXED, CG_SCALAR_HALF, CG_SCALAR_FLOAT, CG_SCALAR_DOUBLE,
    CG_SCALAR_BOOL
};

// Typedef base name for each canonical kind, indexed by kind:

static const char *standardTypeNames[CG_SCALAR_COUNT] = {
    NULL,      // CG_SCALAR_NONE
    NULL,      // CG_SCALAR_UNDEFINED
    "cfloat",  // CG_SCALAR_CFLOAT
    "cint",    // CG_SCALAR_CINT
    "bool",
    "char",
    "uchar",
    "short",
    "ushort",
    "int",
    "uint",
    "long",
    "ulong",
    "fixed",
    "half",
    "float",
    "double"
};

/************************************ Type Name Error Support ********************************/

/*
 * SetScalarTypeName() - Set a scalar type name.
 *
 */

void SetScalarTypeName(int base, int name, Type *fType)
{
    if (base >= 0 && base <= TYPE_BASE_LAST_USER) {
        baseTypeNames[base] = name;
        baseTypes[base] = fType;
    }
} // SetScalarTypeName

/*
 * Canonical sampler spellings, shared by typedef registration and the
 * legacy-base name table (sampler bases sit inside the profile range).
 */

static const struct {
    const char *name;
    CgSamplerKind kind;
} cgSamplerSpellings[] = {
    { "sampler",     CG_SAMPLER_BASE },
    { "sampler1D",   CG_SAMPLER_1D },
    { "sampler2D",   CG_SAMPLER_2D },
    { "sampler3D",   CG_SAMPLER_3D },
    { "samplerCUBE", CG_SAMPLER_CUBE },
    { "samplerRECT", CG_SAMPLER_RECT }
};

/*
 * RegisterStandardTypeSpellings() - Add typedef symbols for the scalar and
 *                                    vector spellings of one canonical kind.
 *
 * Matrix spellings are registered separately (see
 * RegisterMatrixSpellings): the language now owns the canonical
 * float1x1-float4x4 names, and stdlib.cg no longer redefines them.
 *
 */

static void RegisterStandardTypeSpellings(SourceLoc *loc, Scope *fScope,
                                          CgScalarKind kind, const char *base)
{
    char spelling[16];
    Type *fType;
    int len;

    fType = GetStandardTypeKind(kind, 0, 0);
    AddSymbol(loc, fScope, LookUpAddString(atable, base), fType, TYPEDEF_S);
    for (len = 1; len <= 4; len++) {
        sprintf(spelling, "%s%d", base, len);
        fType = GetStandardTypeKind(kind, len, 0);
        AddSymbol(loc, fScope, LookUpAddString(atable, spelling), fType, TYPEDEF_S);
    }
} // RegisterStandardTypeSpellings

/*
 * RegisterMatrixSpellings() - Add typedef symbols for the canonical
 *                             matrix spellings of one canonical kind
 *                             over every interned (rows, columns)
 *                             shape.  The spellings alias the registry
 *                             types directly, so a predefined float4x3
 *                             is the same type the old stdlib.cg
 *                             "typedef packed float3 float4x3[4]"
 *                             produced.
 */

static void RegisterMatrixSpellings(SourceLoc *loc, Scope *fScope,
                                    CgScalarKind kind, const char *base)
{
    char spelling[16];
    Type *fType;
    int rows, columns;

    for (rows = 1; rows <= 4; rows++) {
        for (columns = 1; columns <= 4; columns++) {
            sprintf(spelling, "%s%dx%d", base, rows, columns);
            fType = GetStandardTypeKind(kind, rows, columns);
            AddSymbol(loc, fScope, LookUpAddString(atable, spelling),
                      fType, TYPEDEF_S);
        }
    }
} // RegisterMatrixSpellings

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Symbol Table Fuctions: ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * InitSymbolTable()
 *
 */

int InitSymbolTable(CgStruct *Cg)
{
    SourceLoc dummyLoc = { 0, 0 };
    int ii, name;

    NextSymbolSourceOrdinal = 1;

    // Intern the canonical standard types referenced below:

    InitCgStandardTypes();

    // Create the super-global scope and add predefined types and symbols:

    PushScope(NewScopeInPool(mem_CreatePool(0, 0)));
    UndefinedType = NewType(TYPE_BASE_UNDEFINED_TYPE | TYPE_CATEGORY_SCALAR, 0);
    VoidType = NewType(TYPE_BASE_VOID | TYPE_CATEGORY_SCALAR | TYPE_MISC_VOID, 0);

    // The predefined scalars and packed vectors alias the interned
    // standard-type registry:

    CFloatType = GetStandardTypeKind(CG_SCALAR_CFLOAT, 0, 0);
    CIntType = GetStandardTypeKind(CG_SCALAR_CINT, 0, 0);
    FloatType = GetStandardTypeKind(CG_SCALAR_FLOAT, 0, 0);
    IntType = GetStandardTypeKind(CG_SCALAR_INT, 0, 0);
    BooleanType = GetStandardTypeKind(CG_SCALAR_BOOL, 0, 0);

    CFloat1Type = GetStandardTypeKind(CG_SCALAR_CFLOAT, 1, 0);
    CFloat2Type = GetStandardTypeKind(CG_SCALAR_CFLOAT, 2, 0);
    CFloat3Type = GetStandardTypeKind(CG_SCALAR_CFLOAT, 3, 0);
    CFloat4Type = GetStandardTypeKind(CG_SCALAR_CFLOAT, 4, 0);
    CInt1Type = GetStandardTypeKind(CG_SCALAR_CINT, 1, 0);
    CInt2Type = GetStandardTypeKind(CG_SCALAR_CINT, 2, 0);
    CInt3Type = GetStandardTypeKind(CG_SCALAR_CINT, 3, 0);
    CInt4Type = GetStandardTypeKind(CG_SCALAR_CINT, 4, 0);
    Float1Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 1, 0);
    Float2Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 2, 0);
    Float3Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 3, 0);
    Float4Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 0);
    Int1Type = GetStandardTypeKind(CG_SCALAR_INT, 1, 0);
    Int2Type = GetStandardTypeKind(CG_SCALAR_INT, 2, 0);
    Int3Type = GetStandardTypeKind(CG_SCALAR_INT, 3, 0);
    Int4Type = GetStandardTypeKind(CG_SCALAR_INT, 4, 0);
    Boolean1Type = GetStandardTypeKind(CG_SCALAR_BOOL, 1, 0);
    Boolean2Type = GetStandardTypeKind(CG_SCALAR_BOOL, 2, 0);
    Boolean3Type = GetStandardTypeKind(CG_SCALAR_BOOL, 3, 0);
    Boolean4Type = GetStandardTypeKind(CG_SCALAR_BOOL, 4, 0);

    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cfloat"), CFloatType, TYPEDEF_S);
    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cint"), CIntType, TYPEDEF_S);
    AddSymbol(&dummyLoc, CurrentScope, VOID_SY, VoidType, TYPEDEF_S);

    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cfloat1"), CFloat1Type, TYPEDEF_S);
    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cfloat2"), CFloat2Type, TYPEDEF_S);
    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cfloat3"), CFloat3Type, TYPEDEF_S);
    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cfloat4"), CFloat4Type, TYPEDEF_S);
    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cint1"), CInt1Type, TYPEDEF_S);
    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cint2"), CInt2Type, TYPEDEF_S);
    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cint3"), CInt3Type, TYPEDEF_S);
    AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "cint4"), CInt4Type, TYPEDEF_S);

    for (ii = 0; ii < sizeof(declarationKinds)/sizeof(declarationKinds[0]); ii++) {
        RegisterStandardTypeSpellings(&dummyLoc, CurrentScope,
                                      declarationKinds[ii],
                                      standardTypeNames[declarationKinds[ii]]);
    }

    // Canonical float matrix spellings are language-level predefined
    // types (the stdlib.cg source-level duplicates are gone):

    RegisterMatrixSpellings(&dummyLoc, CurrentScope, CG_SCALAR_FLOAT, "float");

    // Canonical sampler typedefs are language types in every profile;
    // profile backends adapt them to their texture-object bases.

    for (ii = 0; ii < (int) (sizeof(cgSamplerSpellings) /
                             sizeof(cgSamplerSpellings[0])); ii++)
    {
        int atom = LookUpAddString(atable, cgSamplerSpellings[ii].name);
        AddSymbol(&dummyLoc, CurrentScope, atom,
                  GetSamplerType(cgSamplerSpellings[ii].kind), TYPEDEF_S);
    }

    FalseSymb = AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "false"), BooleanType, CONSTANT_S);
    TrueSymb = AddSymbol(&dummyLoc, CurrentScope, LookUpAddString(atable, "true"), BooleanType, CONSTANT_S);
    FalseSymb->details.con.value = 0;
    TrueSymb->details.con.value = 1;

    SetScalarTypeName(TYPE_BASE_NO_TYPE, LookUpAddString(atable, "***no-base-type***"), UndefinedType);
    SetScalarTypeName(TYPE_BASE_UNDEFINED_TYPE, LookUpAddString(atable, "***undefined-base-type***"), UndefinedType);
    SetScalarTypeName(TYPE_BASE_CFLOAT, LookUpAddString(atable, "cfloat"), CFloatType);
    SetScalarTypeName(TYPE_BASE_CINT, LookUpAddString(atable, "cint"), CIntType);
    SetScalarTypeName(TYPE_BASE_VOID, LookUpAddString(atable, "void"), VoidType);
    SetScalarTypeName(TYPE_BASE_FLOAT, LookUpAddString(atable, "float"), FloatType);
    SetScalarTypeName(TYPE_BASE_INT, LookUpAddString(atable, "int"), IntType);
    SetScalarTypeName(TYPE_BASE_BOOLEAN, LookUpAddString(atable, "bool"), BooleanType);

    name = LookUpAddString(atable, "***unknown-profile-base-type***");
    for (ii = TYPE_BASE_FIRST_USER; ii <= TYPE_BASE_LAST_USER; ii++)
        SetScalarTypeName(ii, name, UndefinedType);

    // Sampler legacy bases live inside the profile base range, but they
    // are language-owned canonical types, not profile extensions: keep
    // their names past the unknown-base reset above.

    for (ii = 0; ii < (int) (sizeof(cgSamplerSpellings) /
                             sizeof(cgSamplerSpellings[0])); ii++)
    {
        SetScalarTypeName(
            CgSamplerLegacyBase(cgSamplerSpellings[ii].kind),
            LookUpAddString(atable, cgSamplerSpellings[ii].name),
            GetSamplerType(cgSamplerSpellings[ii].kind));
    }

    // Add profile specific symbols and types:

    Cg->theHAL->RegisterNames(Cg->theHAL);
    AddAtom(atable, "<*** end hal specific atoms ***>");

    // Install the declarative Cg 2.0 standard library over the
    // super-global scope: every catalog signature becomes an ordinary
    // internal function symbol, and the helper-structure variants
    // follow the selected profile family.  This runs before stdlib.cg
    // parses, so portable bodies in that file merge onto the matching
    // catalog symbols.

    InitCgStdlib(CurrentScope);

    // Initialize misc. other globals:

    CurrentDeclTypeSpecs.basetype = UndefinedType;
    CurrentDeclTypeSpecs.IsDerived = 0;
    CurrentDeclTypeSpecs.type = *UndefinedType;

    return 1;
} // InitSymbolTable

int StartGlobalScope(CgStruct *Cg)
{
    // Create user's global scope:
    GlobalScope = NewScopeInPool(mem_CreatePool(0, 0));
    PushScope(GlobalScope);
    return 1;
} // StartGlobalScope

/*
 * FreeSymbolTable()
 *
 */

int FreeSymbolTable(CgStruct *Cg)
{
    Scope *lScope, *nScope;

    lScope = ScopeList;
    while (lScope) {
        nScope = lScope->next;
        // FreeEverythingOwnedByScope(pScope);
        lScope = nScope;
    }
    FreeCgStandardTypes();
    FreeCgSamplerTypes();
    return 1;
} // FreeSymbolTable

static void unlinkScope(void *_scope) {
    Scope *scope = _scope;

    if (scope->next)
        scope->next->prev = scope->prev;
    if (scope->prev)
        scope->prev->next = scope->next;
    else
        ScopeList = scope->next;
}

/*
 * NewScope()
 *
 */
Scope *NewScopeInPool(MemoryPool *pool)
{
    Scope *lScope;

    lScope = mem_Alloc(pool, sizeof(Scope));
    lScope->pool = pool;
    lScope->parent = NULL;
    lScope->funScope = NULL;
    lScope->symbols = NULL;
    lScope->tags = NULL;
    lScope->params = NULL;
    lScope->returnType = NULL;
    lScope->level = 0;
    lScope->funindex = 0;
    lScope->InFormalParameters = 0;
    lScope->HasVoidParameter = 0;
    lScope->HasReturnStmt = 0;
    lScope->IsStructScope = 0;
    lScope->HasSemantics = 0;
    lScope->pid = PID_NONE_ID;
    lScope->programs = NULL;
    lScope->initStmts = NULL;
    if ((lScope->next = ScopeList))
        ScopeList->prev = lScope;
    lScope->prev = 0;
    ScopeList = lScope;
    mem_AddCleanup(pool, unlinkScope, lScope);
    return lScope;
} // NewScope

/*
 * PushScope()
 *
 */

void PushScope(Scope *fScope)
{
    Scope *lScope;

    if (CurrentScope) {
        fScope->level = CurrentScope->level + 1;
        if (fScope->level == 1) {
            if (!GlobalScope) {
                /* HACK - CTD -- if GlobalScope==NULL and level==1, we're
                 * defining a function in the superglobal scope.  Things
                 * will break if we leave the level as 1, so we arbitrarily
                 * set it to 2 */
                fScope->level = 2;
            }
        }
        if (fScope->level >= 2) {
            lScope = fScope;
            while (lScope->level > 2)
                lScope = lScope->next;
            fScope->funScope = lScope;
        }
    } else {
        fScope->level = 0;
    }
    fScope->parent = CurrentScope;
    CurrentScope = fScope;
} // PushScope

/*
 * PopScope()
 *
 */

Scope *PopScope(void)
{
    Scope *lScope;

    lScope = CurrentScope;
    if (CurrentScope)
        CurrentScope = CurrentScope->parent;
    return lScope;
} // PopScope

/*
 * NewSymbol() - Allocate a new symbol node;
 *
 */

Symbol *NewSymbol(SourceLoc *loc, Scope *fScope, int name, Type *fType, symbolkind kind)
{
    Symbol *lSymb;
    char *pch;
    int ii;

    lSymb = (Symbol *) mem_Alloc(fScope->pool, sizeof(Symbol));
    lSymb->left = NULL;
    lSymb->right = NULL;
    lSymb->next = NULL;
    lSymb->name = name;
    lSymb->storageClass = SC_UNKNOWN;
    lSymb->type = fType;
    lSymb->loc = *loc;
    lSymb->sourceOrdinal = NextSymbolSourceOrdinal++;
    lSymb->kind = kind;
    lSymb->properties = 0;
    lSymb->flags = 0;
    lSymb->tempptr = NULL;
    lSymb->tempptr2 = NULL;
    
    // Clear union area:

    pch = (char *) &lSymb->details;
    for (ii = 0; ii < sizeof(lSymb->details); ii++)
        *pch++ = 0;
    /* Unqualified function declarations carry the open profile
     * selector until a profile specifier qualifies them. */
    if (kind == FUNCTION_S) {
        lSymb->details.fun.profileSelector.isOpen = 1;
    }
    return lSymb;
} // NewSymbol

/*
 * lAddToTree() - Using a binary tree is not a good idea for basic atom values because they
 *         are generated in order.  We'll fix this later (by reversing the bit pattern).
 */

static void lAddToTree(Symbol **fSymbols, Symbol *fSymb, Type *fType)
{
    Symbol *lSymb;
    int lrev, frev;

    lSymb = *fSymbols;
    if (lSymb) {
        frev = GetReversedAtom(atable, fSymb->name);
        while (lSymb) {
            lrev = GetReversedAtom(atable, lSymb->name);
            if (lrev == frev) {
                InternalError(Cg->tokenLoc, 9999, "symbol \"%s\" already in table",
                       GetAtomString(atable, fSymb->name));
                break;
            } else {
                if (lrev > frev) {
                    if (lSymb->left) {
                        lSymb = lSymb->left;
                    } else {
                        lSymb->left = fSymb;
                        break;
                    }
                } else {
                    if (lSymb->right) {
                        lSymb = lSymb->right;
                    } else {
                        lSymb->right = fSymb;
                        break;
                    }
                }
            }
        }
    } else {
        *fSymbols = fSymb;
    }
} // lAddToTree


/*
 * AddSymbol() - Add a variable, type, or function name to a scope.
 *
 */

Symbol *AddSymbol(SourceLoc *loc, Scope *fScope, int atom, Type *fType, symbolkind kind)
{
    Symbol *lSymb;

    if (!fScope)
        fScope = CurrentScope;
    lSymb = NewSymbol(loc, fScope, atom, fType, kind);
    lAddToTree(&fScope->symbols, lSymb, fType);
    return lSymb;
} // AddSymbol

/*
 * UniqueSymbol() - Add a symbol to fScope that is different from
 * every other symbol.  Useful for compiler generated temporaries.
 *
 */

Symbol *UniqueSymbol(Scope *fScope, Type *fType, symbolkind kind)
{
    static int nextTmp = 0;
    static SourceLoc tmpLoc = { 0, 0 };
    char buf[256];
    int atom;
  
    sprintf(buf, "@TMP%d", nextTmp++);
    atom = AddAtom(atable, buf);
    return AddSymbol(&tmpLoc, fScope, atom, fType, kind);
} // UniqueSymbol


/*
 * AddTag() - Add a tag name to a scope.
 *
 */

Symbol *AddTag(SourceLoc *loc, Scope *fScope, int atom, int category)
{
    Symbol *lSymb;
    Type *pType;

    if (!fScope)
        fScope = CurrentScope;
    pType = NewType(category, 0);
    pType->str.unqualifiedtype = pType;
    lSymb = NewSymbol(loc, fScope, atom, pType, TAG_S);
    lAddToTree(&fScope->tags, lSymb, pType);
    return lSymb;
} // AddTag

/*********************************************************************************************/
/***************************************** Type Functions ************************************/
/*********************************************************************************************/

/*
 * lKindFromLegacyBase() - Map legacy four-bit base properties to the
 *                         canonical scalar kind.
 *
 */

static CgScalarKind lKindFromLegacyBase(int base)
{
    switch (base) {
    case TYPE_BASE_CFLOAT: return CG_SCALAR_CFLOAT;
    case TYPE_BASE_CINT: return CG_SCALAR_CINT;
    case TYPE_BASE_FLOAT: return CG_SCALAR_FLOAT;
    case TYPE_BASE_INT: return CG_SCALAR_INT;
    case TYPE_BASE_BOOLEAN: return CG_SCALAR_BOOL;
    default: return CG_SCALAR_NONE;
    }
}

/*
 * InitType() - Initialize a type struct.
 *
 */

void InitType(Type *fType)
{
    char *c = (char *) fType;
    int ii;

    for (ii = 0; ii < sizeof(Type); ii++)
        *c++ = 0;
    fType->co.scalarKind = CG_SCALAR_NONE;
} // InitType

/*
 * NewType() - Allocate a new type struct.
 *
 */

Type *NewType(int properties, int size)
{
    Type *lType;

    lType = (Type *) malloc(sizeof(Type));
    InitType(lType);
    lType->properties = properties;
    lType->co.size = size;
    SetScalarKind(lType, lKindFromLegacyBase(properties & TYPE_BASE_MASK));
    return lType;
} // NewType

/*
 * DupType() - Duplicate a type struct.
 *
 */

Type *DupType(Type *fType)
{
    Type *lType;

    lType = (Type *) malloc(sizeof(Type));
    *lType = *fType;
    return lType;
} // DupType

/*
 * NewPackedArrayType() - Define a new packed (vector) array type.
 *
 */

Type *NewPackedArrayType(Type *elType, int numels, int properties)
{
    Type *lType;

    lType = NewType(TYPE_CATEGORY_ARRAY | TYPE_MISC_PACKED | properties | GetBase(elType), 0);
    SetScalarKind(lType, GetScalarKind(elType));
    lType->arr.eltype = elType;
    lType->arr.numels = numels;
    lType->arr.size = Cg->theHAL->GetSizeof(lType);
    return lType;
} // NewPakedArrayType

/*************************************** Category Functions **********************************/

/*
 * IsCategory() - See if a type is of the given category.
 *
 */

int IsCategory(const Type *fType, int category)
{
    if (fType && (fType->properties & TYPE_CATEGORY_MASK) == category) {
        return 1;
    } else {
        return 0;
    }
} // IsCategory

/*
 * IsTypeBase() - See if a type is of the given base.
 *
 */

int IsTypeBase(const Type *fType, int base)
{
    if (fType && (fType->properties & TYPE_BASE_MASK) == base) {
        return 1;
    } else {
        return 0;
    }
} // IsTypeBase

/*
 * IsVoid() - Returns TRUE if a void type.
 *
 */

int IsVoid(const Type *fType)
{
    if (fType && (fType->properties & TYPE_MISC_VOID)) {
        return 1;
    } else {
        return 0;
    }
} // IsVoid

/*
 * IsBoolean() - Returns TRUE if a Boolean type.
 *
 */

int IsBoolean(const Type *fType)
{
    if (fType && (GetScalarKind(fType) == CG_SCALAR_BOOL ||
                  (GetScalarKind(fType) == CG_SCALAR_NONE &&
                   IsTypeBase(fType, TYPE_BASE_BOOLEAN)))) {
        return 1;
    } else {
        return 0;
    }
} // IsBoolean

/*
 * IsScalar() - Returns TRUE if a scalar type.
 *
 */

int IsScalar(const Type *fType)
{
    if (fType && (fType->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_SCALAR) {
        return 1;
    } else {
        return 0;
    }
} // IsScalar

/*
 * IsArray() - Returns TRUE if a packed or unpacked array type.
 *
 */

int IsArray(const Type *fType)
{
    if (fType && (fType->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_ARRAY) {
        return 1;
    } else {
        return 0;
    }
} // IsScalar

/*
 * IsVector() - Returns TRUE if a vector type.
 *
 */

int IsVector(const Type *fType, int *len)
{
    if (fType &&
        (fType->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_ARRAY &&
        (fType->properties & TYPE_MISC_PACKED) &&
        !IsArray(fType->arr.eltype))
    {
        if (len)
            *len = fType->arr.numels;
        return 1;
    } else {
        return 0;
    }
} // IsVector

/*
 * IsMatrix() - Returns TRUE if a matrix type.
 *
 */

int IsMatrix(const Type *fType, int *len, int *len2)
{
    if (fType &&
        (fType->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_ARRAY &&
        (fType->properties & TYPE_MISC_PACKED) &&
        IsVector(fType->arr.eltype, len))
    {
        if (len2)
            *len2 = fType->arr.numels;
        return 1;
    } else {
        return 0;
    }
} // IsMatrix

/*
 * IsUnsizedArray() - Returns TRUE if an array with an unspecified number of
 *         elements.  Unsized arrays carry the CG_ARRAY_UNSIZED sentinel;
 *         zero is reserved for invalid/recovery types.
 *
 */

int IsUnsizedArray(const Type *fType)
{
    if (GetCategory(fType) == TYPE_CATEGORY_ARRAY &&
        fType->arr.numels == CG_ARRAY_UNSIZED)
    {
        return 1;
    } else {
        return 0;
    }
} // IsUnsizedArray

/*
 * IsStruct() - Returns TRUE if a struct.
 *
 */

int IsStruct(const Type *fType)
{
    if (fType &&
        GetCategory(fType) == TYPE_CATEGORY_STRUCT)
    {
        return 1;
    } else {
        return 0;
    }
} // IsStruct

/*
 * IsProgram() - See if a type is a program.
 *
 */

int IsProgram(const Type *fType)
{
    if (fType && (fType->properties & TYPE_MISC_PROGRAM)) {
        return 1;
    } else {
        return 0;
    }
} // IsProgram

/*
 * IsPacked()
 *
 */

int IsPacked(const Type *fType)
{
    if (fType && (fType->properties & TYPE_MISC_PACKED)) {
        return 1;
    } else {
        return 0;
    }
} // IsPacked

/*
 * IsSameUnqualifiedType() - Returns TRUE if the unqualified types aType and bType are the same.
 *
 */

int IsSameUnqualifiedType(const Type *aType, const Type *bType)
{
    if (aType == bType) {
        return 1;
    } else {
        // Scalar identity is canonical; kinds not modeled by the canonical
        // table still compare through their legacy base bits.
        if ((aType->properties & TYPE_CATEGORY_MASK) ==
                (bType->properties & TYPE_CATEGORY_MASK) &&
            GetScalarKind(aType) == GetScalarKind(bType) &&
            (GetScalarKind(aType) != CG_SCALAR_NONE ||
             GetBase(aType) == GetBase(bType))) {
            switch (aType->properties & TYPE_CATEGORY_MASK) {
            case TYPE_CATEGORY_SCALAR:
                return 1;
            case TYPE_CATEGORY_SAMPLER:
                // Sampler identity is the interned kind; qualifiers are
                // the only legal variation between two instances.
                return IsSampler(aType, NULL) && IsSampler(bType, NULL);
            case TYPE_CATEGORY_ARRAY:
                // Packedness is part of the type at every nesting layer,
                // and the numels comparison is sentinel-aware: two unsized
                // arrays of the same element type match, sized never
                // matches unsized.
                if (aType->arr.numels == bType->arr.numels &&
                    IsPacked(aType) == IsPacked(bType)) {
                    return IsSameUnqualifiedType(aType->arr.eltype, bType->arr.eltype);
                }
                break;
            case TYPE_CATEGORY_FUNCTION:
                break;
            case TYPE_CATEGORY_STRUCT:
                if (aType->str.unqualifiedtype == bType->str.unqualifiedtype)
                    return 1;
                break;
            case TYPE_CATEGORY_INTERFACE:
                // Interface identity is the declared type itself.
                return aType == bType;
            default:
                break;
            }
        }
    }
    return 0;
} // IsSameUnqualifiedType

/*
 * IsTypedef() - See if a symbol is a typedef.
 *
 */

int IsTypedef(const Symbol *fSymb)
{
    if (fSymb && fSymb->kind == TYPEDEF_S) {
        return 1;
    } else {
        return 0;
    }
} // IsTypedef

/*
 * IsFunction() - See if a symbol is a function.
 *
 */

int IsFunction(const Symbol *fSymb)
{
    if (fSymb && fSymb->kind == FUNCTION_S) {
        return 1;
    } else {
        return 0;
    }
} // IsFunction

/*
 * IsInline() - See if a symbol is an inline function.
 *
 */

int IsInline(const Symbol *fSymb)
{
    if (fSymb && fSymb->kind == FUNCTION_S && (fSymb->properties & SYMB_IS_INLINE_FUNCTION)) {
        return 1;
    } else {
        return 0;
    }
} // IsInline

/*
 * GetBase() - Return the base attributes of a type.
 *
 */

int GetBase(const Type *fType)
{
    if (fType) {
        return fType->properties & TYPE_BASE_MASK;
    } else {
        return TYPE_BASE_NO_TYPE;
    }
} // GetBase

/*
 * GetCategory() - Return the categpry of a type.
 *
 */

int GetCategory(const Type *fType)
{
    if (fType) {
        return fType->properties & TYPE_CATEGORY_MASK;
    } else {
        return TYPE_CATEGORY_NONE;
    }
} // GetCategory

/*
 * GetDomain() - Return the domain of a type.
 *
 */

int GetDomain(const Type *fType)
{
    if (fType) {
        return fType->properties & TYPE_DOMAIN_MASK;
    } else {
        return TYPE_DOMAIN_UNKNOWN;
    }
} // GetDomain

/*
 * GetQualifiers() - Return a type's qualifiers.
 *
 */

int GetQualifiers(const Type *fType)
{
    if (fType) {
        return fType->properties & TYPE_QUALIFIER_MASK;
    } else {
        return TYPE_QUALIFIER_NONE;
    }
} // GetQualifiers

/*
 * GetQuadRegSize() - Return the number of quad registers required to hold an object
 *         of this type.  Minimum size is 1.
 */

int GetQuadRegSize(const Type *fType)
{
    if (fType) {
        return (fType->co.size + 3) >> 2;
    } else {
        return 1;
    }
} // GetQuadRegSize

/*
 * ClearTypeMisc() - Clear bits in the properties field a type.
 *
 */

void ClearTypeMisc(Type *fType, int misc)
{
    if (fType)
        fType->properties &= ~misc;
} // ClearTypeMisc

/*********************************************************************************************/
/************************************ Symbol Semantic Functions ******************************/
/*********************************************************************************************/

/*
 * LookUpLocalSymbol()
 *
 */

Symbol *LookUpLocalSymbol(Scope *fScope, int atom)
{
    Symbol *lSymb;
    int rname, ratom;

    ratom = GetReversedAtom(atable, atom);
    if (!fScope)
        fScope = CurrentScope;
    lSymb = fScope->symbols;
    while (lSymb) {
        rname = GetReversedAtom(atable, lSymb->name);
        if (rname == ratom) {
            return lSymb;
        } else {
            if (rname > ratom) {
                lSymb = lSymb->left;
            } else {
                lSymb = lSymb->right;
            }
        }
    }
    return NULL;
} // LookUpLocalSymbol

/*
 * LookUpLocalSymbolBySemanticName() - Lookup a symbol in a local tree by the : semantic name.
 *
 * Note:  The tree is not ordered for this lookup so the next field is used.  This only works
 * for structs and other scopes that maintain this list.
 *
 * Note: There can be multiple matches.  Returns the first.
 *
 */

Symbol *LookUpLocalSymbolBySemanticName(Scope *fScope, int atom)
{
    Symbol *lSymb;

    if (!fScope)
        return NULL;
    lSymb = fScope->symbols;
    while (lSymb) {
        if (lSymb->kind == VARIABLE_S) {
            if (lSymb->details.var.semantics == atom)
                return lSymb;
        }
        lSymb = lSymb->next;
    }
    return NULL;
} // LookUpLocalSymbolBySemanticName

/*
 * LookUpLocalSymbolByBindingName() - Lookup a symbol in a local tree by the lname in the
 *         semantic binding structure.
 *
 * Note:  The tree is not ordered for this lookup so the next field is used.  This only works
 * for structs and other scopes that maintain this list.
 *
 * Note: There can be multiple matches.  Returns the first.
 *
 */

Symbol *LookUpLocalSymbolByBindingName(Scope *fScope, int atom)
{
    Symbol *lSymb;

    if (!fScope)
        return NULL;
    lSymb = fScope->symbols;
    while (lSymb) {
        if (lSymb->kind == VARIABLE_S && lSymb->details.var.bind) {
            if (lSymb->details.var.bind->none.lname == atom)
                return lSymb;
        }
        lSymb = lSymb->next;
    }
    return NULL;
} // LookUpLocalSymbolByBindingName

/*
 * LookUpLocalTag()
 *
 */

Symbol *LookUpLocalTag(Scope *fScope, int atom)
{
    Symbol *lSymb;
    int rname, ratom;

    ratom = GetReversedAtom(atable, atom);
    if (!fScope)
        fScope = CurrentScope;
    lSymb = fScope->tags;
    while (lSymb) {
        rname = GetReversedAtom(atable, lSymb->name);
        if (rname == ratom) {
            return lSymb;
        } else {
            if (rname > ratom) {
                lSymb = lSymb->left;
            } else {
                lSymb = lSymb->right;
            }
        }
    }
    return NULL;
} // LookUpLocalTag

/*
 * LookUpSymbol()
 *
 */

Symbol *LookUpSymbol(Scope *fScope, int atom)
{
    Symbol *lSymb;

    if (!fScope)
        fScope = CurrentScope;
    while (fScope) {
        lSymb = LookUpLocalSymbol(fScope, atom);
        if (lSymb)
            return lSymb;
        fScope = fScope->parent;
    }
    return NULL;
} // LookUpSymbol

/*
 * LookUpTag()
 *
 */

Symbol *LookUpTag(Scope *fScope, int atom)
{
    Symbol *lSymb;

    if (!fScope)
        fScope = CurrentScope;
    while (fScope) {
        lSymb = LookUpLocalTag(fScope, atom);
        if (lSymb)
            return lSymb;
        fScope = fScope->parent;
    }
    return NULL;
} // LookUpTag

/*
 * LookUpTypeSymbol()
 *
 */

Type *LookUpTypeSymbol(Scope *fScope, int atom)
{
    Symbol *lSymb;
    Type *lType;

    lSymb = LookUpSymbol(fScope, atom);
    if (lSymb) {
        if (!IsTypedef(lSymb)) {
            InternalError(Cg->tokenLoc, ERROR_S_NAME_NOT_A_TYPE,
                          GetAtomString(atable, atom));
            return UndefinedType;
        }
        lType = lSymb->type;
        if (lType) {
            return lType;
        } else {
            return UndefinedType;
        }
    } else {
        InternalError(Cg->tokenLoc, ERROR_S_TYPE_NAME_NOT_FOUND,
                      GetAtomString(atable, atom));
        return UndefinedType;
    }
} // LookUpTypeSymbol

/*
 * GetStandardType()
 *
 * Scalar: len = 0.
 * Vector: len >= 1 and len2 = 0
 * Matrix: len >= 1 and len2 >= 1
 *
 * len = 1 means "float f[1]" not "float f"
 * Vector and matrix types are PACKED.
 *
 */

Type *GetStandardType(int tbase, int tlen, int tlen2)
{
    Type *lType, *nType;

    if (tbase >= 0 && tbase <= TYPE_BASE_LAST_USER) {
        lType = baseTypes[tbase];
        if (tlen > 0) {
            // Put these in a table, too!!! XYZZY !!!
            nType = NewType(TYPE_CATEGORY_ARRAY | TYPE_MISC_PACKED | tbase, 0);
            nType->arr.eltype = lType;
            nType->arr.numels = tlen;
            nType->arr.size = Cg->theHAL->GetSizeof(nType);
            lType = nType;
            if (tlen2 > 0) {
                // Put these in a table, too!!! XYZZY !!!
                nType = NewType(TYPE_CATEGORY_ARRAY | TYPE_MISC_PACKED | tbase, 0);
                nType->arr.eltype = lType;
                nType->arr.numels = tlen2;
                nType->arr.size = Cg->theHAL->GetSizeof(nType);
                lType = nType;
            }
        }
    } else {
        lType = UndefinedType;
    }
    return lType;
} // GetStandardType

/*
 * GetElementType() - Return a pointer to the type of elements stored in this array.
 *
 */

Type *GetElementType(const Type *fType)
{
    Type *lType;

    if (GetCategory(fType) == TYPE_CATEGORY_ARRAY) {
        lType = fType->arr.eltype;
#if 0000
        if ((fType->properties & TYPE_QUALIFIER_CONST) &&
            !(lType->properties & TYPE_QUALIFIER_CONST))
        {
            lType = DupType(lType);
            lType->properties |= TYPE_QUALIFIER_CONST;
        }
#endif
    } else {
        InternalError(Cg->tokenLoc, ERROR___TYPE_NOT_ARRAY);
        lType = UndefinedType;
    }
    return lType;
} // GetElementType

/*
 * SetMemberOffsets() - Assign offsets to members for use by code generators.
 *
 */

void SetStructMemberOffsets(Type *fType)
{
    int addr, size, alignment;
    Symbol *lSymb;

    addr = 0;
    lSymb = fType->str.members->symbols;
    while (lSymb) {
        if (IsFunction(lSymb)) {
            /* Methods occupy no storage and carry no member offset. */
            lSymb = lSymb->next;
            continue;
        }
        alignment = Cg->theHAL->GetAlignment(lSymb->type);
        size = Cg->theHAL->GetSizeof(lSymb->type);
        addr = ((addr + alignment - 1)/alignment)*alignment;
        lSymb->details.var.addr = addr;
        addr += size;
        lSymb = lSymb->next;
    }
    fType->co.size = ((addr + 3)/4)*4;
} // SetStructMemberOffsets

/*
 * SetStructMembers() - Set the member tree of a structure.
 *
 */

Type *SetStructMembers(SourceLoc *loc, Type *fType, Scope *members)
{
    Symbol *lSymb;
    const char *tagname;

    if (fType) {
        if (fType->str.members) {
            SemanticError(loc, ERROR_SSD_STRUCT_ALREADY_DEFINED,
                            GetAtomString(atable, fType->str.tag),
                            GetAtomString(atable, fType->str.loc.file),
                            fType->str.loc.line);
        } else {
            if (fType->str.tag) {
                tagname = GetAtomString(atable, fType->str.tag);
            } else {
                tagname = "<no-name>";
            }
            fType->str.members = members;
            fType->str.loc = *loc;
            fType->str.HasSemantics = members->HasSemantics;
            SetStructMemberOffsets(fType);
            if (fType->str.tag) {
                lSymb = LookUpLocalSymbol(CurrentScope, fType->str.tag);
                if (!lSymb) {
                    lSymb = DefineTypedef(loc, CurrentScope, fType->str.tag, fType);
                } else {
                    if (IsCategory(fType, TYPE_CATEGORY_STRUCT)) {
                        if (!IsCategory(lSymb->type, TYPE_CATEGORY_STRUCT)) {
                            SemanticError(loc, ERROR_S_NAME_ALREADY_DEFINED, tagname);
                        }
                    }
                }
            }
        }
    }
    return fType;
} // SetStructMembers

/*
 * AddParameter() - Add a parameter to a function's formal parameter list, or a member to a
 *         struct or connector's member list.
 */

void AddParameter(Scope *fScope, Symbol *param)
{
    Symbol *lSymb = fScope->params;

    if (lSymb) {
        while (lSymb->next)
            lSymb = lSymb->next;
        lSymb->next = param;
    } else {
        fScope->params = param;
    }
} // AddParameter

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// Various Support Functions: /////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * GetSwizzleOrWriteMask() - Build a swizzle mask out of the letters in an identifier.
 *
 */
 
int GetSwizzleOrWriteMask(SourceLoc *loc, int atom, int *FIsLValue, int *flen)
{
    const char *s, *t;
    int len, bit, mask, bits;
    int groups, group;
    int LIsLValue;
    char ch;

    s = t = GetAtomString(atable, atom);
    len = mask = bits = groups = 0;
    LIsLValue = 1;
    while (*s) {
        ch = *s++;
        switch (ch) {
        case 'x':
            bit = 0;
            group = 1;
            break;
        case 'y':
            bit = 1;
            group = 1;
            break;
        case 'z':
            bit = 2;
            group = 1;
            break;
        case 'w':
            bit = 3;
            group = 1;
            break;
        case 'r':
            bit = 0;
            group = 2;
            break;
        case 'g':
            bit = 1;
            group = 2;
            break;
        case 'b':
            bit = 2;
            group = 2;
            break;
        case 'a':
            bit = 3;
            group = 2;
            break;
        default:
            SemanticError(loc, ERROR_CS_INVALID_SWIZZLE_CHAR, ch, t);
            return mask;
            break;
        }
        mask |= bit << len*2;
        bit = 1 << bit;
        if (bits & bit)
            LIsLValue = 0;
        bits |= bit;
        if (groups && groups != group) {
            SemanticError(loc, ERROR_CS_INVALID_SWIZZLE_CHAR, ch, t);
            return mask;
        }
        groups |= group;
        len++;
    }
    if (len > 4)
        SemanticError(loc, ERROR_S_SWIZZLE_TOO_LONG, t);
    if (FIsLValue)
        *FIsLValue = LIsLValue;
    if (flen)
        *flen = len;
    return mask;
} // GetSwizzleOrWriteMask

/*
 * GetMatrixSwizzleOrWriteMask() - Build a matrix swizzle mask out of the letters in an identifier.
 *
 */
 
int GetMatrixSwizzleOrWriteMask(SourceLoc *loc, int atom, int *FIsLValue, int *flen)
{
    const char *s, *t;
    int len, bit, mask, bits, base;
    int LIsLValue, Error;
    char lch, ch;

    s = t = GetAtomString(atable, atom);
    len = mask = bits = 0;
    LIsLValue = 1;
    if (s[0] == '_' && s[1] != '\0') {
        Error = 0;
        if (s[1] == 'm') {
            base = 0;
        } else {
            base = 1;
        }
        while (*s) {
            ch = lch = *s++;
            if (ch == '_') {
                if (base == 0) {
                    if (*s++ != 'm') {
                        Error = 1;
                        break;
                    }
                }
                lch = *s++;
                ch = lch - base;
                if (ch >= '0' && ch <= '3') {
                    bit = (ch - '0') << 2;
                    lch = *s++;
                    ch = lch - base;
                    if (ch >= '0' && ch <= '3') {
                        bit = bit | (ch - '0');
                        mask |= bit << len*4;
                        bit = 1 << bit;
                        if (bit & bits)
                            LIsLValue = 0;
                        bits |= bit;
                        len++;
                    } else {
                        Error = 1;
                        break;
                    }
                } else {
                    Error = 1;
                    break;
                }
            } else {
                Error = 1;
                break;
            }
        }
    } else {
        lch = *s;
        Error = 1;
    }
    if (Error) {
        SemanticError(loc, ERROR_CS_INVALID_SWIZZLE_CHAR, lch, t);
    }
    if (len > 4)
        SemanticError(loc, ERROR_S_SWIZZLE_TOO_LONG, t);
    if (FIsLValue)
        *FIsLValue = LIsLValue;
    if (flen)
        *flen = len;
    return mask;
} // GetMatrixSwizzleOrWriteMask

/*
 * GetBaseTypeNameString() - Return a pointer to a string representation of a base type name.
 *
 */

const char *GetBaseTypeNameString(int base)
{
    if (base >= 0 && base <= TYPE_BASE_LAST_USER) {
        return GetAtomString(atable, baseTypeNames[base]);
    } else {
        return "*** bad base value ***";
    }
} // GetBaseTypeNameString

/*
 * ClearSymbolTempptr() - Clear the tempptr for all symbols in this tree.
 *
 */

static void ClearSymbolTempptr(Symbol *fSymb)
{
    if (fSymb) {
        fSymb->tempptr = NULL;
        ClearSymbolTempptr(fSymb->left);
        ClearSymbolTempptr(fSymb->right);
    }
} // ClearSymbolTempptr

/*
 * ClearSymbolTempptrList() - Walk a list of scopes and
 *                            clear tempptr field for all symbols.
 *
 */

static void ClearSymbolTempptrList(Scope *fScope)
{
    while (fScope) {
        ClearSymbolTempptr(fScope->symbols);
        fScope = fScope->next;
    }
} // ClearSymbolTempptrList

/*
 * ClearAllSymbolTempptr
 *
 */

void ClearAllSymbolTempptr(void)
{
    ClearSymbolTempptrList(ScopeList);
} // ClearSymbolTempptr


/*
 * ClearSymbolTempptr2() - Clear the tempptr2 for all symbols in this tree.
 *
 */

static void ClearSymbolTempptr2(Symbol *fSymb)
{
    if (fSymb) {
        fSymb->tempptr2 = NULL;
        ClearSymbolTempptr2(fSymb->left);
        ClearSymbolTempptr2(fSymb->right);
    }
} // ClearSymbolTempptr2

/*
 * ClearSymbolTempptr2List() - Walk a list of scopes and
 *                            clear tempptr field for all symbols.
 *
 */

static void ClearSymbolTempptr2List(Scope *fScope)
{
    while (fScope) {
        ClearSymbolTempptr2(fScope->symbols);
        fScope = fScope->next;
    }
} // ClearSymbolTempptr2List

/*
 * ClearAllSymbolTempptr2
 *
 */

void ClearAllSymbolTempptr2(void)
{
    ClearSymbolTempptr2List(ScopeList);
} // ClearSymbolTempptr2

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// End of symbols.c //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
