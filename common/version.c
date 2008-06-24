/*
 * version.c - version message utility functions
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2008 Hans Ulrich Niedermann <hun@n-dimensional.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "common/version.h"
#include "awesome-version-internal.h"

/** \brief Print version message and quit program.
 * \param executable program name
 */
void
eprint_version(const char *const executable)
{
    printf("%s (awesome) " AWESOME_VERSION
	   " (" AWESOME_RELEASE ")\n"
	   "built",
	   executable);
#if defined(__DATE__) && defined(__TIME__)
    printf(" " __DATE__ " " __TIME__);
#endif
    printf(" for %s", AWESOME_COMPILE_MACHINE);
#if defined(__GNUC__)				\
    && defined(__GNUC_MINOR__)			\
    && defined(__GNUC_PATCHLEVEL__)
    printf(" by gcc version %d.%d.%d",
	   __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
    printf(" (%s@%s)\n", AWESOME_COMPILE_BY, AWESOME_COMPILE_HOSTNAME);
    printf("* Image drawing engine: ");
#ifdef WITH_IMLIB2
    printf("Imlib2\n");
#else
    printf("GdkPixBuf\n");
#endif
    printf("* DBus support: ");
#ifdef WITH_DBUS
    printf("enabled\n");
#else
    printf("disabled\n");
#endif
    exit(EXIT_SUCCESS);
}

/** Get version string.
 * \return A string describing the current version.
 */
const char*
version_string(void)
{
    return AWESOME_VERSION;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
