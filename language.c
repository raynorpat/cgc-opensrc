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

#include <string.h>
#include "language.h"

int ParseCgLanguageVersion(const char *text, CgLanguageVersion *version)
{
    if (!strcmp(text, "1.1")) {
        *version = CG_LANGUAGE_1_1;
        return 1;
    }
    if (!strcmp(text, "2.0")) {
        *version = CG_LANGUAGE_2_0;
        return 1;
    }
    return 0;
}

const char *CgLanguageVersionString(CgLanguageVersion version)
{
    return version == CG_LANGUAGE_1_1 ? "1.1" : "2.0";
}

int CgLanguageAtLeast(CgLanguageVersion actual, CgLanguageVersion required)
{
    return actual >= required;
}

/*
 * Cg 2.0 reserved words that the compiler does not implement as keywords.
 * Implemented keywords (the original keyword set, the Cg 2.0 scalar type
 * spellings, and the sampler type names) are recognized through their
 * scanner tokens, not through this table.  Entries with a non-zero flag are
 * recognized case-insensitively; all other entries are case-sensitive.
 */

typedef struct ReservedWordRec {
    const char *text;
    int caseInsensitive;
} ReservedWord;

static const ReservedWord reservedWords[] = {
    { "catch",            0 },
    { "class",            0 },
    { "compile",          1 },
    { "decl",             0 },
    { "emit",             0 },
    { "enum",             0 },
    { "explicit",         0 },
    { "friend",           0 },
    { "namespace",        0 },
    { "noinline",         0 },
    { "operator",         0 },
    { "pass",             1 },
    { "pixelshader",      0 },
    { "private",          0 },
    { "protected",        0 },
    { "public",           0 },
    { "register",         0 },
    { "reinterpret_cast", 0 },
    { "sampler",          0 },
    { "sampler_state",    0 },
    { "shared",           0 },
    { "signed",           0 },
    { "sizeof",           0 },
    { "snorm",            0 },
    { "stateblock",       0 },
    { "string",           0 },
    { "technique",        1 },
    { "template",         0 },
    { "texture",          0 },
    { "throw",            0 },
    { "try",              0 },
    { "typename",         0 },
    { "union",            0 },
    { "unorm",            0 },
    { "using",            0 },
    { "virtual",          0 },
    { "volatile",         0 }
};

/*
 * lEqualCaseInsensitive() - Compare two strings ignoring ASCII case.
 *
 */

static int lEqualCaseInsensitive(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a;
        char cb = *b;

        if (ca >= 'A' && ca <= 'Z')
            ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z')
            cb += 'a' - 'A';
        if (ca != cb)
            return 0;
        a++;
        b++;
    }
    return *a == *b;
} // lEqualCaseInsensitive

/*
 * CgIsReservedWord() - Return 1 if "text" is a reserved word of the given
 *         language version.  Versions below Cg 2.0 reserve nothing here so
 *         legacy sources keep accepting these names as identifiers.
 *
 */

int CgIsReservedWord(const char *text, CgLanguageVersion version)
{
    int count = sizeof(reservedWords)/sizeof(reservedWords[0]);
    int lo = 0;
    int hi = count - 1;
    int ii;

    if (!CgLanguageAtLeast(version, CG_LANGUAGE_2_0))
        return 0;

    // Exact matches cover the case-sensitive majority of the table:

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int cmp = strcmp(text, reservedWords[mid].text);

        if (cmp == 0)
            return 1;
        if (cmp < 0)
            hi = mid - 1;
        else
            lo = mid + 1;
    }

    // Case-insensitive entries also match other capitalizations:

    for (ii = 0; ii < count; ii++) {
        if (reservedWords[ii].caseInsensitive &&
            lEqualCaseInsensitive(text, reservedWords[ii].text))
        {
            return 1;
        }
    }
    return 0;
} // CgIsReservedWord
