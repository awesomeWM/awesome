/*
 * configopts.c - configuration options
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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
 *
 */

#include "common/configopts.h"
#include "common/util.h"

#define AWESOME_CONFIG_FILE ".awesomerc.lua"

/** Return default configuration file path
 * \return path to the default configuration file
 */
char *
config_file(void)
{
    const char *homedir;
    char * confpath;
    ssize_t confpath_len;

    homedir = getenv("HOME");
    confpath_len = a_strlen(homedir) + a_strlen(AWESOME_CONFIG_FILE) + 2;
    confpath = p_new(char, confpath_len);
    a_strcpy(confpath, confpath_len, homedir);
    a_strcat(confpath, confpath_len, "/");
    a_strcat(confpath, confpath_len, AWESOME_CONFIG_FILE);

    return confpath;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
