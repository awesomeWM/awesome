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
#include <stdlib.h>
#include <sys/stat.h>
#include <getopt.h>

#define KEY_VALUE_BUF_MAX 64
#define READ_BUF_MAX 127

static void
set_api_level(char *value)
{
    if (!value)
        return;

    char *ptr;
    int ret = strtol(value, &ptr, 10);

    /* There is no valid number at all */
    if (value == ptr) {
        fprintf(stderr, "Invalid API level %s\n", value);
        return;
    }

    /* There is a number, but also letters, this is invalid */
    if (ptr[0] != '\0' && ptr[0] != '.') {
        fprintf(stderr, "Invalid API level %s\n", value);
        return;
    }

    /* This API level doesn't exist, fallback to v4 */
    if (ret < 4)
        ret = 4;

    globalconf.api_level = ret;
}

static void
push_arg(string_array_t *args, char *value, size_t *len)
{
    value[*len] = '\0';
    string_array_append(args, a_strdup(value));
    (*len) = 0;
}

/*
 * Support both shebang and modeline modes.
 */
bool
options_init_config(char *execpath, char *configpath, int *init_flags, string_array_t *paths)
{
    /* The different state the parser can have. */
    enum {
        MODELINE_STATE_INIT       , /* Start of line        */
        MODELINE_STATE_NEWLINE    , /* Start of line        */
        MODELINE_STATE_COMMENT    , /* until --             */
        MODELINE_STATE_MODELINE   , /* until awesome_mode:  */
        MODELINE_STATE_SHEBANG    , /* until !              */
        MODELINE_STATE_KEY_DELIM  , /* until the key begins */
        MODELINE_STATE_KEY        , /* until '='            */
        MODELINE_STATE_VALUE_DELIM, /* after ':'            */
        MODELINE_STATE_VALUE      , /* until ',' or '\n'    */
        MODELINE_STATE_COMPLETE   , /* Parsing is done      */
        MODELINE_STATE_INVALID    , /* note a modeline      */
        MODELINE_STATE_ERROR        /* broken modeline      */
    } state = MODELINE_STATE_INIT;

    /* The parsing mode */
    enum {
        MODELINE_MODE_NONE   , /* No modeline */
        MODELINE_MODE_LINE   , /* modeline    */
        MODELINE_MODE_SHEBANG, /* #! shebang  */
    } mode = MODELINE_MODE_NONE;

    static const unsigned char name[] = "awesome_mode:";
    static char key_buf [KEY_VALUE_BUF_MAX+1] = {'\0'};
    static char file_buf[READ_BUF_MAX+1     ] = {'\0'};
    size_t pos = 0;

    string_array_t argv;
    string_array_init(&argv);
    string_array_append(&argv, a_strdup(execpath));

    FILE *fp = fopen(configpath, "r");

    /* Share the error codepath with parsing errors */
    if (!fp)
        return false;

    /* Try to read the first line */
    if (!fgets(file_buf, READ_BUF_MAX, fp)) {
        fclose(fp);
        return false;
    }

    unsigned char c;

    /* Simple state machine to translate both modeline and shebang into argv */
    for (int i = 0; (c = file_buf[i++]) != '\0';) {
        /* Be very permissive, skip the unknown, UTF is not allowed */
        if ((c > 126 || c < 32) && c != 10 && c != 13 && c != 9)
            continue;

        switch (state) {
            case MODELINE_STATE_INIT:
                switch (c) {
                    case '#':
                        state = MODELINE_STATE_SHEBANG;
                        break;
                    case ' ': case '-':
                        state = MODELINE_STATE_COMMENT;
                        break;
                    default:
                        state = MODELINE_STATE_INVALID;
                }
                break;
            case MODELINE_STATE_NEWLINE:
                switch (c) {
                    case ' ': case '-':
                        state = MODELINE_STATE_COMMENT;
                        break;
                    default:
                        state = MODELINE_STATE_INVALID;
                }
                break;
            case MODELINE_STATE_COMMENT:
                switch (c) {
                    case '-':
                        state = MODELINE_STATE_MODELINE;
                        break;
                    default:
                        state = MODELINE_STATE_INVALID;
                }
                break;
            case MODELINE_STATE_MODELINE:
                if (c == ' ')
                    break;
                else if (c != name[pos++]) {
                    state = MODELINE_STATE_INVALID;
                    pos   = 0;
                }

                if (pos == 13) {
                    pos   = 0;
                    state = MODELINE_STATE_KEY_DELIM;
                    mode  = MODELINE_MODE_LINE;
                }

                break;
            case MODELINE_STATE_SHEBANG:
                switch(c) {
                    case '!':
                        mode  = MODELINE_MODE_SHEBANG;
                        state = MODELINE_STATE_KEY_DELIM;
                        break;
                    default:
                        state = MODELINE_STATE_INVALID;
                }
                break;
            case MODELINE_STATE_KEY_DELIM:
                switch (c) {
                    case ' ': case '\t': case ':': case '=':
                        break;
                    case '\n': case '\r':
                        state = MODELINE_STATE_ERROR;
                        break;
                    default:
                        /* In modeline mode, assume all keys are the long name */
                        switch(mode) {
                            case MODELINE_MODE_LINE:
                                strcpy(key_buf, "--");
                                pos = 2;
                                break;
                            case MODELINE_MODE_SHEBANG:
                            case MODELINE_MODE_NONE:
                                break;
                        };
                        state = MODELINE_STATE_KEY;
                        key_buf[pos++] = c;
                }
                break;
            case MODELINE_STATE_KEY:
                switch (c) {
                    case '=':
                        push_arg(&argv, key_buf, &pos);
                        state = MODELINE_STATE_VALUE_DELIM;
                        break;
                    case ' ': case '\t': case ':':
                        push_arg(&argv, key_buf, &pos);
                        state = MODELINE_STATE_KEY_DELIM;
                        break;
                    default:
                        key_buf[pos++] = c;
                }
                break;
            case MODELINE_STATE_VALUE_DELIM:
                switch (c) {
                    case ' ': case '\t':
                        break;
                    case '\n': case '\r':
                        state = MODELINE_STATE_ERROR;
                        break;
                    case ':':
                        state = MODELINE_STATE_KEY_DELIM;
                        break;
                    default:
                        state = MODELINE_STATE_VALUE;
                        key_buf[pos++] = c;
                }
                break;
            case MODELINE_STATE_VALUE:
                switch(c) {
                    case ',': case ' ': case ':': case '\t':
                        push_arg(&argv, key_buf, &pos);
                        state = MODELINE_STATE_KEY_DELIM;
                        break;
                    case '\n': case '\r':
                        state = MODELINE_STATE_COMPLETE;
                        break;
                    default:
                        key_buf[pos++] = c;
                }
                break;
            case MODELINE_STATE_INVALID:
                /* This cannot happen, the `if` below should prevent it */
                state = MODELINE_STATE_ERROR;
                break;
            case MODELINE_STATE_COMPLETE:
            case MODELINE_STATE_ERROR:
                break;
        }

        /* No keys or values are that large */
        if (pos >= KEY_VALUE_BUF_MAX)
            state = MODELINE_STATE_ERROR;

        /* Stop parsing when completed */
        if (state == MODELINE_STATE_ERROR || state == MODELINE_STATE_COMPLETE)
            break;

        /* Try the next line */
        if (((i == READ_BUF_MAX || file_buf[i+1] == '\0') && !feof(fp)) || state == MODELINE_STATE_INVALID) {
            if (state == MODELINE_STATE_KEY || state == MODELINE_STATE_VALUE)
                push_arg(&argv, key_buf, &pos);

            /* Skip empty lines */
            do {
                if (fgets(file_buf, READ_BUF_MAX, fp))
                    state = MODELINE_STATE_NEWLINE;
                else {
                    state = argv.len ? MODELINE_STATE_COMPLETE : MODELINE_STATE_ERROR;
                    break;
                }

                i = 0; /* Always reset `i` to avoid an unlikely invalid read */
            } while (i == 0 && file_buf[0] == '\n');
        }
    }

    fclose(fp);

    /* Reset the global POSIX args counter */
    optind = 0;

    /* Be future proof, allow let unknown keys live, let the Lua code decide */
    (*init_flags) |= INIT_FLAG_ALLOW_FALLBACK;

    options_check_args(argv.len, argv.tab, init_flags, paths);

    /* Cleanup */
    string_array_wipe(&argv);

    return state == MODELINE_STATE_COMPLETE;
}


char *
options_detect_shebang(int argc, char **argv)
{
    /* There is no cross-platform ways to check if it is *really* called by a
     * shebang. There is a couple Linux specific hacks which work with the
     * most common C libraries, but they wont work on *BSD.
     *
     * On some platforms, the argv is going to be parsed by the OS, in other
     * they will be concatenated in one big string. There is some ambiguities
     * caused by that. For example, `awesome -s foo` and and `#!/bin/awesome -s`
     * are both technically valid if `foo` is a directory in the first and
     * lua file (without extension) in the second. While `-s` with a file
     * wont work, it is hard to know by looking at the string.
     *
     * The trick to avoid any ambiguity is to just read the file and see if
     * the args match. `options_init_config` will be called later and the args
     * will be parsed properly.
     */

    /* On WSL and some other *nix this isn't true, but it is true often enough */
    if (argc > 3 || argc == 1)
        return NULL;

    /* Check if it is executable */
    struct stat inf;
    if (stat(argv[argc-1], &inf) || !(inf.st_mode & S_IXUSR))
        return NULL;

    FILE *fp = fopen(argv[argc-1], "r");

    if (!fp)
        return NULL;

    char buf[3];

    if (!fgets(buf, 2, fp)) {
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    if (!strcmp(buf, "#!"))
        return NULL;

    /* Ok, good enough, this is a shebang script, assume it called `awesome` */
    return a_strdup(argv[argc-1]);
}

/** Print help and exit(2) with given exit_code.
 * \param exit_code The exit code.
 */
static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile,
"Usage: awesome [OPTION]\n\
  -h, --help             show help\n\
  -v, --version          show version\n\
  -c, --config FILE      configuration file to use\n\
  -f, --force            ignore modelines and apply the command line arguments\n\
      --search DIR       add a directory to the library search path\n\
  -k, --check            check configuration file syntax\n\
  -a, --no-argb          disable client transparency support\n\
  -l  --api-level LEVEL  select a different API support level than the current version \n\
  -m, --screen on|off    enable or disable automatic screen creation (default: on)\n\
  -r, --replace          replace an existing window manager\n");
    exit(exit_code);
}

#define ARG 1
#define NO_ARG 0

char *
options_check_args(int argc, char **argv, int *init_flags, string_array_t *paths)
{

    static struct option long_options[] =
    {
        { "help"      , NO_ARG, NULL, 'h'  },
        { "version"   , NO_ARG, NULL, 'v'  },
        { "config"    , ARG   , NULL, 'c'  },
        { "force"     , NO_ARG, NULL, 'f'  },
        { "check"     , NO_ARG, NULL, 'k'  },
        { "search"    , ARG   , NULL, 's'  },
        { "no-argb"   , NO_ARG, NULL, 'a'  },
        { "replace"   , NO_ARG, NULL, 'r'  },
        { "screen"    , ARG   , NULL, 'm'  },
        { "api-level" , ARG   , NULL, 'l'  },
        { "reap"      , ARG   , NULL, '\1' },
        { NULL        , NO_ARG, NULL, 0    }
    };

    char *confpath = NULL;
    int opt;

    while((opt = getopt_long(argc, argv, "vhkc:arml:",
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
            globalconf.have_searchpaths = true;
            string_array_append(paths, a_strdup(optarg));
            break;
          case 'a':
            globalconf.had_overriden_depth = true;
            (*init_flags) &= ~INIT_FLAG_ARGB;
            break;
          case 'r':
            (*init_flags) |= INIT_FLAG_REPLACE_WM;
            break;
          case 'l':
              set_api_level(optarg);
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
