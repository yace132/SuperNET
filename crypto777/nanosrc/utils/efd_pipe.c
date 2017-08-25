/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include "err.h"
#include "fast.h"
#include "int.h"
#include "closefd.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

int nn_efd_init(struct nn_efd *self)
{
    int rc,flags,p[2];
    PNACL_message("inside efd_init: pipe\n");
    //sleep(1);
#if defined NN_HAVE_PIPE2
    rc = pipe2(p, O_NONBLOCK | O_CLOEXEC);
#else
    rc = pipe(p);
#endif
    PNACL_message("rc efd_init: %d\n",rc);
    //sleep(1);
    if (rc != 0 && (errno == EMFILE || errno == ENFILE))
        return -EMFILE;
    errno_assert (rc == 0);
    self->r = p[0];
    self->w = p[1];

#if !defined NN_HAVE_PIPE2 && defined FD_CLOEXEC
    rc = fcntl (self->r, F_SETFD, FD_CLOEXEC);
    PNACL_message("pipe efd_init: F_SETFDr rc %d\n",rc);
    errno_assert(rc != -1);
    rc = fcntl(self->w, F_SETFD, FD_CLOEXEC);
    PNACL_message("pipe efd_init: F_SETFDw rc %d\n",rc);
    errno_assert(rc != -1);
#endif

#if !defined NN_HAVE_PIPE2
    flags = fcntl(self->r, F_GETFL, 0);
    PNACL_message("pipe efd_init: flags %d\n",flags);
    if ( flags == -1 )
        flags = 0;
    rc = fcntl(self->r, F_SETFL, flags | O_NONBLOCK);
    PNACL_message("pipe efd_init: rc %d flags.%d\n",rc,flags);
    errno_assert (rc != -1);
#endif

    return 0;
}

void nn_efd_term (struct nn_efd *self)
{
    nn_closefd (self->r);
    nn_closefd (self->w);
}

nn_fd nn_efd_getfd (struct nn_efd *self)
{
    return self->r;
}

void nn_efd_signal (struct nn_efd *self)
{
    ssize_t nbytes;
    char c = 101;

    nbytes = write (self->w, &c, 1);
    errno_assert (nbytes != -1);
    nn_assert (nbytes == 1);
}

void nn_efd_unsignal (struct nn_efd *self)
{
    ssize_t nbytes;
    uint8_t buf [16];

    while (1) {
        nbytes = read (self->r, buf, sizeof (buf));
        if (nbytes < 0 && errno == EAGAIN)
            nbytes = 0;
        errno_assert (nbytes >= 0);
        if (nn_fast ((size_t) nbytes < sizeof (buf)))
            break;
    }
}

