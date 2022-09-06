# Contributing

## Bugs

Please look at [GitHub Issues](https://github.com/awesomeWM/awesome/issues).

## Triage Issues [![Open Source Helpers](https://www.codetriage.com/AwesomeWM/awesome/badges/users.svg)](https://www.codetriage.com/AwesomeWM/awesome)

You can triage issues which may include reproducing bug reports or asking for vital information, such as version numbers or reproduction instructions. If you would like to start triaging issues, one easy way to get started is to [subscribe to AwesomeWM on CodeTriage](https://www.codetriage.com/AwesomeWM/awesome).

## Style

If you intend to patch and contribute to Awesome, please respect the
following guidelines.

Imitate the existing code style. For concrete rules:

 - Use 4 spaces indentation, do not use tabulator characters;

 - Place braces alone on new lines, and do not place braces for single
   line statement where it is not needed, i.e.:

        if(foo)
            x = 1;

        if(foo)
        {
            x = 1;
            bar();
        }

 - Do not put a space after if, for, while or function call statements;

 - The preferred maximum line length is 80 characters;

 - Use `/* */` for comments;

 - Use the API: there is a list of `a_*()` functions you should use instead
   of the standard libc ones. There is also a common API for linked lists,
   tabulars, etc.;

 - Be *clear* in what you do;

 - Prefix your function names with the module they are enhancing,
   i.e. if you add a function to manipulate a tag, prefix it with `tag_`;

 - Write documentation for any new functions, options, whatever.

A vim modeline is set in each file to respect this.

### Documentation of Lua files

For documentation purposes LDoc---see
[here](https://stevedonovan.github.io/ldoc/manual/doc.md.html) for its
documentation---is used. Comments that shall be parsed by LDoc have the
following format:

    --- summary.
    -- Description; this can extend over
    -- several lines

    -----------------
    -- This will also do.

    --[[--
     Summary. A description
     ...;
    ]]

You can use the full power of
[Markdown](http://daringfireball.net/projects/markdown) with the extensions of
[Discount](http://www.pell.portland.or.us/~orc/Code/discount/) for markup in
the comments.

Every module and class should have a short description at its beginning which
should include `@author author`, `@copyright year author` and
`@module module-name` or `@classmod class-name`.

Parameters of functions should be documented using
`@tparam <type> <parmname> <description>`, and return values via
`@treturn <type> <description>`.

For a more comprehensive description of the available tags see the [LDoc
documentation](https://stevedonovan.github.io/ldoc/manual/doc.md.html).

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
  `Signed-off-by: Your Name <you@example.com>` line to the
  commit message (or just use the option `-s` when committing);
- make sure that you have tests for the bug you are fixing;
- if possible, add a unit test to the test suite under spec/.

### Patches

Submitting patches via pull requests on the GitHub project is the preferred
method.

#### Pull request
- create a [pull request](https://github.com/awesomeWM/awesome/pulls)

