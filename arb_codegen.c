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
express or implied, are granted by NVIDIA herein including but not
limited to any patent rights that may be infringed by your derivative
works. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
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
// arb_codegen.c - Legalization, resource validation, metadata, and ARB
//        assembly emission for the private vector IR.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "arb_ir.h"

#define NUMELS(x) (sizeof(x) / sizeof((x)[0]))

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Stage Register Spelling /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Vertex input spellings by attrib number.  A generic ATTRn binding always
 * selects the vertex.attrib[n] spelling instead.
 */

static const struct {
    const char *conventional;
    const char *generic;
} VertexInputNames[16] = {
    { "vertex.position",          "vertex.attrib[0]"  },
    { "vertex.weight",            "vertex.attrib[1]"  },
    { "vertex.normal",            "vertex.attrib[2]"  },
    { "vertex.color.primary",     "vertex.attrib[3]"  },
    { "vertex.color.secondary",   "vertex.attrib[4]"  },
    { "vertex.fogcoord",          "vertex.attrib[5]"  },
    { "vertex.attrib[6]",         "vertex.attrib[6]"  },
    { "vertex.attrib[7]",         "vertex.attrib[7]"  },
    { "vertex.texcoord[0]",       "vertex.attrib[8]"  },
    { "vertex.texcoord[1]",       "vertex.attrib[9]"  },
    { "vertex.texcoord[2]",       "vertex.attrib[10]" },
    { "vertex.texcoord[3]",       "vertex.attrib[11]" },
    { "vertex.texcoord[4]",       "vertex.attrib[12]" },
    { "vertex.texcoord[5]",       "vertex.attrib[13]" },
    { "vertex.texcoord[6]",       "vertex.attrib[14]" },
    { "vertex.texcoord[7]",       "vertex.attrib[15]" },
};

static const char *VertexInputName(int regno)
{
    if (regno < 0 || regno > 15)
        return "<bad-vertex-input>";
    return VertexInputNames[regno].conventional;
} // VertexInputName

static const char *VertexGenericInputName(int regno)
{
    if (regno < 0 || regno > 15)
        return "<bad-vertex-input>";
    return VertexInputNames[regno].generic;
} // VertexGenericInputName

static const char *VertexOutputTexNames[8] = {
    "result.texcoord[0]", "result.texcoord[1]",
    "result.texcoord[2]", "result.texcoord[3]",
    "result.texcoord[4]", "result.texcoord[5]",
    "result.texcoord[6]", "result.texcoord[7]",
};

static const char *FragmentInputTexNames[8] = {
    "fragment.texcoord[0]", "fragment.texcoord[1]",
    "fragment.texcoord[2]", "fragment.texcoord[3]",
    "fragment.texcoord[4]", "fragment.texcoord[5]",
    "fragment.texcoord[6]", "fragment.texcoord[7]",
};

static const char *VertexOutputName(int regno)
{
    switch (regno) {
    case 0:  return "result.position";
    case 1:  return "result.color.front.primary";
    case 2:  return "result.color.front.secondary";
    case 3:  return "result.color.back.primary";
    case 4:  return "result.color.back.secondary";
    case 5:  return "result.fogcoord";
    case 6:  return "result.pointsize";
    default:
        if (regno >= 7 && regno <= 14)
            return VertexOutputTexNames[regno - 7];
        return "<bad-vertex-output>";
    }
} // VertexOutputName

static const char *FragmentInputName(int regno)
{
    switch (regno) {
    case 0:  return "fragment.position";
    case 1:  return "fragment.color.primary";
    case 2:  return "fragment.color.secondary";
    case 3:  return "fragment.fogcoord";
    default:
        if (regno >= 4 && regno <= 11)
            return FragmentInputTexNames[regno - 4];
        return "<bad-fragment-input>";
    }
} // FragmentInputName

static const char *FragmentOutputName(int regno)
{
    switch (regno) {
    case 0:  return "result.color";
    case 1:  return "result.depth";
    default: return "<bad-fragment-output>";
    }
} // FragmentOutputName

static const char *OpcodeName(ArbStage stage, ArbOpcode opcode)
{
    static const char *names[] = {
        "ABS", "ADD", "ARL", "CMP", "COS",
        "DP3", "DP4", "DPH", "DST", "EX2",
        "EXP", "FLR", "FRC", "KIL", "LG2",
        "LIT", "LOG", "LRP", "MAD", "MAX",
        "MIN", "MOV", "MUL", "POW", "RCP",
        "RSQ", "SCS", "SGE", "SIN", "SLT",
        "SUB", "SWZ", "TEX", "TXB", "TXP",
        "XPD"
    };
    if (opcode < 0 || opcode >= ARB_OP_LAST)
        return "<bad-opcode>";

    // The fragment stage rejects the vertex-only scalar opcodes; the
    // validator already screens these before emission.

    return names[opcode];
} // OpcodeName

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Operand and Instruction ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

static int IsIdentitySwizzle(const signed char *swizzle)
{
    return swizzle[0] == 0 && swizzle[1] == 1 &&
           swizzle[2] == 2 && swizzle[3] == 3;
} // IsIdentitySwizzle

static int RegisterFileUsesMask(ArbRegisterFile file)
{
    return file == ARB_REG_TEMP || file == ARB_REG_OUTPUT;
} // RegisterFileUsesMask

/*
 * FormatRegister() - Render the register portion of an operand into buf.
 */

static void FormatRegister(char *buf, int bufsize, const ArbProgram *program,
                           const ArbOperand *operand)
{
    switch (operand->file) {
    case ARB_REG_TEMP:
        // The allocator rewrites TEMP operand indices to physical registers.
        sprintf(buf, "R%d", operand->index);
        break;
    case ARB_REG_INPUT:
        if (program->stage == ARB_STAGE_VERTEX) {
            const char *name = GetAtomString(atable, operand->bindingName);
            if (name[0] == 'A' && name[1] == 'T' && name[2] == 'T' &&
                name[3] == 'R')
            {
                sprintf(buf, "%s", VertexGenericInputName(operand->index));
            } else {
                sprintf(buf, "%s", VertexInputName(operand->index));
            }
        } else {
            sprintf(buf, "%s", FragmentInputName(operand->index));
        }
        break;
    case ARB_REG_OUTPUT:
        if (program->stage == ARB_STAGE_VERTEX)
            sprintf(buf, "%s", VertexOutputName(operand->index));
        else
            sprintf(buf, "%s", FragmentOutputName(operand->index));
        break;
    case ARB_REG_PARAM:
        if (operand->relative) {
            if (operand->relativeOffset >= 0)
                sprintf(buf, "c[A0.x + %d]", operand->relativeOffset);
            else
                sprintf(buf, "c[A0.x - %d]", -operand->relativeOffset);
        } else {
            sprintf(buf, "c[%d]", operand->index);
        }
        break;
    case ARB_REG_CONST:
        sprintf(buf, "literal%d", operand->index);
        break;
    case ARB_REG_ADDRESS:
        sprintf(buf, "A0.x");
        break;
    default:
        sprintf(buf, "<bad-register>");
        break;
    }
} // FormatRegister

/*
 * WriteOperand() - Emit one operand.  Destination operands print their
 *         write mask when it is not xyzw; source operands print negation,
 *         absolute-value bars, and non-identity swizzles.
 */

static int WriteOperand(FILE *out, const ArbProgram *program,
                        const ArbOperand *operand, int destination, int mask)
{
    char regbuf[64];
    int ii;

    FormatRegister(regbuf, sizeof(regbuf), program, operand);
    if (destination) {
        fprintf(out, "%s", regbuf);
        if (RegisterFileUsesMask(operand->file)) {
            if (mask != ARB_MASK_XYZW) {
                fprintf(out, ".");
                if (mask & ARB_MASK_X) fprintf(out, "x");
                if (mask & ARB_MASK_Y) fprintf(out, "y");
                if (mask & ARB_MASK_Z) fprintf(out, "z");
                if (mask & ARB_MASK_W) fprintf(out, "w");
            }
        }
    } else {
        if (operand->negate && !operand->absolute)
            fprintf(out, "-");
        if (operand->absolute)
            fprintf(out, "|");
        fprintf(out, "%s", regbuf);
        if (!IsIdentitySwizzle(operand->swizzle)) {
            fprintf(out, ".");
            for (ii = 0; ii < 4; ii++) {
                switch (operand->swizzle[ii]) {
                case 0: fprintf(out, "x"); break;
                case 1: fprintf(out, "y"); break;
                case 2: fprintf(out, "z"); break;
                default: fprintf(out, "w"); break;
                }
            }
        } else if (operand->file == ARB_REG_PARAM ||
                   operand->file == ARB_REG_CONST ||
                   operand->file == ARB_REG_ADDRESS)
        {
            // Scalar register files read through x; nothing to print.
        }
        if (operand->absolute)
            fprintf(out, "|");
    }
    return 1;
} // WriteOperand

/*
 * WriteInstruction() - Emit one instruction terminated by ";\n".
 */

static int WriteInstruction(FILE *out, const ArbProgram *program,
                            const ArbInstruction *instruction)
{
    int ii;

    fprintf(out, "%s%s", OpcodeName(program->stage, instruction->opcode),
            instruction->saturate ? "_SAT" : "");
    if (instruction->opcode != ARB_OP_KIL) {
        fprintf(out, " ");
        WriteOperand(out, program, &instruction->dst, 1, instruction->mask);
    }
    for (ii = 0; ii < instruction->srcCount; ii++) {
        fprintf(out, "%s", ii == 0 && instruction->opcode != ARB_OP_KIL ?
                            ", " : (ii == 0 ? " " : ", "));
        WriteOperand(out, program, &instruction->src[ii], 0, instruction->mask);
    }
    fprintf(out, ";\n");
    return 1;
} // WriteInstruction

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// Legalization and Allocation ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * ArbLegalizeAndAllocate() - Validate the IR and assign physical temporaries.
 *         With no virtual temporaries in the program there is nothing to
 *         allocate; the linear-scan allocator replaces the tail of this
 *         function as lowering grows.
 */

int ArbLegalizeAndAllocate(ArbProgram *ir, const ArbProfileDesc *profile,
                           SourceLoc *loc)
{
    ArbIRStatus status = ArbValidateIR(ir);
    ArbInstruction *inst;

    if (status != ARB_IR_VALID) {
        InternalError(loc, ERROR___ARB_INVALID_IR);
        return 0;
    }
    for (inst = ir->first; inst; inst = inst->next) {
        if (inst->dst.file == ARB_REG_TEMP &&
            inst->physicalTemp < 0)
        {
            ir->numPhysicalTemps = ir->numVirtualTemps;
            break;
        }
    }
    if (ir->numPhysicalTemps > profile->limits->temporaries) {
        SemanticError(loc, ERROR_SDD_ARB_RESOURCE_LIMIT, "temporaries",
                      ir->numPhysicalTemps, profile->limits->temporaries);
        return 0;
    }
    return 1;
} // ArbLegalizeAndAllocate

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// Resource Validation ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * ArbValidateResources() - Compare the program's resource use with the
 *         portable base-profile limits guaranteed by the Khronos ARB
 *         specifications.  Additional counters join as backend features
 *         come online.
 */

int ArbValidateResources(ArbProgram *ir, const ArbProfileDesc *profile,
                         SourceLoc *loc)
{
    if (ir->numInstructions > profile->limits->instructions) {
        SemanticError(loc, ERROR_SDD_ARB_RESOURCE_LIMIT, "instructions",
                      ir->numInstructions, profile->limits->instructions);
        return 0;
    }
    return 1;
} // ArbValidateResources

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Binding Metadata Output /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

static void ArbPrintUniformVarDescription(FILE *out, const char *symbolName,
                                          Type *fType, Binding *fBind,
                                          int bindOffset,
                                          const char *semanticName,
                                          int paramNo);

static void lFormatTypeParts(Type *fType, char *str1, int str1size,
                             char *str2, int str2size)
{
    FormatTypeStringRT(str1, str1size, str2, str2size, fType, 1);
} // lFormatTypeParts

static void lArbWriteVaryingVar(FILE *out, const char *symbolName,
                                Symbol *fSymb, const char *scopeName,
                                int paramNo)
{
    char str1[100], str2[100], newSymbolName[1024];
    Type *lType;
    Symbol *lSymb;
    int category, len, len2, mname, semantics;
    const char *bindingRegName;
    Binding *lBind;

    lType = fSymb->type;
    if (GetCategory(lType) == TYPE_CATEGORY_FUNCTION) {
        lType = lType->fun.rettype;
        semantics = 0;
        lBind = NULL;
    } else {
        semantics = fSymb->details.var.semantics;
        lBind = fSymb->details.var.bind;
    }
    category = GetCategory(lType);
    switch (category) {
    case TYPE_CATEGORY_SCALAR:
    case TYPE_CATEGORY_ARRAY:
        if (IsScalar(lType) || IsVector(lType, &len) ||
            IsMatrix(lType, &len, &len2))
        {
            lFormatTypeParts(lType, str1, sizeof(str1), str2, sizeof(str2));
            if (lBind) {
                if (lBind->none.properties & BIND_HIDDEN)
                    return;
                mname = semantics ? semantics : lBind->conn.rname;
                switch (lBind->none.kind) {
                case BK_CONNECTOR:
                    bindingRegName = GetAtomString(atable, lBind->conn.rname);
                    break;
                default:
                    bindingRegName = "?????";
                    break;
                }
            } else {
                bindingRegName = "";
            }
            fprintf(out, "#var %s %s%s", str1, symbolName, str2);
            fprintf(out, " : %s.%s", scopeName,
                    GetAtomString(atable, mname));
            fprintf(out, " : %s", bindingRegName);
            fprintf(out, " : %d", paramNo);
            fprintf(out, " : %d", 1);
            fprintf(out, "\n");
        } else {
            fprintf(out, "# *** Can't have varying nonpacked array ***\n");
        }
        break;
    case TYPE_CATEGORY_STRUCT:
        lSymb = lType->str.members->symbols;
        while (lSymb) {
            if (*symbolName != '\0')
                sprintf(newSymbolName, "%s.%s", symbolName,
                        GetAtomString(atable, lSymb->name));
            else
                strcpy(newSymbolName, GetAtomString(atable, lSymb->name));
            lArbWriteVaryingVar(out, newSymbolName, lSymb, scopeName, paramNo);
            lSymb = lSymb->next;
        }
        break;
    default:
        break;
    }
} // lArbWriteVaryingVar

static void ArbPrintUniformVarDescription(FILE *out, const char *symbolName,
                                          Type *fType, Binding *fBind,
                                          int bindOffset,
                                          const char *semanticName,
                                          int paramNo)
{
    char str1[100], str2[100], newSymbolName[1024], newSemanticName[1024];
    Symbol *lSymb;
    int category, len, len2, ii;

    category = GetCategory(fType);
    switch (category) {
    case TYPE_CATEGORY_SCALAR:
    case TYPE_CATEGORY_ARRAY:
        if (IsScalar(fType) || IsVector(fType, &len) ||
            IsMatrix(fType, &len, &len2))
        {
            lFormatTypeParts(fType, str1, sizeof(str1), str2, sizeof(str2));
            fprintf(out, "#var %s ", str1);
            fprintf(out, "%s%s : %s", symbolName, str2, semanticName);
            if (fBind) {
                switch (fBind->none.kind) {
                case BK_REGARRAY:
                    fprintf(out, " : %s[%d]",
                            GetAtomString(atable, fBind->reg.rname),
                            fBind->reg.regno + bindOffset);
                    ii = GetQuadRegSize(fType);
                    if (ii > 1)
                        fprintf(out, ", %d", ii);
                    break;
                case BK_TEXUNIT:
                    fprintf(out, " : texunit %d", fBind->texunit.unitno);
                    break;
                case BK_NONE:
                    fprintf(out, " : ");
                    break;
                default:
                    fprintf(out, " : ?????");
                    break;
                }
            } else {
                fprintf(out, " : ");
            }
            fprintf(out, " : %d : %d", paramNo, 1);
            fprintf(out, "\n");
        } else {
            for (ii = 0; ii < fType->arr.numels; ii++) {
                sprintf(newSymbolName, "%s[%d]", symbolName, ii);
                if (semanticName && semanticName[0] != '\0')
                    sprintf(newSemanticName, "%s[%d]", semanticName, ii);
                else
                    newSemanticName[0] = '\0';
                ArbPrintUniformVarDescription(out, newSymbolName,
                                              fType->arr.eltype, fBind,
                                              bindOffset, newSemanticName,
                                              paramNo);
                bindOffset += GetQuadRegSize(fType->arr.eltype);
            }
        }
        break;
    case TYPE_CATEGORY_STRUCT:
        lSymb = fType->str.members->symbols;
        while (lSymb) {
            sprintf(newSymbolName, "%s.%s", symbolName,
                    GetAtomString(atable, lSymb->name));
            if (semanticName && semanticName[0] != '\0')
                sprintf(newSemanticName, "%s.%s", semanticName,
                        GetAtomString(atable, lSymb->name));
            else
                newSemanticName[0] = '\0';
            ArbPrintUniformVarDescription(out, newSymbolName, lSymb->type,
                                          lSymb->details.var.bind,
                                          bindOffset +
                                          (lSymb->details.var.addr >> 2),
                                          newSemanticName, paramNo);
            lSymb = lSymb->next;
        }
        break;
    default:
        break;
    }
} // ArbPrintUniformVarDescription

static void lArbWriteConstantBindings(FILE *out, slHAL *fHAL)
{
    BindingList *lBindList = fHAL->constantBindings;
    Binding *lBind;
    int ii;

    while (lBindList) {
        lBind = lBindList->binding;
        if (lBind->none.kind == BK_CONSTANT) {
            fprintf(out, "#const %s[%d] =",
                    GetAtomString(atable, lBind->constdef.rname),
                    lBind->constdef.regno);
            for (ii = 0; ii < lBind->constdef.size; ii++)
                fprintf(out, " %.7g", lBind->constdef.val[ii]);
            fprintf(out, "\n");
        }
        lBindList = lBindList->next;
    }
} // lArbWriteConstantBindings

static void lArbWriteDefaultBindings(FILE *out, slHAL *fHAL)
{
    BindingList *lBindList = fHAL->defaultBindings;
    Binding *lBind;
    int ii;

    while (lBindList) {
        lBind = lBindList->binding;
        if (lBind->none.kind == BK_DEFAULT) {
            fprintf(out, "#default ");
            if (lBind->constdef.gname)
                fprintf(out, " %s.", GetAtomString(atable, lBind->constdef.gname));
            fprintf(out, "%s =", GetAtomString(atable, lBind->constdef.lname));
            for (ii = 0; ii < lBind->constdef.size; ii++)
                fprintf(out, " %.7g", lBind->constdef.val[ii]);
            fprintf(out, "\n");
        }
        lBindList = lBindList->next;
    }
} // lArbWriteDefaultBindings

/*
 * ArbWriteBindingMetadata() - Write vendor/version/profile/program headers
 *         plus #semantic, #var, #const, and #default records using the
 *         compiler's documented output format.  Return-value members are
 *         prefixed with the program name, matching NVIDIA Cg output.
 */

void ArbWriteBindingMetadata(FILE *out, slHAL *fHAL, Symbol *program)
{
    Symbol *lSymb;
    SymbolList *lGlobals;
    UniformSemantic *lUniform;
    const char *semanticName, *symbolName, *varyingScopeName;
    int paramNo;
    Type *retType;

    fprintf(out, "#vendor %s\n", fHAL->vendor);
    fprintf(out, "#version %s\n", fHAL->version);
    fprintf(out, "#profile %s\n",
            GetAtomString(atable, fHAL->profileName));
    fprintf(out, "#program %s\n", GetAtomString(atable, program->name));

    lUniform = fHAL->uniforms;
    while (lUniform) {
        fprintf(out, "#semantic ");
        if (lUniform->gname)
            fprintf(out, "%s.", GetAtomString(atable, lUniform->gname));
        fprintf(out, "%s", GetAtomString(atable, lUniform->vname));
        if (lUniform->semantic)
            fprintf(out, " : %s", GetAtomString(atable, lUniform->semantic));
        fprintf(out, "\n");
        lUniform = lUniform->next;
    }

    lGlobals = fHAL->uniformGlobal;
    while (lGlobals) {
        lSymb = lGlobals->symb;
        if (lSymb) {
            if (lSymb->details.var.semantics)
                semanticName = GetAtomString(atable, lSymb->details.var.semantics);
            else
                semanticName = "";
            symbolName = GetAtomString(atable, lSymb->name);
            ArbPrintUniformVarDescription(out, symbolName, lSymb->type,
                                          lSymb->details.var.bind, 0,
                                          semanticName, -1);
        }
        lGlobals = lGlobals->next;
    }

    paramNo = 0;
    lSymb = program->details.fun.params;
    while (lSymb) {
        symbolName = GetAtomString(atable, lSymb->name);
        if (GetDomain(lSymb->type) & TYPE_DOMAIN_UNIFORM) {
            if (lSymb->details.var.semantics)
                semanticName = GetAtomString(atable, lSymb->details.var.semantics);
            else
                semanticName = "";
            ArbPrintUniformVarDescription(out, symbolName, lSymb->type,
                                          lSymb->details.var.bind, 0,
                                          semanticName, paramNo);
        } else {
            if (GetQualifiers(lSymb->type) & TYPE_QUALIFIER_OUT)
                varyingScopeName = "$vout";
            else
                varyingScopeName = "$vin";
            lArbWriteVaryingVar(out, symbolName, lSymb, varyingScopeName,
                                paramNo);
        }
        paramNo++;
        lSymb = lSymb->next;
    }

    retType = program->type->fun.rettype;
    if (GetCategory(retType) == TYPE_CATEGORY_STRUCT) {
        lArbWriteVaryingVar(out, GetAtomString(atable, program->name),
                            program, "$vout", -1);
    } else if (!IsVoid(retType) && program->details.fun.semantics) {
        // Scalar or vector return bound to an implicit $vout member.
        Scope *voutScope = fHAL->varyingOut->type->str.members;
        Symbol *outSymb = LookUpLocalSymbol(voutScope,
                                            program->details.fun.semantics);
        if (outSymb) {
            char str1[100], str2[100];
            lFormatTypeParts(retType, str1, sizeof(str1), str2, sizeof(str2));
            fprintf(out, "#var %s %s%s", str1,
                    GetAtomString(atable, program->name), "");
            fprintf(out, " : $vout.%s",
                    GetAtomString(atable, program->details.fun.semantics));
            fprintf(out, " : %s",
                    GetAtomString(atable, outSymb->details.var.bind->conn.rname));
            fprintf(out, " : -1 : 1\n");
        }
    }

    lArbWriteConstantBindings(out, fHAL);
    lArbWriteDefaultBindings(out, fHAL);
} // ArbWriteBindingMetadata

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Program Emission /////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * ArbWriteProgram() - Emit declarations, instructions, END, and the
 *         statistics comment.  Declarations grow as backend features
 *         reference them.
 */

int ArbWriteProgram(FILE *out, const ArbProgram *program,
                    const ArbProfileDesc *profile)
{
    const ArbInstruction *inst;

    for (inst = program->first; inst; inst = inst->next)
        WriteInstruction(out, program, inst);
    fprintf(out, "END\n");
    fprintf(out, "# %d instructions, %d R-regs\n",
            program->numInstructions, program->numPhysicalTemps);
    return 1;
} // ArbWriteProgram
