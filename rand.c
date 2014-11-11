/*-
 * Copyright (c) 2000 Mark R. V. Murray & Jeroen C. van Gelderen
 * Copyright (c) 2001-2004 Mark R. V. Murray
 * Copyright (c) 2014 Eitan Adler
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Random number generator based on XKCD/221. This code originated from
 * FreeBSD's /dev/null driver. Changes made by Shawn Webb.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/priv.h>
#include <sys/disk.h>
#include <sys/filio.h>

#include <machine/bus.h>
#include <machine/vmparam.h>

/* For use with destroy_dev(9). */
static struct cdev *xkcd_dev;

static d_write_t xkcd_write;
static d_ioctl_t xkcd_ioctl;
static d_read_t xkcd_read;

static struct cdevsw xkcd_cdevsw = {
	.d_version =	D_VERSION,
	.d_read =	xkcd_read,
	.d_write =	xkcd_write,
	.d_ioctl =	xkcd_ioctl,
	.d_name =	"rand",
	.d_flags =	D_MMAP_ANON,
};

/* ARGSUSED */
static int
xkcd_write(struct cdev *dev __unused, struct uio *uio, int flags __unused)
{
	uio->uio_resid = 0;

	return (0);
}

/* ARGSUSED */
static int
xkcd_ioctl(struct cdev *dev __unused, u_long cmd, caddr_t data __unused,
	   int flags __unused, struct thread *td)
{
	int error;
	error = 0;

	switch (cmd) {
	case FIONBIO:
		break;
	case FIOASYNC:
		if (*(int *)data != 0)
			error = EINVAL;
		break;
	default:
		error = ENOIOCTL;
	}
	return (error);
}

/* ARGSUSED */
static int
xkcd_read(struct cdev *dev __unused, struct uio *uio, int flags __unused)
{
	const char zbuf[1] = { 0x04 };
	int error = 0;

	KASSERT(uio->uio_rw == UIO_READ,
	    ("Can't be in %s for write", __func__));
	while (uio->uio_resid > 0 && error == 0)
		error = uiomove((void *)zbuf, 1, uio);

	return (error);
}

/* ARGSUSED */
static int
xkcd_modevent(module_t mod __unused, int type, void *data __unused)
{
	switch(type) {
	case MOD_LOAD:
		xkcd_dev = make_dev_credf(MAKEDEV_ETERNAL_KLD, &xkcd_cdevsw, 0,
		    NULL, UID_ROOT, GID_WHEEL, 0666, "rand");
		break;

	case MOD_UNLOAD:
		destroy_dev(xkcd_dev);
		break;

	case MOD_SHUTDOWN:
		break;

	default:
		return (EOPNOTSUPP);
	}

	return (0);
}

DEV_MODULE(rand, xkcd_modevent, NULL);
MODULE_VERSION(rand, 1);
