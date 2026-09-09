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
// semantic.c
//

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"

/*
 * GetVectorConst()
 *
 */

static void GetVectorConst(float *fVal, expr *fExpr)
{
    int Oops = 0;
    constant *pconst;
    binary *pbin;
    unary *pun;
    int ii;

    for (ii = 0; ii < 4; ii++)
        fVal[ii] = 0.0f;
    if (fExpr) {
        switch (fExpr->common.kind) {
        case UNARY_N:
            pun = (unary *) fExpr;
            switch (pun->op) {
            //case FCONST_V_OP:
            //    for (ii = 0; ii < SUBOP_GET_S1(pun->subop); ii++)
            //        fVal[ii] = 1.1f;
            //    break;
            case VECTOR_V_OP:
                Oops = 2;
                break;
            default:
                Oops = 1;
                break;
            }
            break;
        case BINARY_N:
            pbin = (binary *) fExpr;
            switch (pbin->op) {
            case EXPR_LIST_OP:
                if (pbin->right == NULL &&
                    pbin->left->common.kind == CONST_N)
                {
                    pconst = (constant *) pbin->left;
                    switch (pconst->op) {
                    case FCONST_V_OP:
                    case HCONST_V_OP:
                    case XCONST_V_OP:
                        for (ii = 0; ii < SUBOP_GET_S1(pconst->subop); ii++)
                            fVal[ii] = (float) pconst->val[ii].value.f;
                        break;
                    case ICONST_V_OP:
                    case BCONST_V_OP:
                        for (ii = 0; ii < SUBOP_GET_S1(pconst->subop); ii++)
                            fVal[ii] = (float) pconst->val[ii].value.i;
                        break;
                    case FCONST_OP:
                    case HCONST_OP:
                    case XCONST_OP:
                        fVal[0] = (float) pconst->val[0].value.f;
                        break;
                    case ICONST_OP:
                    case BCONST_OP:
                        fVal[0] = (float) pconst->val[0].value.i;
                        break;
                    default:
                        Oops = 4;
                        break;
                    }
                } else {
                    Oops = 3;
                }
                break;
            default:
                Oops = 1;
                break;
            }
            break;
        default:
            Oops = 1;
            break;
        }
    }
    if (Oops)
        SemanticWarning(Cg->tokenLoc, 9999, "*** GetVectorConst() not finished ***");
} // GetVectorConst

/*
 * lNewUniformSemantic() - Add an entry to the uniform semantic table.
 *
 */
static void lNewUniformSemantic(int gname, Symbol *fSymb, int semantics)
{
    UniformSemantic *lUniform, *nUniform;

    lUniform = NewUniformSemantic(gname, fSymb->name, semantics);
    nUniform = Cg->theHAL->uniforms;
    if (nUniform) {
        while (nUniform->next)
            nUniform = nUniform->next;
        nUniform->next = lUniform;
    } else {
        Cg->theHAL->uniforms = lUniform;
    }
} // lNewUniformSemantic

/*
 * CanonicalSemanticAtom() - Return the case-canonical atom for a binding
 *         semantic name.  Semantic identity is compared through this atom
 *         so that e.g. "COLOR0" and "color0" denote one connector slot,
 *         while diagnostics and normalized output retain the source
 *         spelling of whichever declaration is being reported.
 */

static int CanonicalSemanticAtom(int semantic)
{
    const char *source = GetAtomString(atable, semantic);
    char text[MAX_SYMBOL_NAME_LEN + 1];
    int index;

    if (!source)
        return 0;
    for (index = 0; source[index] && index < MAX_SYMBOL_NAME_LEN; ++index)
        text[index] = (char) toupper((unsigned char) source[index]);
    text[index] = '\0';
    return LookUpAddString(atable, text);
} // CanonicalSemanticAtom

/*
 * Program outputs join one per-compilation map keyed by the canonical
 * semantic atom.  The first declaration seen is recorded; a later
 * declaration with the same canonical atom aliases an output connector
 * and is rejected by lCheckOutputSemantic().
 *
 */

typedef struct OutputSemanticRec {
    int canonical;
    Symbol *symb;
    struct OutputSemanticRec *next;
} OutputSemantic;

static OutputSemantic *lOutputSemantics;

/*
 * lCheckOutputSemantic() - Record "fSymb" under its output semantic, or
 *         reject it when the canonical atom was already bound: reports
 *         ERROR_SSSD_PROGRAM_OUTPUT_ALIAS at the second declaration with
 *         the first declaration's location as the note.  Returns 1 when
 *         the binding conflicts.
 */

static int lCheckOutputSemantic(Symbol *fSymb, int semantics)
{
    OutputSemantic *lOutput;
    Symbol *lFirst;
    int canonical;

    if (!semantics)
        return 0;
    canonical = CanonicalSemanticAtom(semantics);
    for (lFirst = NULL, lOutput = lOutputSemantics; lOutput;
         lOutput = lOutput->next)
    {
        if (lOutput->canonical == canonical) {
            lFirst = lOutput->symb;
            break;
        }
    }
    if (lFirst) {
        SemanticError(&fSymb->loc, ERROR_SSSD_PROGRAM_OUTPUT_ALIAS,
                      GetAtomString(atable, semantics),
                      GetAtomString(atable, lFirst->name),
                      GetAtomString(atable, lFirst->loc.file),
                      lFirst->loc.line);
        return 1;
    }
    lOutput = (OutputSemantic *) malloc(sizeof(OutputSemantic));
    lOutput->canonical = canonical;
    lOutput->symb = fSymb;
    lOutput->next = lOutputSemantics;
    lOutputSemantics = lOutput;
    return 0;
} // lCheckOutputSemantic

/*
 * EffectiveProgramDomain() - The program domain a symbol participates in:
 *         an explicit domain qualifier wins; otherwise a top-level entry
 *         parameter defaults to varying and everything else (globals and
 *         helper parameters, whose domain qualifiers and binding semantics
 *         the language ignores) defaults to uniform.  Function parameter
 *         directions (in/out/inout) are orthogonal and unaffected.
 */

static int EffectiveProgramDomain(const Symbol *symbol, int isTopLevelParam)
{
    int domain = GetDomain(symbol->type);

    if (domain != TYPE_DOMAIN_UNKNOWN)
        return domain;
    return isTopLevelParam ? TYPE_DOMAIN_VARYING : TYPE_DOMAIN_UNIFORM;
} // EffectiveProgramDomain

/*
 * lBindUniformVariable() - Bind a uniform variable to $uniform connector.  Record semantics
 *         value if present.
 */

static int lBindUniformVariable(Symbol *fSymb, int gname, int IsParameter)
{
    int category, domain, qualifiers, OK, errorsBefore;
    BindingTree *ltree;
    Binding *lBind;
    SymbolList **lList, *mList, *nList;

    OK = 0;
    category = GetCategory(fSymb->type);
    domain = GetDomain(fSymb->type);
    qualifiers = GetQualifiers(fSymb->type);
    switch (category) {
    case TYPE_CATEGORY_SCALAR:
    case TYPE_CATEGORY_ARRAY:
    case TYPE_CATEGORY_STRUCT:
    case TYPE_CATEGORY_SAMPLER:
        lNewUniformSemantic(gname, fSymb, fSymb->details.var.semantics);
        if (IsParameter) {
            lList = &Cg->theHAL->uniformParam;
        } else {
            lList = &Cg->theHAL->uniformGlobal;
        }
        lBind = /* theHAL-> */ NewBinding(gname, fSymb->name);
        lBind->none.properties = BIND_INPUT | BIND_UNIFORM;
        ltree = LookupBinding(gname, fSymb->name);
        if (ltree) {
            errorsBefore = GetErrorCount();
            if (Cg->theHAL->BindUniformPragma(&fSymb->loc, fSymb, lBind, &ltree->binding)) {
                if (!(lBind->none.properties & BIND_UNIFORM)) {
                    SemanticError(&fSymb->loc, ERROR_S_NON_UNIF_BIND_TO_UNIF_VAR,
                                  GetAtomString(atable, fSymb->name));
                    lList = NULL;
                }
            } else if (GetErrorCount() == errorsBefore) {
                SemanticError(&fSymb->loc, ERROR_S_INCOMPATIBLE_BIND_DIRECTIVE,
                              GetAtomString(atable, fSymb->name));
                lList = NULL;
            }
        }
        if (lList && !(lBind->none.properties & BIND_HIDDEN)) {
            OK = 1;
            fSymb->details.var.bind = lBind;
            nList = (SymbolList*) malloc(sizeof(SymbolList));
            nList->next = NULL;
            nList->symb = fSymb;
            if (*lList) {
                mList = *lList;
                while (mList->next != NULL)
                    mList = mList->next;
                mList->next = nList;
            } else {
                *lList = nList;
            }
        }
        break;
    default:
        SemanticError(&fSymb->loc, ERROR_S_ILLEGAL_TYPE_UNIFORM_VAR,
                      GetAtomString(atable, fSymb->name));
        break;
    }
    return OK;
} // lBindUniformVariable

/*
 * lResolveGlobalVaryingDirection() - A varying-domain global has no
 *         out qualifier, so its connector direction is inferred from the
 *         semantic: input first, then output.  HAL errors pass through;
 *         a semantic that binds in neither direction reports unknown
 *         semantics here.  Returns 1 with *fIsOutVal set on success.
 */

static int lResolveGlobalVaryingDirection(Symbol *fSymb, Binding *lBind,
                                          int *fIsOutVal)
{
    SourceLoc *loc = &fSymb->loc;
    int lname = fSymb->details.var.semantics;
    int errorsBefore = GetErrorCount();

    if (Cg->theHAL->BindVaryingSemantic(loc, fSymb, lname, lBind, 0)) {
        *fIsOutVal = 0;
        return 1;
    }
    if (GetErrorCount() != errorsBefore)
        return 0;
    /* Reset the probe binding before retrying in the output direction. */
    lBind->none.properties = 0;
    lBind->none.gname = 0;
    lBind->none.lname = 0;
    lBind->none.base = 0;
    lBind->none.size = 0;
    lBind->none.kind = BK_NONE;
    if (Cg->theHAL->BindVaryingSemantic(loc, fSymb, lname, lBind, 1)) {
        *fIsOutVal = 1;
        return 1;
    }
    if (GetErrorCount() == errorsBefore) {
        SemanticError(loc, ERROR_S_UNKNOWN_SEMANTICS,
                      GetAtomString(atable, fSymb->name));
    }
    return 0;
} // lResolveGlobalVaryingDirection

/*
 * lBindVaryingVariable() - Bind a variable to $vin, $vout, or $uniform connector.
 *
 */

static Symbol *lBindVaryingVariable(Symbol *fSymb, int gname, int IsOutVal,
                                    int IsStructMember, int structSemantics,
                                    int IsGlobal)
{
    int category, domain, qualifiers;
    Symbol *lSymb, *mSymb;
    BindingTree *ltree;
    Binding *lBind;
    Scope *lScope;
    int lname, errorsBefore;

    lSymb = NULL;
    category = GetCategory(fSymb->type);
    domain = GetDomain(fSymb->type);
    qualifiers = GetQualifiers(fSymb->type);
    switch (category) {
    case TYPE_CATEGORY_SCALAR:
    case TYPE_CATEGORY_ARRAY:
    case TYPE_CATEGORY_SAMPLER:
        lScope = NULL;
        lname = 0;
        lBind = /* theHAL-> */ NewBinding(gname, fSymb->name);
        ltree = LookupBinding(gname, fSymb->name);
        if (fSymb->details.var.semantics) {
            if (ltree) {
                SemanticWarning(&fSymb->loc, WARNING_S_SEMANTICS_AND_BINDING,
                                GetAtomString(atable, fSymb->name));
            }
            lname = fSymb->details.var.semantics;
            if (IsGlobal &&
                !lResolveGlobalVaryingDirection(fSymb, lBind, &IsOutVal))
            {
                return NULL;
            }
            /* Profiles without canonical conflict ownership use the common
             * case-insensitive output map before profile binding. */
            if (IsOutVal &&
                !Cg->theHAL->GetCapsBit(
                    CAPS_CANONICAL_OUTPUT_SEMANTIC_CONFLICTS) &&
                lCheckOutputSemantic(fSymb, lname))
            {
                return NULL;
            }
            errorsBefore = GetErrorCount();
            if (Cg->theHAL->BindVaryingSemantic(&fSymb->loc, fSymb, lname, lBind, IsOutVal)) {
                if (lBind->none.properties & BIND_INPUT) {
                    if (IsOutVal) {
                        SemanticError(&fSymb->loc, ERROR_S_OUT_QUALIFIER_IN_SEMANTIC,
                                      GetAtomString(atable, fSymb->name));
                    }
                    lScope = Cg->theHAL->varyingIn->type->str.members;
                } else {
                    if (!IsOutVal) {
                        SemanticError(&fSymb->loc, ERROR_S_IN_QUALIFIER_OUT_SEMANTIC,
                                      GetAtomString(atable, fSymb->name));
                    }
                    lScope = Cg->theHAL->varyingOut->type->str.members;
                }
            } else if (GetErrorCount() == errorsBefore) {
                SemanticError(&fSymb->loc, ERROR_S_UNKNOWN_SEMANTICS,
                              GetAtomString(atable, fSymb->name));
            }
        } else {
            // If no semantics, check for #pragma bind directive.
            lname = fSymb->name;
            if (ltree) {
                errorsBefore = GetErrorCount();
                if (Cg->theHAL->BindVaryingPragma(&fSymb->loc, fSymb, lBind, &ltree->binding, IsOutVal)) {
                    if (lBind->none.properties & BIND_UNIFORM) {
                        SemanticError(&fSymb->loc, ERROR_S_UNIF_BIND_TO_NON_UNIF_VAR,
                                      GetAtomString(atable, fSymb->name));
                    } else {
                        if (lBind->none.properties & BIND_INPUT) {
                            lScope = Cg->theHAL->varyingIn->type->str.members;
                        } else {
                            lScope = Cg->theHAL->varyingOut->type->str.members;
                        }
                    }
                } else if (GetErrorCount() == errorsBefore) {
                    SemanticError(&fSymb->loc, ERROR_S_INCOMPATIBLE_BIND_DIRECTIVE,
                                  GetAtomString(atable, fSymb->name));
                }
            } else {
                // If no semantics or #pragma bind, get default binding from profile if it allows them:
                errorsBefore = GetErrorCount();
                if (Cg->theHAL->BindVaryingUnbound(&fSymb->loc, fSymb, lname, structSemantics, lBind, IsOutVal)) {
                    if (IsOutVal) {
                        lScope = Cg->theHAL->varyingOut->type->str.members;
                    } else {
                        lScope = Cg->theHAL->varyingIn->type->str.members;
                    }
                } else if (GetErrorCount() == errorsBefore) {
                    SemanticError(&fSymb->loc, ERROR_S_SEMANTIC_NOT_DEFINED_VOUT,
                                  GetAtomString(atable, fSymb->name));
                }
            }
        }
        if (lScope) {
            lBind->none.lname = lname;
            fSymb->details.var.bind = lBind;
            if (!(lBind->none.properties & BIND_HIDDEN)) {
                lSymb = LookUpLocalSymbol(lScope, lname);
                if (lSymb) {
                    // Already defined - second use of this name.
                } else {
                    lSymb = AddSymbol(&fSymb->loc, lScope, lname, fSymb->type, VARIABLE_S);
                    lSymb->details.var.bind = lBind;
                    if (lScope->symbols != lSymb) {
                        mSymb = lScope->symbols;
                        while (mSymb->next)
                            mSymb = mSymb->next;
                        mSymb->next = lSymb;
                    }
                }
            }
        }
        break;
    case TYPE_CATEGORY_STRUCT:
        SemanticError(&fSymb->loc, ERROR_S_NESTED_SEMANTIC_STRUCT,
                      GetAtomString(atable, fSymb->name));
        break;
    default:
        SemanticError(&fSymb->loc, ERROR_S_ILLEGAL_PARAM_TO_MAIN,
                      GetAtomString(atable, fSymb->name));
        break;
    }
    return lSymb;
} // lBindVaryingVariable

/*
 * lVerifyConnectorDirection() - Verify that this connector name is valid for the current
 *         profile and is of the appropriate direction.
 */

static void lVerifyConnectorDirection(SourceLoc *loc, int semantics, int IsOutParam)
{
    int cid, uses;

    // If connector semantics present make sure that connector direction matches parameter's:

    if (semantics) {
        cid = Cg->theHAL->GetConnectorID(semantics);
        if (cid) {
            uses = Cg->theHAL->GetConnectorUses(cid, Cg->theHAL->pid);
            if (IsOutParam) {
                if (!(uses & CONNECTOR_IS_OUTPUT))
                    SemanticError(loc, ERROR_S_CONNECT_FOR_INPUT,
                                  GetAtomString(atable, semantics));
            } else {
                if (!(uses & CONNECTOR_IS_INPUT))
                    SemanticError(loc, ERROR_S_CONNECT_FOR_OUTPUT,
                                  GetAtomString(atable, semantics));
            }
        } else {
            SemanticError(loc, ERROR_S_CONNECTOR_TYPE_INVALID,
                          GetAtomString(atable, semantics));
        }
    }
} // lVerifyConnectorDirection

/*
 * lSynthesizeEntryReturnConnector() - Wrap a non-struct entry return type in
 *         a single-member connector struct so downstream profiles see the
 *         struct-return shape they already handle.  The member's semantic is
 *         the one recorded on the program's declarator; the member name is
 *         the semantic atom itself, since binding and return lowering key on
 *         the semantic when one is present.  Reports
 *         ERROR_S_PROGRAM_RETURN_NEEDS_SEMANTIC and returns NULL when no
 *         semantic was recorded.
 */

static Type *lSynthesizeEntryReturnConnector(SourceLoc *loc, Scope *fScope,
                                             Symbol *program, Type *rettype)
{
    int tag, lname;
    Scope *members;
    Symbol *member;
    Type *connector;

    lname = program->details.fun.semantics;
    if (!lname) {
        SemanticError(&program->loc, ERROR_S_PROGRAM_RETURN_NEEDS_SEMANTIC,
                      GetAtomString(atable, program->name));
        return NULL;
    }
    tag = AddAtom(atable, "$progret");
    connector = StructHeader(loc, fScope, 0, tag);
    members = NewScope();
    members->HasSemantics = 1;
    members->level = 1;
    members->IsStructScope = 1;
    connector->str.members = members;
    member = DefineVar(loc, members, lname, rettype);
    member->details.var.semantics = lname;
    SetStructMemberOffsets(connector);
    return connector;
} // lSynthesizeEntryReturnConnector

/*
 * lBindGlobalVarying() - Bind one explicit varying-domain global into the
 *         program interface: reading it copies its connector member in
 *         before main's body and writing it copies out with the other
 *         entry outputs.
 */

static void lBindGlobalVarying(Symbol *lGlobal, Symbol *program,
                               Symbol *vinVar, Symbol *voutVar,
                               StmtList *instmts, StmtList *outstmts)
{
    Symbol *lBound;
    Binding *lBind;
    expr *lExpr, *rExpr, *vExpr;
    stmt *lStmt;
    int len, rlen;

    lBound = lBindVaryingVariable(lGlobal, 0, 0, 0, 0, 1);
    if (! lBound)
        return;
    lBind = lBound->details.var.bind;
    if (!lBind || (lBind->none.properties & BIND_HIDDEN))
        return;
    lExpr = GenSymb(lGlobal);
    if (IsScalar(lGlobal->type) || IsVector(lGlobal->type, &len)) {
        if (lBind->none.properties & BIND_INPUT) {
            // Assign $vin member to bound global:
            vExpr = (expr *) NewSymbNode(VARIABLE_OP, vinVar);
            rExpr = GenMemberReference(vExpr, lBound);
            if (IsVector(lBound->type, &rlen))
                rExpr = GenConvertVectorLength(rExpr, GetBase(lBound->type), rlen, len);
            lStmt = NewSimpleAssignmentStmt(&program->loc, lExpr, rExpr, 0);
            AppendStatements(instmts, lStmt);
        } else {
            // Assign bound global to $vout member:
            vExpr = (expr *) NewSymbNode(VARIABLE_OP, voutVar);
            rExpr = GenMemberReference(vExpr, lBound);
            if (IsVector(lBound->type, &rlen))
                lExpr = GenConvertVectorLength(lExpr, GetBase(lGlobal->type), len, rlen);
            lStmt = NewSimpleAssignmentStmt(&program->loc, rExpr, lExpr, 0);
            AppendStatements(outstmts, lStmt);
        }
    } else {
        FatalError("Parameter of unsupported type");
        // xxx
    }
} // lBindGlobalVarying

/*
 * lBindGlobalVaryingTree() - Walk the global scope's symbol tree binding
 *         every explicit varying-domain scalar or array global.  Other
 *         globals keep their existing treatment: default-domain globals
 *         stay on the lazy uniform path (CheckForGlobalUniformReferences),
 *         and helper parameters never reach binding semantics at all.
 */

static void lBindGlobalVaryingTree(Symbol *lSymb, Symbol *vinVar,
                                   Symbol *voutVar, Symbol *program,
                                   StmtList *instmts, StmtList *outstmts)
{
    int category;

    if (! lSymb)
        return;
    lBindGlobalVaryingTree(lSymb->left, vinVar, voutVar, program,
                           instmts, outstmts);
    if (lSymb != vinVar && lSymb != voutVar &&
        lSymb->kind == VARIABLE_S &&
        lSymb->storageClass != SC_STATIC)
    {
        category = GetCategory(lSymb->type);
        if ((category == TYPE_CATEGORY_SCALAR ||
             category == TYPE_CATEGORY_ARRAY) &&
            EffectiveProgramDomain(lSymb, 0) == TYPE_DOMAIN_VARYING)
        {
            lBindGlobalVarying(lSymb, program, vinVar, voutVar,
                               instmts, outstmts);
        }
    }
    lBindGlobalVaryingTree(lSymb->right, vinVar, voutVar, program,
                           instmts, outstmts);
} // lBindGlobalVaryingTree

/*
 * lEntryResolvesToGeometry() - Silent pre-check of the same topology
 *         resolution selected-program analysis performs: should this
 *         compilation's entry keep its geometry-interface formals out
 *         of the $vin/$vout binder?  Resolution failures never bind
 *         here -- option and configuration problems are analysis
 *         diagnostics with their own anchors -- so any compilation
 *         whose raw options or modifiers express geometry intent
 *         answers yes and lets analysis report precisely.
 */

static int lEntryResolvesToGeometry(Symbol *program)
{
    CgGeometryModifiers source;
    CgGeometryOptions options;
    CgGeometryConfig config;
    CgGeometryDiagnostic diagnostic;

    if (!CgLanguageAllowsGeometry(Cg->options.languageVersion)) {
        return 0;
    }
    source = program->details.fun.geometry;
    CgGeometryInitOptions(&options);
    if (!CgGeometryParseOptions(Cg->options.profileOptions, &options,
                                &diagnostic)) {
        return 1;
    }
    if (!CgGeometryResolveConfig(&source, &options,
                                 CgProfileProgramStage(
                                     &Cg->theHAL->profileIdentity),
                                 &config, &diagnostic)) {
        return 1;
    }
    return config.stage == CGIR_STAGE_GEOMETRY;
} // lEntryResolvesToGeometry

/*
 * BuildSemanticStructs() - Build the three global semantic type structure,  Check main for
 *         type errors in its arguments.
 */

void BuildSemanticStructs(SourceLoc *loc, Scope *fScope, Symbol *program)
{
    int category, domain, qualifiers, len, rlen;
    int isGeometryProgram;
    Scope *vinScope, *voutScope, *lScope;
    Type *vinType, *voutType;
    Symbol *vinVar, *voutVar;
    int vinTag, voutTag;
    Symbol *formal, *member, *lSymb;
    expr *lExpr, *rExpr, *vExpr;
    Type *lType, *rettype;
    StmtList instmts, outstmts;
    Binding *lBind;
    int IsOutParam, entryDomain;
    float lVal[4];
    stmt *lStmt;

    // One program interface per compilation: reset the output map.

    lOutputSemantics = NULL;
    isGeometryProgram = lEntryResolvesToGeometry(program);

    // Define pseudo type structs for semantics:

    vinScope = NewScope();
    vinScope->HasSemantics = 1;
    vinScope->level = 1;
    vinScope->IsStructScope = 1;
    voutScope = NewScope();
    voutScope->HasSemantics = 1;
    voutScope->level = 1;
    voutScope->IsStructScope = 1;

    vinTag = AddAtom(atable, "$vin");
    vinType = StructHeader(loc, fScope, 0, vinTag);
    vinType->str.members = vinScope;
    Cg->theHAL->varyingIn = vinVar = DefineVar(loc, fScope, vinTag, vinType);
    //vinTypedef = DefineTypedef(loc, fScope, vinTag, vinType); // Not sure this is neessary
    voutTag = AddAtom(atable, "$vout");
    voutType = StructHeader(loc, fScope, 0, voutTag);
    voutType->str.members = voutScope;
    Cg->theHAL->varyingOut = voutVar = DefineVar(loc, fScope, voutTag, voutType);
    //voutTypedef = DefineTypedef(loc, fScope, voutTag, voutType); // Not sure this is neessary

    instmts.first = instmts.last = NULL;
    outstmts.first = outstmts.last = NULL;

    // Bind explicit varying-domain globals into the program interface
    // ahead of the parameters; see lBindGlobalVaryingTree().

    lBindGlobalVaryingTree(fScope->symbols, vinVar, voutVar, program,
                           &instmts, &outstmts);

    // Walk list of formals creating semantic struct members for all parameters:

    formal = program->details.fun.params;
    while (formal) {
        category = GetCategory(formal->type);
        domain = GetDomain(formal->type);
        qualifiers = GetQualifiers(formal->type);
        if (IsSampler(formal->type, NULL)) {
            /* Samplers enter a program only through its uniform
             * interface; varying-domain sampler parameters have no
             * binding semantics at the language level. */
            if (domain != TYPE_DOMAIN_UNIFORM ||
                (qualifiers & (TYPE_QUALIFIER_OUT | TYPE_QUALIFIER_INOUT)))
            {
                SemanticError(&formal->loc, ERROR_S_ILLEGAL_PARAM_TO_MAIN,
                              GetAtomString(atable, formal->name));
                formal = formal->next;
                continue;
            }
        }
        if ((qualifiers & TYPE_QUALIFIER_INOUT) == TYPE_QUALIFIER_INOUT) {
            if (Cg->theHAL->GetCapsBit(CAPS_HLSL_GEOMETRY_ENTRY_ABI)) {
                SemanticError(&formal->loc, ERROR_S_HLSL_ENTRY_ABI,
                              "geometry inout parameter");
            } else if (!Cg->theHAL->GetCapsBit(
                           CAPS_ENTRY_INOUT_PARAMETERS))
            {
                SemanticError(&formal->loc,
                              ERROR_S_MAIN_PARAMS_CANT_BE_INOUT,
                              GetAtomString(atable, formal->name));
            }
        }
        entryDomain = EffectiveProgramDomain(formal, 1);
        if (isGeometryProgram && entryDomain != TYPE_DOMAIN_UNIFORM &&
            (category == TYPE_CATEGORY_ATTRIB_ARRAY ||
             CgGeometryClassifySemantic(formal->details.var.semantics) !=
                 CG_GEOMETRY_SEMANTIC_ORDINARY))
        {
            /* A geometry program's vertex/primitive interface --
             * attribute arrays and geometry-special bindings alike --
             * publishes through Cg IR geometry metadata, not the
             * $vin/$vout connector: no varying binding exists for it,
             * so the formal passes through untouched and selected-
             * program analysis owns its rules. */
            formal = formal->next;
            continue;
        }
        if (entryDomain == TYPE_DOMAIN_UNIFORM) {
            if (qualifiers & TYPE_QUALIFIER_OUT) {
                SemanticError(&formal->loc, ERROR_S_UNIFORM_ARG_CANT_BE_OUT,
                              GetAtomString(atable, formal->name));
            }
            switch (category) {
            case TYPE_CATEGORY_SCALAR:
            case TYPE_CATEGORY_ARRAY:
            case TYPE_CATEGORY_STRUCT:
            case TYPE_CATEGORY_SAMPLER:
                if (lBindUniformVariable(formal, program->name, 1) && formal->details.var.init) {
                    formal->details.var.init = FoldConstants(formal->details.var.init);
                    if (Cg->theHAL->GetCapsBit(
                            CAPS_AGGREGATE_DEFAULT_BINDINGS))
                    {
                        memset(lVal, 0, sizeof(lVal));
                    } else {
                        GetVectorConst(lVal, formal->details.var.init);
                    }
                    lBind = NewConstDefaultBinding(0, formal->name, 4, 0, 0, lVal);
                    lBind->constdef.kind = BK_DEFAULT;
                    AddDefaultBinding(lBind, formal,
                                      formal->details.var.init,
                                      formal->type);
                }
                break;
            default:
                SemanticError(&formal->loc, ERROR_S_ILLEGAL_PARAM_TO_MAIN,
                              GetAtomString(atable, formal->name));
                break;
            }
        } else {
            IsOutParam = (qualifiers & TYPE_QUALIFIER_OUT) != 0;
            switch (category) {
            case TYPE_CATEGORY_SCALAR:
            case TYPE_CATEGORY_ARRAY:
                lSymb = lBindVaryingVariable(formal, program->name, IsOutParam, 0,
                                             0, 0);
                if (lSymb) {
                    lBind = lSymb->details.var.bind;
                    if (lBind && !(lBind->none.properties & BIND_HIDDEN)) {
                        lExpr = GenSymb(formal);
                        if (IsScalar(formal->type) || IsVector(formal->type, &len)) {
                            if (lBind->none.properties & BIND_INPUT) {
                                // Assign $vin member to bound variable:
                                vExpr = (expr *) NewSymbNode(VARIABLE_OP, vinVar);
                                rExpr = GenMemberReference(vExpr, lSymb);
                                if (IsVector(lSymb->type, &rlen))
                                    rExpr = GenConvertVectorLength(rExpr, GetBase(lSymb->type), rlen, len);
                                lStmt = NewSimpleAssignmentStmt(&program->loc, lExpr, rExpr, 0);
                                AppendStatements(&instmts, lStmt);
                            } else {
                                // Assign bound variable to $vout member:
                                vExpr = (expr *) NewSymbNode(VARIABLE_OP, voutVar);
                                rExpr = GenMemberReference(vExpr, lSymb);
                                if (IsVector(lSymb->type, &rlen))
                                    lExpr = GenConvertVectorLength(lExpr, GetBase(formal->type), len, rlen);
                                lStmt = NewSimpleAssignmentStmt(&program->loc, rExpr, lExpr, 0);
                                AppendStatements(&outstmts, lStmt);
                            }
                        } else {
                            FatalError("Parameter of unsupported type");
                            // xxx
                        }
                    }
                }
                break;
            case TYPE_CATEGORY_STRUCT:
                lType = formal->type;
                lVerifyConnectorDirection(&formal->loc, lType->str.semantics, IsOutParam);
                lScope = lType->str.members;
                member = lScope->symbols;
                while (member) {
                    lSymb = lBindVaryingVariable(member, lType->str.tag, IsOutParam, 1,
                                                 lType->str.semantics, 0);
                    if (lSymb) {
                        lBind = lSymb->details.var.bind;
                        if (lBind && !(lBind->none.properties & BIND_HIDDEN)) {
                            lExpr = GenMemberReference((expr *) NewSymbNode(VARIABLE_OP, formal), member);
                            if (IsScalar(member->type) || IsVector(member->type, &len)) {
                                if (lBind->none.properties & BIND_INPUT) {
                                    // Assign $vin member to bound variable:
                                    vExpr = (expr *) NewSymbNode(VARIABLE_OP, vinVar);
                                    rExpr = GenMemberReference(vExpr, lSymb);
                                    if (IsVector(lSymb->type, &rlen))
                                        rExpr = GenConvertVectorLength(rExpr, GetBase(lSymb->type), rlen, len);
                                    lStmt = NewSimpleAssignmentStmt(&program->loc, lExpr, rExpr, 0);
                                    AppendStatements(&instmts, lStmt);
                                } else {
                                    // Assign bound variable to $vout member:
                                    vExpr = (expr *) NewSymbNode(VARIABLE_OP, voutVar);
                                    rExpr = GenMemberReference(vExpr, lSymb);
                                    if (IsVector(lSymb->type, &rlen))
                                        lExpr = GenConvertVectorLength(lExpr, GetBase(member->type), len, rlen);
                                    lStmt = NewSimpleAssignmentStmt(&program->loc, rExpr, lExpr, 0);
                                    AppendStatements(&outstmts, lStmt);
                                }
                            } else {
                                FatalError("Parameter of unsupported type");
                                // xxx
                            }
                        }
                    }
                    member = member->next;
                }
                break;
            default:
                SemanticError(&formal->loc, ERROR_S_ILLEGAL_PARAM_TO_MAIN,
                              GetAtomString(atable, formal->name));
                break;
            }
        }
        formal = formal->next;
    }

    // Add return value's semantics to the $vout connector:

    lType = program->type;
    rettype = lType->fun.rettype;
    category = GetCategory(rettype);
    if (!IsVoid(rettype)) {
        if (category != TYPE_CATEGORY_STRUCT) {
            rettype = lSynthesizeEntryReturnConnector(loc, fScope, program,
                                                      rettype);
            if (rettype != NULL) {
                if (!Cg->theHAL->GetCapsBit(
                        CAPS_PRESERVE_TERMINAL_ENTRY_RETURN))
                {
                    lType->fun.rettype = rettype;
                }
                category = TYPE_CATEGORY_STRUCT;
            }
        }
        if (category == TYPE_CATEGORY_STRUCT) {
            lVerifyConnectorDirection(&program->loc, rettype->str.semantics, 1);
            lScope = rettype->str.members;
            member = lScope->symbols;
            while (member) {
                lSymb = lBindVaryingVariable(member, rettype->str.tag, 1, 1,
                                             rettype->str.semantics, 0);
                member = member->next;
            }
        }
    }

    // Set the output connector variety:

    voutType->str.variety = Cg->theHAL->outcid;

    // Add input assignments and retain profile-managed output assignments:

    program->details.fun.statements = ConcatStmts(instmts.first,
                                                  program->details.fun.statements);
    if (Cg->theHAL->GetCapsBit(CAPS_PRESERVE_ENTRY_RETURNS)) {
        program->details.fun.entryOutputAssignments = outstmts.first;
    } else {
        program->details.fun.statements = ConcatStmts(
            program->details.fun.statements, outstmts.first);
    }

} // BuildSemanticStructs


void BindDefaultSemantic(Symbol *lSymb, int category, int gname)
{
    Binding *lBind;
    float lVal[4];

    switch (category) {
    case TYPE_CATEGORY_SCALAR:
    case TYPE_CATEGORY_ARRAY:
    case TYPE_CATEGORY_STRUCT:
    case TYPE_CATEGORY_SAMPLER:
        gname = 0;
        if (lBindUniformVariable(lSymb, gname, 0) && lSymb->details.var.init) {
            lSymb->details.var.init = FoldConstants(lSymb->details.var.init);
            if (Cg->theHAL->GetCapsBit(
                    CAPS_AGGREGATE_DEFAULT_BINDINGS))
            {
                memset(lVal, 0, sizeof(lVal));
            } else {
                GetVectorConst(lVal, lSymb->details.var.init);
            }
            lBind = NewConstDefaultBinding(0, lSymb->name, 4, 0, 0, lVal);
            lBind->constdef.kind = BK_DEFAULT;
            AddDefaultBinding(lBind, lSymb,
                              lSymb->details.var.init, lSymb->type);
        }
        break;
    default:
        SemanticError(&lSymb->loc, ERROR_S_NON_STATIC_GLOBAL_TYPE_ERR,
                      GetAtomString(atable, lSymb->name));
    }
    lSymb->properties &= ~SYMB_NEEDS_BINDING;
} // BindDefaultSemantic
