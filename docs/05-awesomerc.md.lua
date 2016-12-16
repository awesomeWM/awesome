local filename, rcfile, new_rcfile = ...

local f = io.open(filename, "w")

f:write[[# Default configuration file documentation

This document explains the default `rc.lua` file provided by Awesome.

]]

-- Document sections of the file to guide new users to the right doc pages

local sections = {}

sections.DOC_REQUIRE_SECTION = [[
The Awesome API is distributed across many libraries (also called modules).

Here are the modules that we import:

<table class='widget_list' border=1>
  <tr><td>`gears`</td><td>Utilities such as color parsing and objects</td></tr>
  <tr><td>`wibox`</td><td>Awesome own generic widget framework</td></tr>
  <tr><td>`awful`</td><td>Everything related to window managment</td></tr>
  <tr><td>`naughty`</td><td>Notifications</td></tr>
  <tr><td>`menubar`</td><td>XDG (application) menu implementation</td></tr>
  <tr><td>`beautiful`</td><td>Awesome theme module</td></tr>
</table>

]]

sections.DOC_ERROR_HANDLING = [[
Awesome is a window managing framework. It allows its users great (ultimate?)
flexibility. However, it also allows the user to write invalid code. Here's a
non-exhaustive list of possible errors:

 * Syntax: There is an `awesome -k` option available in the command line to
   check the configuration file. Awesome cannot start with an invalid `rc.lua`
 * Invalid APIs and type errors: Lua is a dynamic language. It doesn't have much
   support for static/compile time checks. There is the `luacheck` utility to
   help find some categories of errors. Those errors will cause Awesome to
   "drop" the current call stack and start over. Note that if it cannot
   reach the end of the `rc.lua` without errors, it will fall back to the
   original file.
 * Invalid logic: It is possible to write fully valid code that will render
   Awesome unusable (like an infinite loop or blocking commands). In that case,
   the best way to debug this is either using `print()` or using `gdb`. For
   this, see the [Debugging tips Readme section](../documentation/01-readme.md.html)
 * Deprecated APIs: The Awesome API is not frozen for eternity. After a decade
   of development and recent changes to enforce consistency, it hasn't
   changed much. This doesn't mean it won't change in the future. Whenever
   possible, changes won't cause errors but will instead print a deprecation
   message in the Awesome logs. These logs are placed in various places
   depending on the distribution. By default, Awesome will print errors on
  `stderr` and `stdout`.


]]


sections.DOC_LOAD_THEME = [[
To create custom themes, the easiest way is to copy the `default` theme folder
from `/usr/share/awesome/themes/` into `~/.config/awesome` and modify it.

Awesome currently doesn't behave well without a theme containing all the "basic"
variables such as `bg_normal`. To get a list of all official variables, see
the [appearance guide](../documentation/06-appearance.md.html).
]]


sections.DOC_DEFAULT_APPLICATIONS = [[
 &nbsp;
]]


sections.DOC_LAYOUT = [[
 &nbsp;
]]


sections.DOC_MENU = [[
 &nbsp;
]]


sections.TAGLIST_BUTTON = [[
 &nbsp;
]]


sections.TASKLIST_BUTTON = [[
 &nbsp;
]]


sections.DOC_FOR_EACH_SCREEN = [[
 &nbsp;
]]


sections.DOC_WIBAR = [[
 &nbsp;
]]


sections.DOC_SETUP_WIDGETS = [[
 &nbsp;
]]


sections.DOC_ROOT_BUTTONS = [[
 &nbsp;
]]


sections.DOC_GLOBAL_KEYBINDINGS = [[
 &nbsp;
]]


sections.DOC_CLIENT_KEYBINDINGS = [[
 &nbsp;
]]


sections.DOC_NUMBER_KEYBINDINGS = [[
 &nbsp;
]]


sections.DOC_CLIENT_BUTTONS = [[
 &nbsp;
]]


sections.DOC_RULES = [[
 &nbsp;
]]


sections.DOC_GLOBAL_RULE = [[
 &nbsp;
]]


sections.DOC_FLOATING_RULE = [[
 &nbsp;
]]


sections.DOC_DIALOG_RULE = [[
 &nbsp;
]]


sections.DOC_MANAGE_HOOK = [[
 &nbsp;
]]


sections.DOC_TITLEBARS = [[
 &nbsp;
]]


sections.DOC_BORDER = [[
 &nbsp;
]]

-- Ask ldoc to generate links

local function add_links(line)
    for _, module in ipairs {
        "awful", "wibox", "gears", "naughty", "menubar", "beautiful"
    } do
        if line:match(module.."%.") then
            line = line:gsub("("..module.."[.a-zA-Z]+)", "`%1`")
        end
    end

    return "    "..line.."\n"
end

-- Parse the default awesomerc.lua

local rc = io.open(rcfile)

local doc_block = false

local output = {}

for line in rc:lines() do
    local tag = line:match("@([^@]+)@")

    if not tag then
        local section = line:match("--[ ]*{{{[ ]*(.*)")
        if line == "-- }}}" or line == "-- {{{" then
            -- Hide some noise
        elseif section then
            -- Turn Vim sections into markdown sections
            if doc_block then
                f:write("\n")
                doc_block = false
            end
            f:write("## "..section.."\n")
        elseif line:sub(1,2) == "--" then
            -- Display "top level" comments are normal text.
            if doc_block then
                f:write("\n")
                doc_block = false
            end
            f:write(line:sub(3).."\n")
        else
            -- Write the code in <code> sections
            if not doc_block then
                f:write("\n")
                doc_block = true
            end
            f:write(add_links(line))
        end
        table.insert(output, line)
    else
        -- Take the documentation found in this file and append it
        if doc_block then
            f:write("\n")
            doc_block = false
        end

        if sections[tag] then
            f:write(sections[tag])
        else
            f:write(" \n\n")
        end
    end
end

f:write("\n")

f:close()

local rc_lua = io.open(new_rcfile, "w")
rc_lua:write(table.concat(output, "\n"))
rc_lua:close()
