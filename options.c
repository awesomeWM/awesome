/*
 * stack.h - client stack management header
 *
 * Copyright © 2020 Emmanuel Lepage-Vallee <elv1313@gmail.com>
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

#include "options.h"
#include "common/version.h"

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>

/** Print help and exit(2) with given exit_code.
 * \param exit_code The exit code.
 */
static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile,
"Usage: awesome [OPTION]\n\
  -h, --help           show help\n\
  -v, --version        show version\n\
  -f, --force          ignore modelines and apply the command line arguments\n\
  -c, --config FILE    configuration file to use\n\
      --search DIR     add a directory to the library search path\n\
  -k, --check          check configuration file syntax\n\
  -a, --no-argb        disable client transparency support\n\
  -m, --screen on|off  enable or disable automatic screen creation (default: on)\n\
  -r, --replace        replace an existing window manager\n");
    exit(exit_code);
}

#define ARG 1
#define NO_ARG 0

char *
options_check_args(int argc, char **argv, int *init_flags, string_array_t *paths)
{

    static struct option long_options[] =
    {
        { "help",    NO_ARG, NULL, 'h'  },
        { "version", NO_ARG, NULL, 'v'  },
        { "config",  ARG   , NULL, 'c'  },
        { "force" ,  NO_ARG, NULL, 'f'  },
        { "check",   NO_ARG, NULL, 'k'  },
        { "search",  ARG   , NULL, 's'  },
        { "no-argb", NO_ARG, NULL, 'a'  },
        { "replace", NO_ARG, NULL, 'r'  },
        { "screen" , ARG   , NULL, 'm'  },
        { "reap",    ARG   , NULL, '\1' },
        { NULL,      NO_ARG, NULL, 0    }
    };

    char *confpath = NULL;
    int opt;

    while((opt = getopt_long(argc, argv, "vhkc:arm:",
                             long_options, NULL)) != -1) {
        switch(opt)
        {
          case 'v':
            eprint_version();
            break;
          case 'h':
            if (! ((*init_flags) & INIT_FLAG_ALLOW_FALLBACK))
                exit_help(EXIT_SUCCESS);
            break;
          case 'f':
            (*init_flags) |= INIT_FLAG_FORCE_CMD_ARGS;
            break;
          case 'k':
            (*init_flags) |= INIT_FLAG_RUN_TEST;
            break;
          case 'c':
            if (confpath != NULL)
                fatal("--config may only be specified once");
            confpath = a_strdup(optarg);
            break;
          case 'm':
            /* Validation */
            if ((!optarg) || !(A_STREQ(optarg, "off") || A_STREQ(optarg, "on")))
                fatal("The possible values of -m/--screen are \"on\" or \"off\"");

            globalconf.no_auto_screen = A_STREQ(optarg, "off");

            (*init_flags) &= ~INIT_FLAG_AUTO_SCREEN;

            break;
          case 's':
            string_array_append(paths, a_strdup(optarg));
            break;
          case 'a':
            (*init_flags) &= ~INIT_FLAG_ARGB;
            break;
          case 'r':
            (*init_flags) |= INIT_FLAG_REPLACE_WM;
            break;
          case '\1':
            /* Silently ignore --reap and its argument */
            break;
          default:
            if (! ((*init_flags) & INIT_FLAG_ALLOW_FALLBACK))
                exit_help(EXIT_FAILURE);
            break;
        }}

    return confpath;
}

#undef AR
#undef NO_ARG
#undef KEY_VALUE_BUF_MAX
#undef READ_BUF_MAX
