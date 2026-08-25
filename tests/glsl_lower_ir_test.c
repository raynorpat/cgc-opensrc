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
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
\****************************************************************************/
// glsl_lower_ir_test.c
//
// Unit seam for the Cg IR -> GLSL 1.10 lowering (GlslLowerCgIR).
// Shader-level fixtures cannot produce two sibling stores sharing one
// base expression node -- every source statement lowers its own nodes
// -- so the matrix group-write recognizer branches below are driven by
// direct CgIRModule construction:
//   - distinct per-store values whose constant coordinates differ from
//     their left coordinates fall back to independent statements that
//     print their own coordinates (never mirrored);
//   - an intact producer group write still collapses to cg_set_matN;
//   - a producer-marked run (selectorRead on the store targets) hit by
//     any fifth consecutive matching-shape store fails loudly instead
//     of falling back elementwise;
//   - an over-long run of unmarked user stores still falls back and
//     lowers elementwise;
//   - a whole-vector value reaching the plain assignment path fails
//     loudly instead of emitting into one scalar component target.
//

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "cg_ir.h"
#include "glsl_ir.h"
#include "glsl_hal.h"

static CgStruct testCg;
CgStruct *Cg = &testCg;
Scope *CurrentScope;

/*
 * Stub HAL name registration for InitSymbolTable.
 */

static int TestRegisterNames(slHAL *fHAL)
{
    (void) fHAL;
    return 1;
} // TestRegisterNames

/*
 * Minimal size query mirroring GetSizeof_HAL for InitSymbolTable.
 */

static int TestGetSizeof(Type *fType)
{
    if (!fType)
        return 0;
    return fType->co.size;
} // TestGetSizeof

void SemanticError(SourceLoc *loc, int num, const char *mess, ...)
{
    (void) loc;
    (void) num;
    (void) mess;
}

void InternalError(SourceLoc *loc, int num, const char *mess, ...)
{
    (void) loc;
    (void) num;
    (void) mess;
}

dtype CurrentDeclTypeSpecs = { 0, };

Symbol *DefineTypedef(SourceLoc *loc, Scope *fScope, int atom, Type *fType)
{
    (void) loc;
    (void) fScope;
    (void) atom;
    (void) fType;
    return NULL;
}

/*
 * compile.c/link seam stubs: the HAL translation paths reference these
 * through GenerateCode_glsl and profile registration, which this unit
 * never drives.
 */

int GetErrorCount(void)
{
    return 0;
}

void ReportProfileCallPath(const void *failingSymbol)
{
    (void) failingSymbol;
}

slProfile *RegisterProfile(int (*InitHAL)(slHAL *), const char *name, int id)
{
    (void) InitHAL;
    (void) name;
    (void) id;
    return NULL;
}

void SetProfileIdentity(const char *name, CgProfileStage stage,
                        const char *wildcardName, int wildcardSpecificity)
{
    (void) name;
    (void) stage;
    (void) wildcardName;
    (void) wildcardSpecificity;
}

ConnectorDescriptor *LookupConnectorHAL(ConnectorDescriptor *connectors,
                                        int cid, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (connectors[i].cid == cid)
            return &connectors[i];
    }
    return NULL;
}

void SetSymbolConnectorBindingHAL(Binding *binding,
                                  ConnectorRegisters *connector)
{
    binding->conn.properties = BIND_IS_BOUND;
    binding->conn.kind = BK_CONNECTOR;
    if (connector->properties & REG_WRITE_REQUIRED)
        binding->conn.properties |= BIND_WRITE_REQUIRED;
    binding->conn.base = connector->base;
    binding->conn.size = connector->size;
    binding->conn.rname = connector->name;
    binding->conn.regno = connector->regno;
}

int HasNumericSuffix(const char *text, char *root, int size, int *suffix)
{
    int value, hasSuffix, len, scale;
    char *s, ch;

    strncpy(root, text, size - 1);
    len = strlen(text);
    if (len >= size)
        len = size - 1;
    root[len] = 0;
    value = 0;
    hasSuffix = 0;
    scale = 1;
    s = &root[len];
    while (1) {
        ch = *--s;
        if (ch >= '0' && ch <= '9' && s >= root) {
            value = value + scale * (ch - '0');
            scale *= 10;
            hasSuffix = 1;
        }
        else {
            break;
        }
    }
    s[1] = '\0';
    *suffix = value;
    return hasSuffix;
}

///////////////////////////////// Allocators //////////////////////////////////

static void *TestAlloc(void *arg, size_t size)
{
    (void) arg;
    return calloc(1, size);
} // TestAlloc

//////////////////////////////////// State ////////////////////////////////////

static slHAL testHal;
static Type *floatType;
static Type *float4Type;
static Type *mat4Type;
static SourceLoc nodeLoc;

#define TEXT_BUFFER_SIZE 16384
static char textBuffer[TEXT_BUFFER_SIZE];

/////////////////////////////// Module fixtures ///////////////////////////////

/*
 * lMakeSymbol() - Minimal standalone symbol carrying a name atom, a
 *          kind, and a canonical type.
 */

static Symbol *lMakeSymbol(symbolkind kind, const char *name, Type *fType)
{
    Symbol *lSymb;

    lSymb = (Symbol *) calloc(1, sizeof(Symbol));
    assert(lSymb != NULL);
    lSymb->name = LookUpAddString(atable, name);
    lSymb->kind = kind;
    lSymb->type = fType;
    return lSymb;
} // lMakeSymbol

/*
 * lInitModule() - Fresh IR module owned by the zeroed allocator.
 */

static void lInitModule(CgIRModule *module)
{
    memset(module, 0, sizeof(*module));
    CgIRInitModule(module, TestAlloc, NULL);
} // lInitModule

/*
 * lAddEntry() - void main(void) entry whose body is one block holding
 *          "list".
 */

static CgIRFunction *lAddEntry(CgIRModule *module, CgIRStmt *list)
{
    Symbol *symbol;
    CgIRFunction *function;
    CgIRStmt *block;

    symbol = lMakeSymbol(FUNCTION_S, "main", VoidType);
    function = CgIRNewFunction(module, symbol, VoidType, &nodeLoc);
    assert(function != NULL);
    function->isEntry = 1;
    module->entry = function;
    CgIRAppendFunction(&module->functions, function);
    block = CgIRNewBlockStmt(module, &nodeLoc);
    assert(block != NULL);
    block->u.block = list;
    function->body = block;
    return function;
} // lAddEntry

/*
 * lAddLocalDecl() - One leading local declaration statement; returns
 *          the declared symbol so references share its identity.
 */

static Symbol *lAddLocalDecl(CgIRModule *module, CgIRStmt **list,
                             const char *name, Type *fType)
{
    Symbol *symbol;
    CgIRDecl *decl;
    CgIRStmt *stmt;

    symbol = lMakeSymbol(VARIABLE_S, name, fType);
    decl = CgIRNewDecl(module, symbol, symbol->name, fType,
                       CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE, 0, NULL,
                       &nodeLoc);
    assert(decl != NULL);
    stmt = CgIRNewDeclStmt(module, &nodeLoc, decl);
    assert(stmt != NULL);
    CgIRAppendStmt(list, stmt);
    return symbol;
} // lAddLocalDecl

/*
 * lIntConstant() - Constant-index helper for element chains.
 */

static CgIRExpr *lIntConstant(CgIRModule *module, int value)
{
    CgNumericValue number;

    memset(&number, 0, sizeof(number));
    number.kind = CG_SCALAR_INT;
    number.value.i = value;
    return CgIRNewConstant(module, IntType, &nodeLoc, &number);
} // lIntConstant

/*
 * lFloatValue() - Scalar float constant store value.
 */

static CgIRExpr *lFloatValue(CgIRModule *module, float literal)
{
    CgNumericValue number;

    memset(&number, 0, sizeof(number));
    number.kind = CG_SCALAR_FLOAT;
    number.value.f = literal;
    return CgIRNewConstant(module, floatType, &nodeLoc, &number);
} // lFloatValue

/*
 * lAppendStore() - One scalar matrix-element store statement:
 *          ASSIGN(INDEX(INDEX(base,row),column), value).  "base" is
 *          shared by pointer with sibling stores exactly like producer
 *          fan-out; the chain nodes themselves are fresh per store.
 *          "marked" mirrors the Task 16 selector marker on synthesized
 *          store targets (selectorRead in cg_ir.h).
 */

static void lAppendStore(CgIRModule *module, CgIRStmt **list,
                         CgIRExpr *base, int row, int column,
                         CgIRExpr *value, int marked)
{
    CgIRExpr *rowIndex;
    CgIRExpr *rowVector;
    CgIRExpr *columnIndex;
    CgIRExpr *target;
    CgIRExpr *assign;
    CgIRStmt *stmt;

    rowIndex = lIntConstant(module, row);
    assert(rowIndex != NULL);
    rowVector = CgIRNewIndex(module, float4Type, &nodeLoc, base,
                             rowIndex);
    assert(rowVector != NULL);
    columnIndex = lIntConstant(module, column);
    assert(columnIndex != NULL);
    target = CgIRNewIndex(module, floatType, &nodeLoc, rowVector,
                          columnIndex);
    assert(target != NULL);
    target->isLvalue = 1;
    if (marked)
        target->selectorRead = 1;
    assign = CgIRNewAssign(module, floatType, &nodeLoc, CGIR_OP_ASSIGN,
                           target, value);
    assert(assign != NULL);
    stmt = CgIRNewExprStmt(module, &nodeLoc, assign);
    assert(stmt != NULL);
    CgIRAppendStmt(list, stmt);
} // lAppendStore

/*
 * lValueElement() - One distinct constant selection value:
 *          INDEX(INDEX(base,row),column) over a fresh chain.
 */

static CgIRExpr *lValueElement(CgIRModule *module, CgIRExpr *base,
                               int row, int column)
{
    CgIRExpr *rowVector;

    rowVector = CgIRNewIndex(module, float4Type, &nodeLoc, base,
                             lIntConstant(module, row));
    assert(rowVector != NULL);
    return CgIRNewIndex(module, floatType, &nodeLoc, rowVector,
                        lIntConstant(module, column));
} // lValueElement

/*
 * lLowerText() - Lower "module" through the real GLSL lowering; on
 *          success write it and return the emitted text.  Returns NULL
 *          after a recorded lowering failure.
 */

static const char *lLowerText(CgIRModule *source, GlslModule *out,
                              const GlslProfileDesc *profile)
{
    FILE *writer;
    long size;
    int OK;

    memset(out, 0, sizeof(*out));
    GlslInitModule(out, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    OK = GlslLowerCgIR(out, profile, source);
    if (!OK)
        return NULL;
    writer = tmpfile();
    assert(writer != NULL);
    assert(GlslWriteModule(writer, out));
    size = ftell(writer);
    assert(size > 0 && size < (long) TEXT_BUFFER_SIZE);
    rewind(writer);
    assert(fread(textBuffer, 1, (size_t) size, writer) == (size_t) size);
    textBuffer[size] = '\0';
    assert(!fclose(writer));
    return textBuffer;
} // lLowerText

///////////////////////////////// Assertions //////////////////////////////////

/*
 * Scenario 1: two stores share one base node with distinct values
 * whose coordinates differ from their left coordinates.  The run is a
 * plain user pair, not a fan-out: it must lower independently with
 * each value's own coordinates, never mirrored onto the left ones.
 */

static void lScenarioMixedCoordinates(const GlslProfileDesc *profile)
{
    CgIRModule ir;
    GlslModule out;
    CgIRFunction *entry;
    CgIRStmt *list = NULL;
    CgIRExpr *leftBase;
    CgIRExpr *rightBase;
    Symbol *gm;
    Symbol *gs;
    const char *text;

    lInitModule(&ir);
    gm = lAddLocalDecl(&ir, &list, "gm", mat4Type);
    gs = lAddLocalDecl(&ir, &list, "gs", mat4Type);
    leftBase = CgIRNewSymbol(&ir, mat4Type, &nodeLoc, gm);
    rightBase = CgIRNewSymbol(&ir, mat4Type, &nodeLoc, gs);

    /* gm[0][0] = gs[0][1]; -- value column 1 differs from left 0. */
    lAppendStore(&ir, &list, leftBase, 0, 0,
                 lValueElement(&ir, rightBase, 0, 1), 0);

    /* gm[1][1] = gs[1][0]; -- fresh chain, again mismatched. */
    lAppendStore(&ir, &list, leftBase, 1, 1,
                 lValueElement(&ir, rightBase, 1, 0), 0);

    entry = lAddEntry(&ir, list);
    assert(entry != NULL);

    text = lLowerText(&ir, &out, profile);
    assert(text != NULL);
    assert(strstr(text, "gm[0][0] = gs[0][1];") != NULL);
    assert(strstr(text, "gm[1][1] = gs[1][0];") != NULL);
    /* The mixed coordinates must not be mirrored onto the values. */
    assert(strstr(text, "gs[1][1]") == NULL);
    assert(strstr(text, "gs[0][0]") == NULL);
    assert(out.errors == 0);
    printf("glsl-lower-ir: mixed-coordinate fallback\n");
} // lScenarioMixedCoordinates

/*
 * Scenario 2: an intact producer group write (four marked stores
 * sharing one base node and one whole-vector value) still collapses
 * into the single cg_set_matN helper call.
 */

static void lScenarioProducerGroupWrite(const GlslProfileDesc *profile)
{
    CgIRModule ir;
    GlslModule out;
    CgIRFunction *entry;
    CgIRStmt *list = NULL;
    CgIRExpr *base;
    CgIRExpr *value;
    Symbol *pm;
    Symbol *pv;
    static const int coordinates[4][2] = {
        { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }
    };
    const char *text;
    int i;

    lInitModule(&ir);
    pm = lAddLocalDecl(&ir, &list, "pm", mat4Type);
    pv = lAddLocalDecl(&ir, &list, "pv", float4Type);
    base = CgIRNewSymbol(&ir, mat4Type, &nodeLoc, pm);
    value = CgIRNewSymbol(&ir, float4Type, &nodeLoc, pv);
    for (i = 0; i < 4; i++)
        lAppendStore(&ir, &list, base, coordinates[i][0],
                     coordinates[i][1], value, 1);
    entry = lAddEntry(&ir, list);
    assert(entry != NULL);

    text = lLowerText(&ir, &out, profile);
    assert(text != NULL);
    assert(strstr(text, "cg_set_mat4") != NULL);
    assert(out.errors == 0);
    printf("glsl-lower-ir: producer group write collapses\n");
} // lScenarioProducerGroupWrite

/*
 * Scenario 3: a producer-marked four-store run followed by ANY fifth
 * consecutive matching-shape element store cannot fall back
 * elementwise (each whole-vector value would print into one scalar
 * target), so the cap keeps the loud failure.
 */

static void lScenarioProducerCapStaysLoud(const GlslProfileDesc *profile)
{
    CgIRModule ir;
    GlslModule out;
    CgIRFunction *entry;
    CgIRStmt *list = NULL;
    CgIRExpr *base;
    CgIRExpr *value;
    Symbol *cm;
    Symbol *cv;
    Symbol *cs;
    static const int coordinates[4][2] = {
        { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }
    };
    int i;

    lInitModule(&ir);
    cm = lAddLocalDecl(&ir, &list, "cm", mat4Type);
    cv = lAddLocalDecl(&ir, &list, "cv", float4Type);
    cs = lAddLocalDecl(&ir, &list, "cs", floatType);
    base = CgIRNewSymbol(&ir, mat4Type, &nodeLoc, cm);
    value = CgIRNewSymbol(&ir, float4Type, &nodeLoc, cv);
    for (i = 0; i < 4; i++)
        lAppendStore(&ir, &list, base, coordinates[i][0],
                     coordinates[i][1], value, 1);
    /* Fifth consecutive matching-shape store: cm[0][1] = cs;. */
    lAppendStore(&ir, &list, base, 0, 1,
                 CgIRNewSymbol(&ir, floatType, &nodeLoc, cs), 0);
    entry = lAddEntry(&ir, list);
    assert(entry != NULL);

    memset(&out, 0, sizeof(out));
    GlslInitModule(&out, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!GlslLowerCgIR(&out, profile, &ir));
    assert(out.errors > 0);
    printf("glsl-lower-ir: producer cap stays loud\n");
} // lScenarioProducerCapStaysLoud

/*
 * Scenario 4: five consecutive unmarked user stores over one base
 * node pass the cap and fall back to independent elementwise
 * statements.
 */

static void lScenarioUserFillFallsBack(const GlslProfileDesc *profile)
{
    CgIRModule ir;
    GlslModule out;
    CgIRFunction *entry;
    CgIRStmt *list = NULL;
    CgIRExpr *base;
    Symbol *uf;
    static const int rows[5] = { 0, 1, 2, 3, 0 };
    static const int columns[5] = { 0, 0, 0, 0, 1 };
    const char *text;
    int i;

    lInitModule(&ir);
    uf = lAddLocalDecl(&ir, &list, "uf", mat4Type);
    base = CgIRNewSymbol(&ir, mat4Type, &nodeLoc, uf);
    for (i = 0; i < 5; i++)
        lAppendStore(&ir, &list, base, rows[i], columns[i],
                     lFloatValue(&ir, (float) (i + 1)), 0);
    entry = lAddEntry(&ir, list);
    assert(entry != NULL);

    text = lLowerText(&ir, &out, profile);
    assert(text != NULL);
    assert(strstr(text, "uf[0][0] = 1.0;") != NULL);
    assert(strstr(text, "uf[1][0] = 2.0;") != NULL);
    assert(strstr(text, "uf[2][0] = 3.0;") != NULL);
    assert(strstr(text, "uf[3][0] = 4.0;") != NULL);
    assert(strstr(text, "uf[0][1] = 5.0;") != NULL);
    assert(out.errors == 0);
    printf("glsl-lower-ir: user fill falls back elementwise\n");
} // lScenarioUserFillFallsBack

/*
 * Scenario 5: one unmarked store whose value is a whole vector cannot
 * reach the group recognizer (a single store never forms a run), so
 * the plain assignment path must fail loudly rather than print a
 * vector into one scalar component target.
 */

static void lScenarioVectorIntoScalarFailsLoud(
    const GlslProfileDesc *profile)
{
    CgIRModule ir;
    GlslModule out;
    CgIRFunction *entry;
    CgIRStmt *list = NULL;
    CgIRExpr *base;
    CgIRExpr *value;
    Symbol *gf;
    Symbol *gw;

    lInitModule(&ir);
    gf = lAddLocalDecl(&ir, &list, "gf", mat4Type);
    gw = lAddLocalDecl(&ir, &list, "gw", float4Type);
    base = CgIRNewSymbol(&ir, mat4Type, &nodeLoc, gf);
    value = CgIRNewSymbol(&ir, float4Type, &nodeLoc, gw);
    lAppendStore(&ir, &list, base, 0, 0, value, 0);
    entry = lAddEntry(&ir, list);
    assert(entry != NULL);

    memset(&out, 0, sizeof(out));
    GlslInitModule(&out, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!GlslLowerCgIR(&out, profile, &ir));
    assert(out.errors > 0);
    assert(out.errorReason != NULL);
    assert(!strcmp(out.errorReason, "GLSL 1.10 expression"));
    printf("glsl-lower-ir: vector-into-scalar fails loudly\n");
} // lScenarioVectorIntoScalarFailsLoud

////////////////////////////////////// main ///////////////////////////////////

int main(int argc, char **argv)
{
    const GlslProfileDesc *profile;
    int result;

    if (argc == 2 && !strcmp(argv[1], "--verify-assertions-active")) {
        int assertionsActive;

        assertionsActive = 0;
        assert((assertionsActive = 1) != 0);
        if (!assertionsActive)
            return 2;
        /* Token must match tests/check_assertions_active.cmake, which
         * is shared with glsl_ir_unit. */
        puts("glsl-ir-assertions-active");
        return 0;
    }

    memset(&testHal, 0, sizeof(testHal));
    testHal.GetSizeof = TestGetSizeof;
    testHal.RegisterNames = TestRegisterNames;
    Cg->theHAL = &testHal;

    assert(InitAtomTable(atable, 0));
    assert(InitSymbolTable(Cg));
    assert(StartGlobalScope(Cg));

    floatType = GetStandardTypeKind(CG_SCALAR_FLOAT, 0, 0);
    float4Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 0);
    mat4Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 4);
    assert(floatType != UndefinedType);
    assert(float4Type != UndefinedType);
    assert(mat4Type != UndefinedType);

    nodeLoc.file = 1;
    nodeLoc.line = 10;

    result = InitHAL_glslv(&testHal);
    assert(result);
    profile = (const GlslProfileDesc *) testHal.localData;
    assert(profile != NULL);
    assert(profile->stage == GLSL_STAGE_VERTEX);

    lScenarioMixedCoordinates(profile);
    lScenarioProducerGroupWrite(profile);
    lScenarioProducerCapStaysLoud(profile);
    lScenarioUserFillFallsBack(profile);
    lScenarioVectorIntoScalarFailsLoud(profile);

    return 0;
}
