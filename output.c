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
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
WHETHER UNDER THEORY OF CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// output.c - Same-directory temporary output with commit/abort.
//
// BeginOutputTransaction() opens "<destination>.cgc-tmp-<pid>-<n>" for
// binary write -- the first candidate name that does not exist yet --
// or hands back stdout when no destination is supplied.  The temporary
// lives next to the destination so the final replace stays within one
// file system.  CommitOutputTransaction() flushes, checks the stream,
// closes, and replaces the destination atomically (MoveFileExA with
// MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH on Windows,
// rename on POSIX).  AbortOutputTransaction() closes and removes only
// the exact temporary path it opened.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#define CGC_GETPID() ((int) _getpid())
#else
#include <unistd.h>
#define CGC_GETPID() ((int) getpid())
#endif

#include "output.h"

// Bound on candidate names tried before giving up; each attempt uses a
// fresh counter suffix, so exhaustion means something is deeply wrong.

#define CGC_TEMP_ATTEMPTS 1024

static int lTempCounter = 0;

/*
 * BeginOutputTransaction() - Start generating into a private stream.
 *          With a NULL/empty destination the transaction wraps stdout
 *          and always succeeds; otherwise it returns nonzero holding an
 *          open temporary stream, zero on any failure with the
 *          transaction reset to empty.
 */

int BeginOutputTransaction(OutputTransaction *transaction,
                           const char *destination)
{
    int attempt;

    memset(transaction, 0, sizeof(*transaction));
    if (!destination || !*destination) {
        transaction->stream = stdout;
        transaction->isStdout = 1;
        return 1;
    }
    transaction->destination = destination;
    transaction->temporary = malloc(strlen(destination) + 40);
    if (!transaction->temporary) {
        memset(transaction, 0, sizeof(*transaction));
        return 0;
    }
    for (attempt = 0; attempt < CGC_TEMP_ATTEMPTS; attempt++) {
        FILE *probe;

        sprintf(transaction->temporary, "%s.cgc-tmp-%d-%d",
                destination, CGC_GETPID(), lTempCounter++);
        probe = fopen(transaction->temporary, "rb");
        if (probe) {
            /* Candidate already taken; never touch another run's
             * temporary, just try the next suffix. */
            fclose(probe);
            continue;
        }
        transaction->stream = fopen(transaction->temporary, "wb");
        if (transaction->stream)
            return 1;
        break;
    }
    free(transaction->temporary);
    memset(transaction, 0, sizeof(*transaction));
    return 0;
}

/*
 * CommitOutputTransaction() - Finish a successful compilation: flush,
 *          check, close, then replace the destination.  On any failure
 *          the destination keeps its previous contents and only the
 *          exact temporary path is removed.  Returns zero on success,
 *          nonzero on failure, with the transaction reset either way.
 */

int CommitOutputTransaction(OutputTransaction *transaction)
{
    FILE *stream = transaction->stream;
    char *temporary = transaction->temporary;
    const char *destination = transaction->destination;
    int failed;

    if (transaction->isStdout) {
        failed = fflush(stream) != 0 || ferror(stream) != 0;
        memset(transaction, 0, sizeof(*transaction));
        return failed ? 1 : 0;
    }
    failed = fflush(stream) != 0 || ferror(stream) != 0 ||
             fclose(stream) != 0;
    if (!failed) {
#if defined(_WIN32)
        failed = !MoveFileExA(temporary, destination,
                              MOVEFILE_REPLACE_EXISTING |
                              MOVEFILE_WRITE_THROUGH);
#else
        failed = rename(temporary, destination) != 0;
#endif
    }
    if (failed)
        remove(temporary);
    free(temporary);
    memset(transaction, 0, sizeof(*transaction));
    return failed ? 1 : 0;
}

/*
 * AbortOutputTransaction() - Discard a transaction: close the
 *          temporary stream and remove exactly the temporary path this
 *          transaction opened.  Stdout transactions are flushed but
 *          never closed.  Safe on an already-finished transaction.
 */

void AbortOutputTransaction(OutputTransaction *transaction)
{
    if (transaction->stream && !transaction->isStdout)
        fclose(transaction->stream);
    if (transaction->temporary) {
        remove(transaction->temporary);
        free(transaction->temporary);
    }
    memset(transaction, 0, sizeof(*transaction));
}
