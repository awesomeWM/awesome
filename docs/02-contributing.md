# Contributing


## Bugs

Please look at `https://github.com/awesomeWM/awesome/issues`.

## Style

If you intend to patch and contribute to awesome, please respect the
following guidelines.

Imitate the existing code style. For concrete rules:

 - Use 4 spaces indentation, do not use tabulator characters;

 - Place braces alone on new lines, and do not place braces for single
   line statement where it is not needed, i.e. no:

    if(bla) {
        x = 1;
    }

 - Do not put a space after if, for, while or function call statements;

 - The preferred line length is 80 characters;

 - Use `/* */` for comments;

 - Use the API: there's a list of `a_*()` function you should use instead
   of the standard libc ones. There is also common API for linked list,
   tabulars, etc.;

 - Be *clear* in what you do;

 - Prefix your function name with the module they are enhancing,
   i.e. if you add a function to manipulate a tag prefix it with `tag_`.

 - Write documentation for any new functions, options, whatever.

A vim modeline is set in each file to respect this.

## Patches

If you plan to submit patches, you should follow the following guidelines.

### Commits

- make commits of logical units;
- do not modify piece of code not related to your commit;
- do not try to fix style of code you are not writing,
  it's just adding noise for no gain;
- check for unnecessary whitespace with `git diff --check` before committing;
- do not check in commented out code or unneeded files;
- provide a meaningful commit message;
- the first line of the commit message should be a short;
  description and should skip the full stop;
- if you want your work included, add a
  `Signed-off-by: Your Name &lt;you@example.com>` line to the
  commit message (or just use the option `-s` when committing);
- make sure that you have tests for the bug you are fixing.
- if possible, add a unit test to the test suite under spec/.

### Patches

- use `git format-patch -M` to create the patch;
- do *not* PGP sign your patch;
- be *careful* doing cut & paste into your mailer, not to corrupt whitespaces;
- if you change, add or remove the user API, the associated documentation
  should be updated as well;
- send the patch to the list (`awesome-devel@naquadah.org`) if (and only if)
  the patch is ready for inclusion. If you use git-send-email(1), please
  *test* it first by sending email to yourself.
