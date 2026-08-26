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
// check.c
//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "cg_stdlib.h"    // CgIntrinsicIsGeometrySpecial for call checking
static int CheckFunctionDefinition(Scope *fScope, Symbol *funSymb, int IsProgram);

#define NOT_CHECKED     0
#define BEING_CHECKED   1
#define ALREADY_CHECKED 2

/*
 * CheckSymbolTree()
 *
 */

static int CheckSymbolTree(Scope *fScope, Symbol *fSymb, int IsProgram)
{
    int count = 0;

    if (fSymb) {
        Cg->theHAL->CheckDefinition(&fSymb->loc, fSymb->name, fSymb->type);
        count += CheckSymbolTree(fScope, fSymb->left, IsProgram);
        count += CheckSymbolTree(fScope, fSymb->right, IsProgram);
    }
    return count;
} // CheckSymbolTree

/*
 * CheckParamsAndLocals() - Check this functions format parameters and local variables
 *         for unallowed things.
 */

static int CheckParamsAndLocals(Symbol *funSymb, int IsProgram)
{
    Scope *lScope;
    int count = 0;

    lScope = funSymb->details.fun.locals;
    count += CheckSymbolTree(lScope, lScope->symbols, IsProgram);
    return count;
} // CheckParamsAndLocals

/*
 * BuildProgramReturnAssignments() - Insert a series of assignment statements before each return
 *         statement to set the values of the program's result for these members.  (Should only
 *         be applied to the main program.)  Profiles may preserve nonterminal returns.
 */

struct BuildReturnAssignments {
    Scope *globalScope;
    Symbol *program;
    stmt *terminalReturn;
    stmt *entryOutputAssignments;
    Symbol *returnTemp;
    int preserveReturns;
};

static stmt *DuplicateEntryOutputAssignments(stmt *source)
{
    stmt *copy;
    stmt *list;

    list = NULL;
    while (source != NULL) {
        assert(source->commonst.kind == EXPR_STMT);
        copy = (stmt *) NewExprStmt(&source->commonst.loc,
                                    DupExpr(source->exprst.exp));
        list = ConcatStmts(list, copy);
        source = source->commonst.next;
    }
    return list;
}

static int ProgramReturnExprNeedsTemp(expr *fExpr)
{
    if (fExpr == NULL)
        return 0;
    if (fExpr->common.HasSideEffects)
        return 1;
    switch (fExpr->common.kind) {
    case DECL_N:
    case SYMB_N:
    case CONST_N:
        return 0;
    case UNARY_N:
        return ProgramReturnExprNeedsTemp(fExpr->un.arg);
    case BINARY_N:
        if (fExpr->bin.op == FUN_CALL_OP ||
            fExpr->bin.op == FUN_INTRINSIC_OP) return 1;
        return ProgramReturnExprNeedsTemp(fExpr->bin.left) ||
               ProgramReturnExprNeedsTemp(fExpr->bin.right);
    case TRINARY_N:
        return ProgramReturnExprNeedsTemp(fExpr->tri.arg1) ||
               ProgramReturnExprNeedsTemp(fExpr->tri.arg2) ||
               ProgramReturnExprNeedsTemp(fExpr->tri.arg3);
    default:
        assert(!"bad kind to ProgramReturnExprNeedsTemp()");
        return 1;
    }
}

static Symbol *NewProgramReturnTemp(Symbol *program, Type *type,
                                    SourceLoc *loc)
{
    Scope *scope;
    Symbol *temp;
    char name[64];
    int atom;
    int index;

    scope = program->details.fun.locals;
    index = 0;
    do {
        if (index == 0) {
            strcpy(name, "cg_return");
        } else {
            sprintf(name, "cg_return%d", index);
        }
        atom = AddAtom(atable, name);
        temp = LookUpLocalSymbol(scope, atom);
        index++;
    } while (temp != NULL);
    temp = DefineVar(loc, scope, atom, type);
    temp->properties |= SYMB_IS_NATIVE_AGGREGATE_TEMP;
    return temp;
}

static stmt *BuildProgramReturnAssignments(stmt *fStmt, void *arg1, int arg2)
{
    struct BuildReturnAssignments *lstr;
    Symbol *program, *lSymb, *voutVar, *outSymb, *retSymb;
    Type *lType, *rettype;
    expr *lExpr, *rexpr, *returnVar, *outputVar;
    Scope *lScope, *gScope, *voutScope;
    stmt *lStmt, *stmtlist;
    stmt *sourceReturn;
    int category, len, lname, resultMembers, useReturnTemp;

    if (fStmt->commonst.kind == RETURN_STMT) {
        sourceReturn = fStmt;
        lstr = (struct BuildReturnAssignments *) arg1;
        gScope = lstr->globalScope;
        program = lstr->program;
        lType = program->type;
        rettype = lType->fun.rettype;
        category = GetCategory(rettype);
        if (IsVoid(rettype)) {
            fStmt = NULL;
        } else {
            if (category == TYPE_CATEGORY_STRUCT) {
                stmtlist = NULL;
                voutVar = Cg->theHAL->varyingOut;
                voutScope = voutVar->type->str.members;
                lScope = rettype->str.members;
                resultMembers = 0;
                lSymb = lScope->symbols;
                while (lSymb) {
                    lname = lSymb->details.var.semantics ?
                        lSymb->details.var.semantics : lSymb->name;
                    outSymb = LookUpLocalSymbol(voutScope, lname);
                    retSymb = LookUpLocalSymbol(lScope, lSymb->name);
                    if (outSymb && retSymb)
                        resultMembers++;
                    lSymb = lSymb->next;
                }
                useReturnTemp = lstr->preserveReturns &&
                    resultMembers > 1 && ProgramReturnExprNeedsTemp(
                        sourceReturn->returnst.exp);
                if (useReturnTemp) {
                    if (lstr->returnTemp == NULL) {
                        lstr->returnTemp = NewProgramReturnTemp(
                            program, rettype, &sourceReturn->commonst.loc);
                    }
                    lStmt = NewSimpleAssignmentStmt(
                        &sourceReturn->commonst.loc,
                        GenSymb(lstr->returnTemp),
                        sourceReturn->returnst.exp, 0);
                    stmtlist = ConcatStmts(stmtlist, lStmt);
                }
                lSymb = lScope->symbols;
                while (lSymb) {
                    // Create an assignment statement of the bound variable to the $vout member:
                    lname = lSymb->details.var.semantics ? lSymb->details.var.semantics : lSymb->name;
                    outSymb = LookUpLocalSymbol(voutScope, lname);
                    retSymb = LookUpLocalSymbol(lScope, lSymb->name);
                    if (outSymb && retSymb) {
                        // outSymb may not be in the symbol table if it's a "hidden" register.
                        if (useReturnTemp) {
                            returnVar = GenSymb(lstr->returnTemp);
                        } else {
                            returnVar = DupExpr(
                                sourceReturn->returnst.exp);
                        }
                        outputVar = (expr *) NewSymbNode(VARIABLE_OP, voutVar);
                        lExpr = GenMemberReference(outputVar, outSymb);
                        if (sourceReturn->returnst.exp->common.type != NULL &&
                            GetCategory(sourceReturn->returnst.exp->common.type) !=
                                TYPE_CATEGORY_STRUCT)
                        {
                            // Entry returned through a synthesized single-member
                            // connector: assign the returned value directly:
                            rexpr = returnVar;
                        } else {
                            rexpr = GenMemberReference(returnVar, retSymb);
                        }
                        if (IsScalar(lSymb->type) || IsVector(lSymb->type, &len)) {
                            lStmt = NewSimpleAssignmentStmt(&program->loc, lExpr, rexpr, 0);
                            stmtlist = ConcatStmts(stmtlist, lStmt);
                        } else {
                            FatalError("Return of unsupported type");
                            // xxx
                        }
                    }
                    lSymb = lSymb->next;
                }
                fStmt = stmtlist;
            }
        }
        if (lstr->preserveReturns) {
            if (lstr->entryOutputAssignments != NULL) {
                if (sourceReturn == lstr->terminalReturn) {
                    lStmt = lstr->entryOutputAssignments;
                } else {
                    lStmt = DuplicateEntryOutputAssignments(
                        lstr->entryOutputAssignments);
                }
                fStmt = ConcatStmts(fStmt, lStmt);
            }
            if (sourceReturn != lstr->terminalReturn) {
                lStmt = (stmt *) NewReturnStmt(
                    &sourceReturn->commonst.loc, NULL, NULL);
                fStmt = ConcatStmts(fStmt, lStmt);
            }
        }
    }
    return fStmt;
} // BuildProgramReturnAssignments

/*
 * CheckNodeForUndefinedFunctions() - Check an expression nodefor calls to undefined functions.
 *
 */

static expr *CheckNodeForUndefinedFunctions(expr *fExpr, void *arg1, int arg2)
{
    Symbol *lSymb;
    expr *lExpr;
    int *count = (int *) arg1;

    switch (fExpr->common.kind) {
    case DECL_N:
    case SYMB_N:
    case CONST_N:
    case UNARY_N:
        break;
    case BINARY_N:
        if (fExpr->bin.op == FUN_CALL_OP) {
            lExpr = fExpr->bin.left;
            if (lExpr->common.kind == SYMB_N) {
                lSymb = lExpr->sym.symbol;
                if (IsFunction(lSymb)) {
                    const CgIntrinsicSignature *lSignature =
                        CgIntrinsicSignatureForSymbol(lSymb);

                    if (lSignature &&
                        CgIntrinsicIsGeometrySpecial(lSignature->intrinsic))
                    {
                        /* Geometry operations are catalog intrinsics
                         * without bodies on purpose: selected-program
                         * analysis and lowering own them, so the
                         * defined-body rule never applies. */
                        break;
                    }
                    if (!(lSymb->properties & SYMB_IS_DEFINED)) {
                        SemanticError(Cg->pLastSourceLoc, ERROR_S_CALL_UNDEF_FUN,
                                      GetAtomString(atable, lSymb->name));
                        count++;
                    } else {
                        if (lSymb->flags == BEING_CHECKED) {
                            SemanticError(Cg->pLastSourceLoc, ERROR_S_RECURSION,
                                          GetAtomString(atable, lSymb->name));
                            count++;
                        } else {
                            CheckFunctionDefinition(NULL, lSymb, 0);
                        }
                    }
                }
            }
        }
        break;
    case TRINARY_N:
        break;
    default:
        assert(!"bad kind to CheckNodeForUndefinedFunctions()");
        break;
    }
    return fExpr;
} // CheckNodeForUndefinedFunctions

/*
 * CheckExpressionForUndefinedFunctions() - Check an expression for calls to undefined functions.
 *
 */

static expr *CheckExpressionForUndefinedFunctions(expr *fExpr, void *arg1, int arg2)
{
    PostApplyToNodes(CheckNodeForUndefinedFunctions, fExpr, arg1, arg2);
    return fExpr;
} // CheckExpressionForUndefinedFunctions

/*
 * CheckNodeForUnsupportedOperators() - Check a node for operators not supported
 *         in the target profile.
 */

static expr *CheckNodeForUnsupportedOperators(expr *fExpr, void *arg1, int arg2)
{
    int *count = (int *) arg1;

    switch (fExpr->common.kind) {
    case DECL_N:
    case SYMB_N:
    case CONST_N:
        break;
    case UNARY_N:
        if (!Cg->theHAL->IsValidOperator(Cg->pLastSourceLoc, opcode_atom[fExpr->un.op], fExpr->un.op,
                                         fExpr->un.subop))
        {
            *count++;
        }
        break;
    case BINARY_N:
        if (!Cg->theHAL->IsValidOperator(Cg->pLastSourceLoc, opcode_atom[fExpr->bin.op], fExpr->bin.op,
                                         fExpr->bin.subop))
        {
            *count++;
        }
        break;
    case TRINARY_N:
        if (!Cg->theHAL->IsValidOperator(Cg->pLastSourceLoc, opcode_atom[fExpr->tri.op], fExpr->tri.op,
                                         fExpr->tri.subop))
        {
            *count++;
        }
        break;
    default:
        assert(!"bad kind to CheckNodeForUnsupportedOperators()");
        break;
    }
    return fExpr;
} // CheckNodeForUnsupportedOperators

/*
 * CheckForUnsupportedVariables() - Check for references to object that are not supported
 *         by the target profile.
 */

static expr *CheckForUnsupportedVariables(expr *fExpr, void *arg1, int arg2)
{
    int *count = (int *) arg1;

    switch (fExpr->common.kind) {
    case SYMB_N:
        if (fExpr->sym.op == VARIABLE_OP) {
            ///lSymb = fExpr->sym.symbol;
        }
        break;
    case DECL_N:
    case CONST_N:
    case UNARY_N:
    case BINARY_N:
    case TRINARY_N:
        break;
    default:
        assert(!"bad kind to CheckForUnsupportedVariables()");
        break;
    }
    return fExpr;
} // CheckForUnsupportedVariables

/*
 * CheckForGlobalUniformReferences() - Check for references to previously unreferenced non-static
 *         uniform global variables.  These must have explicit or implied semantics.  Add them to
 *         to the $uniform connector and insert an initialization statement at the start of main.
 */

static expr *CheckForGlobalUniformReferences(expr *fExpr, void *arg1, int arg2)
{
    int category, domain, qualifiers;
    Symbol *lSymb;
    Type *lType;
    int gname;

    switch (fExpr->common.kind) {
    case SYMB_N:
        if (fExpr->sym.op == VARIABLE_OP) {
            lSymb = fExpr->sym.symbol;
            lType = fExpr->sym.type;
            category = GetCategory(lType);
            domain = GetDomain(lSymb->type);
            qualifiers = GetQualifiers(lSymb->type);
            if (lSymb->properties & SYMB_NEEDS_BINDING) {
                // This is a non-static global and has not yet been bound
                gname = 0;
                BindDefaultSemantic(lSymb, category, gname);
            }
        }
        break;
        default:
        break;
    }
    return fExpr;
} // CheckForGlobalUniformReferences

/*
 * CheckForReturnStmts() - Issue an error if a return statement is encountered.
 *
 */

static stmt *CheckForReturnStmts(stmt *fStmt, void *arg1, int arg2)
{
    if (fStmt->commonst.kind == RETURN_STMT)
        SemanticError(&fStmt->commonst.loc, ERROR___RETURN_NOT_LAST);
    return fStmt;
} // CheckForReturnStmts

/*
 * CheckForUnsupportedStatements() - Issue an error if an unsupported statement is encountered.
 *
 */

static stmt *CheckForUnsupportedStatements(stmt *fStmt, void *arg1, int arg2)
{
    if (fStmt) {
        if (!Cg->theHAL->CheckStatement(&fStmt->commonst.loc, fStmt))
            ++(int *) arg1;
    }
    return fStmt;
} // CheckForUnsupportedStatements

/*
 * CheckJumpStatements() - Reject loop-control statements outside a loop.
 *         This is a source-language rule shared by every profile.
 */

static int CheckJumpStatements(stmt *fStmt, int loopDepth)
{
    int count;

    count = 0;
    for (; fStmt != NULL; fStmt = fStmt->commonst.next) {
        switch (fStmt->commonst.kind) {
        case IF_STMT:
            count += CheckJumpStatements(fStmt->ifst.thenstmt, loopDepth);
            count += CheckJumpStatements(fStmt->ifst.elsestmt, loopDepth);
            break;
        case WHILE_STMT:
        case DO_STMT:
            count += CheckJumpStatements(fStmt->whilest.body,
                                         loopDepth + 1);
            break;
        case FOR_STMT:
            count += CheckJumpStatements(fStmt->forst.init, loopDepth);
            count += CheckJumpStatements(fStmt->forst.step, loopDepth);
            count += CheckJumpStatements(fStmt->forst.body,
                                         loopDepth + 1);
            break;
        case BLOCK_STMT:
            count += CheckJumpStatements(fStmt->blockst.body, loopDepth);
            break;
        case BREAK_STMT:
            if (loopDepth == 0) {
                SemanticError(&fStmt->commonst.loc,
                              ERROR_S_JUMP_NOT_IN_LOOP, "break");
                count++;
            }
            break;
        case CONTINUE_STMT:
            if (loopDepth == 0) {
                SemanticError(&fStmt->commonst.loc,
                              ERROR_S_JUMP_NOT_IN_LOOP, "continue");
                count++;
            }
            break;
        case EXPR_STMT:
        case RETURN_STMT:
        case DISCARD_STMT:
        case COMMENT_STMT:
            break;
        default:
            assert(!"bad kind to CheckJumpStatements()");
            break;
        }
    }
    return count;
} // CheckJumpStatements

/*
 * BindUnboundUniformMembers() - Bind any members that are currently unbound.  Must be a
 *         uniform pseudo-connector.
 */

static void BindUnboundUniformMembers(SymbolList *fList)
{
    Symbol *lSymb;
    Binding *lBind;
    int errorsBefore;

    while (fList != NULL) {
        lSymb = fList->symb;
        if (lSymb) {
            lBind = lSymb->details.var.bind;
            if (lBind && !(lBind->none.properties & BIND_IS_BOUND)) {
                errorsBefore = GetErrorCount();
                if (!Cg->theHAL->BindUniformUnbound(&lSymb->loc, lSymb, lBind)) {
                    if (GetErrorCount() == errorsBefore) {
                        SemanticWarning(&lSymb->loc, WARNING_S_CANT_BIND_UNIFORM_VAR,
                                        GetAtomString(atable, lSymb->name));
                    }
                }
            }
        }
        fList = fList->next;
    }
} // BindUnboundUniformMembers

/*
 * CheckFunctionDefinition()
 *
 */

static int CheckFunctionDefinition(Scope *fScope, Symbol *funSymb, int IsProgram)
{
    int count = 0;
    stmt *lStmt;

    if (funSymb->flags == NOT_CHECKED) {
        funSymb->flags = BEING_CHECKED;
        lStmt = funSymb->details.fun.statements;
        CheckParamsAndLocals(funSymb, IsProgram);
        count += CheckJumpStatements(lStmt, 0);
        if (IsProgram) {
            struct BuildReturnAssignments lstr;
            stmt *terminalReturn;

            lstr.globalScope = fScope;
            lstr.program = funSymb;
            lstr.entryOutputAssignments =
                funSymb->details.fun.entryOutputAssignments;
            lstr.returnTemp = NULL;
            terminalReturn = lStmt;
            while (terminalReturn != NULL &&
                   terminalReturn->commonst.next != NULL)
            {
                terminalReturn = terminalReturn->commonst.next;
            }
            if (terminalReturn != NULL &&
                terminalReturn->commonst.kind != RETURN_STMT)
            {
                terminalReturn = NULL;
            }
            lstr.terminalReturn = terminalReturn;
            lstr.preserveReturns = Cg->theHAL->GetCapsBit(
                CAPS_PRESERVE_ENTRY_RETURNS);
            lStmt = PreApplyToStatements(BuildProgramReturnAssignments,
                                         lStmt, &lstr, 0);
            if (lstr.preserveReturns && terminalReturn == NULL) {
                lStmt = ConcatStmts(lStmt,
                    lstr.entryOutputAssignments);
            }
            funSymb->details.fun.statements = lStmt;
        }
        ApplyToTopExpressions(CheckExpressionForUndefinedFunctions, lStmt, &count, 0);
        PostApplyToExpressions(CheckNodeForUnsupportedOperators, lStmt, &count, 0);
        PostApplyToExpressions(CheckForUnsupportedVariables, lStmt, &count, 0);
        PostApplyToExpressions(CheckForGlobalUniformReferences, lStmt, 0, 0);
        PreApplyToStatements(CheckForUnsupportedStatements, lStmt, &count, 0);
        if (Cg->theHAL->GetCapsBit(CAPS_RESTRICT_RETURNS)) {
            while (lStmt) {
                if (lStmt->commonst.next)
                    CheckForReturnStmts(lStmt, NULL, 0);
                PostApplyToChildStatements(CheckForReturnStmts, lStmt, NULL, 0);
                lStmt = lStmt->commonst.next;
            }
        }
        funSymb->flags = ALREADY_CHECKED;
    }
    return count;
} // CheckFunctionDefinition

/*
 * CheckFunctionDefinitions() - Walk a function and check for errors:
 *     1. see if any functions it calls aren't defined,
 *     2. detect recursion,
 *     3. detect early return statements,
 *     4. check for unsupported operators in the target profile.
 *     5. Build uniform and varying pseudo structs: $vin. $vout. $main, $global
 */

int CheckFunctionDefinitions(SourceLoc *loc, Scope *fScope, Symbol *program)
{
    int count;

    SetSymbolFlagsList(fScope, NOT_CHECKED);
    count = CheckFunctionDefinition(fScope, program, 1);
    SetStructMemberOffsets(Cg->theHAL->varyingIn->type);
    SetStructMemberOffsets(Cg->theHAL->varyingOut->type);
    BindUnboundUniformMembers(Cg->theHAL->uniformParam);
    BindUnboundUniformMembers(Cg->theHAL->uniformGlobal);
    return count;
} // CheckFunctionDefinitions

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Check Connector Usage ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * CheckConnectorUsage() - Check connector usage for illegal references.
 *
 */

static expr *CheckConnectorUsage(expr *fExpr, void *arg1, int arg2)
{
    int WeAreWriting = arg2;
    Symbol *lSymb;
    expr *lExpr;
    Binding *lBind;

    lExpr = fExpr;
    if (fExpr) {
        switch (fExpr->common.kind) {
        case SYMB_N:
            lSymb = lExpr->sym.symbol;
            if (lSymb->properties & SYMB_IS_CONNECTOR_REGISTER) {
                if (!(lSymb->properties & (SYMB_CONNECTOR_CAN_WRITE | SYMB_CONNECTOR_CAN_READ))) {
                    SemanticError(Cg->pLastSourceLoc, ERROR_S_CMEMBER_NOT_VISIBLE,
                                  GetAtomString(atable, lSymb->name));
                } else {
                    if (WeAreWriting) {
                        if (!(lSymb->properties & SYMB_CONNECTOR_CAN_WRITE)) {
                            SemanticError(Cg->pLastSourceLoc, ERROR_S_CMEMBER_NOT_WRITABLE,
                                          GetAtomString(atable, lSymb->name));
                        }
                    } else {
                        if (!(lSymb->properties & SYMB_CONNECTOR_CAN_READ)) {
                            SemanticError(Cg->pLastSourceLoc, ERROR_S_CMEMBER_NOT_READABLE,
                                          GetAtomString(atable, lSymb->name));
                        }
                    }
                }
            }
            break;
        case CONST_N:
            break;
        case UNARY_N:
            fExpr->un.arg = CheckConnectorUsage(fExpr->un.arg, arg1, arg2);
            break;
        case BINARY_N:
            switch (fExpr->bin.op) {
            case MEMBER_SELECTOR_OP:
                lExpr = fExpr->bin.right;
                if (lExpr && lExpr->common.kind == SYMB_N && WeAreWriting) {
                    // Mark connector registers that are written.
                    lSymb = lExpr->sym.symbol;
                    lBind = lSymb->details.var.bind;
                    if (lBind)
                        lBind->none.properties |= BIND_WAS_WRITTEN;
                }
                fExpr->bin.left = CheckConnectorUsage(fExpr->bin.left, arg1, arg2);
                fExpr->bin.right = CheckConnectorUsage(fExpr->bin.right, arg1, arg2);
                lExpr = fExpr;
                break;
            case ASSIGN_OP:
            case ASSIGN_V_OP:
            case ASSIGN_GEN_OP:
            case ASSIGN_DYN_OP:
            case ASSIGN_MASKED_KV_OP:
                fExpr->bin.left = CheckConnectorUsage(fExpr->bin.left, arg1, 1);
                fExpr->bin.right = CheckConnectorUsage(fExpr->bin.right, arg1, 0);
                break;
            case FUN_ARG_OP:
                arg2 = SUBOP_GET_MASK(fExpr->bin.subop) & 2 ? 1 : 0;
                fExpr->bin.left = CheckConnectorUsage(fExpr->bin.left, arg1, arg2);
                fExpr->bin.right = CheckConnectorUsage(fExpr->bin.right, arg1, 0);
                break;
            default:
                fExpr->bin.left = CheckConnectorUsage(fExpr->bin.left, arg1, arg2);
                fExpr->bin.right = CheckConnectorUsage(fExpr->bin.right, arg1, arg2);
                break;
            }
            break;
        case TRINARY_N:
            switch (fExpr->bin.op) {
            case ASSIGN_COND_OP:
            case ASSIGN_COND_V_OP:
            case ASSIGN_COND_SV_OP:
            case ASSIGN_COND_GEN_OP:
                fExpr->tri.arg1 = CheckConnectorUsage(fExpr->tri.arg1, arg1, 1);
                fExpr->tri.arg2 = CheckConnectorUsage(fExpr->tri.arg2, arg1, 0);
                fExpr->tri.arg3 = CheckConnectorUsage(fExpr->tri.arg3, arg1, 0);
                break;
            default:
                fExpr->tri.arg1 = CheckConnectorUsage(fExpr->tri.arg1, arg1, arg2);
                fExpr->tri.arg2 = CheckConnectorUsage(fExpr->tri.arg2, arg1, arg2);
                fExpr->tri.arg3 = CheckConnectorUsage(fExpr->tri.arg3, arg1, arg2);
                break;
            }
            break;
        default:
            FatalError("bad kind to CheckConnectorUsage()");
            break;
        }
    } else {
        lExpr = NULL;
    }
    return lExpr;
} // CheckConnectorUsage

/*
 * CheckConnectorUsageMain() - Check connector usage for illegal references.
 *
 */

void CheckConnectorUsageMain(Symbol *program, stmt *fStmt)
{
    Symbol *outConn, *lSymb;
    Type *cType;
    int len, cid;
    Binding *lBind;

    outConn = Cg->theHAL->varyingOut;
    if (!outConn || !outConn->type)
        return;
    cType = outConn->type;
    cid = cType->str.variety;
    len = Cg->theHAL->GetConnectorRegister(cid, 1, -1, NULL);
    ApplyToTopExpressions(CheckConnectorUsage, fStmt, NULL, 0);
    lSymb = Cg->theHAL->varyingOut->type->str.members->symbols;
    // This doesn't work!  The output value is always written by the return statement!  RSG
    while (lSymb) {
        lBind = lSymb->details.var.bind;
        if (lBind) {
            if ((lBind->none.properties & BIND_WRITE_REQUIRED) &&
                !(lBind->none.properties & BIND_WAS_WRITTEN))
            {
                SemanticWarning(&program->loc, WARNING_S_CMEMBER_NOT_WRITTEN,
                    GetAtomString(atable, lBind->conn.rname));
            }
        }
        lSymb = lSymb->next;
    }
} // CheckConnectorUsageMain

///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * CheckForHiddenVaryingReferences() - Check for references to varying l-values with the
 *         "hidden" semantic bit set.  These shouldn't be referenced in this profile.
 */

expr *CheckForHiddenVaryingReferences(expr *fExpr, void *arg1, int arg2)
{
    Binding *lBind;
    Symbol *lSymb;
    Type *lType;

    switch (fExpr->common.kind) {
    case SYMB_N:
        if (fExpr->sym.op == VARIABLE_OP || fExpr->sym.op == MEMBER_OP) {
            lSymb = fExpr->sym.symbol;
            if (lSymb->kind == VARIABLE_S) {
                lType = fExpr->sym.type;
                lBind = lSymb->details.var.bind;
                if (lBind && lBind->none.properties & BIND_HIDDEN) {
                    SemanticError(Cg->pLastSourceLoc, ERROR_SS_VAR_SEMANTIC_NOT_VISIBLE,
                        GetAtomString(atable, lSymb->name),
                        GetAtomString(atable, lBind->conn.rname));
                }
            }
        }
        break;
        default:
        break;
    }
    return fExpr;
} // CheckForHiddenVaryingReferences

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// End of check.c ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
