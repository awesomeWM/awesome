/*
 * awesome-check.c - awesome configuration file testing
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include <confuse.h>

#include "common/awesome-version.h"
#include "common/util.h"
#include "common/configopts.h"

#define PROGNAME "awesome-check"

/** Print help and exit(2) with given exit_code.
 */
static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile, "Usage: %s [-v | -h | -c configfile]\n", PROGNAME);
    exit(exit_code);
}

int
main(int argc, char *argv[])
{
    cfg_t *cfg;
    char *confpath = NULL;
    const char *homedir = NULL;
    int args_ok = 1, ret;
    ssize_t confpath_len;

    /* check args */
    if(argc >= 2)
    {
        args_ok = 0;
        if(!a_strcmp("-v", argv[1]) || !a_strcmp("--version", argv[1]))
	    eprint_version(PROGNAME);
        else if(!a_strcmp("-h", argv[1]) || !a_strcmp("--help", argv[1]))
            exit_help(EXIT_SUCCESS);
        else if(!a_strcmp("-c", argv[1]))
        {
            if(a_strlen(argv[2]))
                confpath = argv[2], args_ok = 1;
            else
                eprint("-c option requires a file name\n");
        }
        else
            exit_help(EXIT_FAILURE);
    }
    if(!args_ok)
        exit_help(EXIT_FAILURE);

    if(!confpath)
    {
        homedir = getenv("HOME");
        confpath_len = a_strlen(homedir) + a_strlen(AWESOME_CONFIG_FILE) + 2;
        confpath = p_new(char, confpath_len);
        a_strcpy(confpath, confpath_len, homedir);
        a_strcat(confpath, confpath_len, "/");
        a_strcat(confpath, confpath_len, AWESOME_CONFIG_FILE);
    }

    cfg = cfg_init(opts, CFGF_NONE);

    switch((ret = cfg_parse(cfg, confpath)))
    {
      case CFG_FILE_ERROR:
        perror("awesome: parsing configuration file failed");
        break;
      case CFG_PARSE_ERROR:
        cfg_error(cfg, "awesome: parsing configuration file %s failed.\n", confpath);
        break;
      case CFG_SUCCESS:
        printf("Configuration file OK.\n");
        break;
    }

    return ret;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
