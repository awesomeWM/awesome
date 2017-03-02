---------------------------------------------------------------------------
--- Utility module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module awful.util
---------------------------------------------------------------------------

-- Grab environment we need
local os = os
local assert = assert
local load = loadstring or load -- luacheck: globals loadstring (compatibility with Lua 5.1)
local loadfile = loadfile
local debug = debug
local pairs = pairs
local type = type
local gtable = require("gears.table")
local string = string
local grect = require("gears.geometry").rectangle
local Gio = require("lgi").Gio
local gcolor = require("gears.color")
local capi =
{
    awesome = awesome,
    mouse = mouse
}
local gears_debug = require("gears.debug")
local gmath = require("gears.math")

local util = {}
util.table = {}

--- The default shell used when spawing processes.
util.shell = os.getenv("SHELL") or "/bin/sh"

local displayed_deprecations = {}
--- Display a deprecation notice, but only once per traceback.
-- @param[opt] see The message to a new method / function to use.
-- @tparam table args Extra arguments
-- @tparam boolean args.raw Print the message as-is without the automatic context
-- @tparam integer args.deprecated_in Print the message only when Awesome's
--   version is equal to or greater than deprecated_in.
function util.deprecate(see, args)
    args = args or {}
    if args.deprecated_in then
        local dep_ver = "v" .. tostring(args.deprecated_in)
        if awesome.version < dep_ver then
            return
        end
    end
    local tb = debug.traceback()
    if displayed_deprecations[tb] then
        return
    end
    displayed_deprecations[tb] = true

    -- Get function name/desc from caller.
    local info = debug.getinfo(2, "n")
    local funcname = info.name or "?"
    local msg = "awful: function " .. funcname .. " is deprecated"
    if see then
        if args.raw then
            msg = see
        elseif string.sub(see, 1, 3) == 'Use' then
            msg = msg .. ". " .. see
        else
            msg = msg .. ", see " .. see
        end
    end
    gears_debug.print_warning(msg .. ".\n" .. tb)
end

--- Create a class proxy with deprecation messages.
-- This is useful when a class has moved somewhere else.
-- @tparam table fallback The new class
-- @tparam string old_name The old class name
-- @tparam string new_name The new class name
-- @treturn table A proxy class.
function util.deprecate_class(fallback, old_name, new_name)
    local message = old_name.." has been renamed to "..new_name

    local function call(_,...)
        util.deprecate(message, {raw = true})

        return fallback(...)
    end

    local function index(_, k)
        util.deprecate(message, {raw = true})

        return fallback[k]
    end

    local function newindex(_, k, v)
        util.deprecate(message, {raw = true})

        fallback[k] = v
    end

    return setmetatable({}, {__call = call, __index = index, __newindex  = newindex})
end

--- Get a valid color for Pango markup
-- @deprecated ensure_pango_color
-- @param color The color.
-- @tparam string fallback The color to return if the first is invalid. (default: black)
-- @treturn string color if it is valid, else fallback.
-- @see gears.color
function util.ensure_pango_color(color, fallback)
    util.deprecate("gears.color.ensure_pango_color")

    return gcolor.ensure_pango_color(color, fallback)
end

--- Make i cycle.
-- @deprecated util.cycle
-- @param t A length. Must be greater than zero.
-- @param i An absolute index to fit into #t.
-- @return An integer in (1, t) or nil if t is less than or equal to zero.
-- @see gears.math
function util.cycle(t, i)
    util.deprecate("gears.math.cycle", {deprecated_in=5})

    return gmath.cycle(t, i)
end

--- Create a directory
-- @param dir The directory.
-- @return mkdir return code
function util.mkdir(dir)
    return os.execute("mkdir -p " .. dir)
end

--- Eval Lua code.
-- @return The return value of Lua code.
function util.eval(s)
    return assert(load(s))()
end

local xml_entity_names = { ["'"] = "&apos;", ["\""] = "&quot;", ["<"] = "&lt;", [">"] = "&gt;", ["&"] = "&amp;" };
--- Escape a string from XML char.
-- Useful to set raw text in textbox.
-- @param text Text to escape.
-- @return Escape text.
function util.escape(text)
    return text and text:gsub("['&<>\"]", xml_entity_names) or nil
end

local xml_entity_chars = { lt = "<", gt = ">", nbsp = " ", quot = "\"", apos = "'", ndash = "-", mdash = "-",
    amp = "&" };
--- Unescape a string from entities.
-- @param text Text to unescape.
-- @return Unescaped text.
function util.unescape(text)
    return text and text:gsub("&(%a+);", xml_entity_chars) or nil
end

--- Check if a file is a Lua valid file.
-- This is done by loading the content and compiling it with loadfile().
-- @param path The file path.
-- @return A function if everything is alright, a string with the error
-- otherwise.
function util.checkfile(path)
    local f, e = loadfile(path)
    -- Return function if function, otherwise return error.
    if f then return f end
    return e
end

--- Try to restart awesome.
-- It checks if the configuration file is valid, and then restart if it's ok.
-- If it's not ok, the error will be returned.
-- @return Never return if awesome restart, or return a string error.
function util.restart()
    local c = util.checkfile(capi.awesome.conffile)

    if type(c) ~= "function" then
        return c
    end

    capi.awesome.restart()
end

--- Get the config home according to the XDG basedir specification.
-- @return the config home (XDG_CONFIG_HOME) with a slash at the end.
function util.get_xdg_config_home()
    return (os.getenv("XDG_CONFIG_HOME") or os.getenv("HOME") .. "/.config") .. "/"
end

--- Get the cache home according to the XDG basedir specification.
-- @return the cache home (XDG_CACHE_HOME) with a slash at the end.
function util.get_xdg_cache_home()
    return (os.getenv("XDG_CACHE_HOME") or os.getenv("HOME") .. "/.cache") .. "/"
end

--- Get the path to the user's config dir.
-- This is the directory containing the configuration file ("rc.lua").
-- @return A string with the requested path with a slash at the end.
function util.get_configuration_dir()
    return capi.awesome.conffile:match(".*/") or "./"
end

--- Get the path to a directory that should be used for caching data.
-- @return A string with the requested path with a slash at the end.
function util.get_cache_dir()
    return util.get_xdg_cache_home() .. "awesome/"
end

--- Get the path to the directory where themes are installed.
-- @return A string with the requested path with a slash at the end.
function util.get_themes_dir()
    return (os.getenv('AWESOME_THEMES_PATH') or awesome.themes_path) .. "/"
end

--- Get the path to the directory where our icons are installed.
-- @return A string with the requested path with a slash at the end.
function util.get_awesome_icon_dir()
    return (os.getenv('AWESOME_ICON_PATH') or awesome.icon_path) .. "/"
end

--- Get the user's config or cache dir.
-- It first checks XDG_CONFIG_HOME / XDG_CACHE_HOME, but then goes with the
-- default paths.
-- @param d The directory to get (either "config" or "cache").
-- @return A string containing the requested path.
function util.getdir(d)
    if d == "config" then
        -- No idea why this is what is returned, I recommend everyone to use
        -- get_configuration_dir() instead
        return util.get_xdg_config_home() .. "awesome/"
    elseif d == "cache" then
        return util.get_cache_dir()
    end
end

--- Search for an icon and return the full path.
-- It searches for the icon path under the given directories with respect to the
-- given extensions for the icon filename.
-- @param iconname The name of the icon to search for.
-- @param exts Table of image extensions allowed, otherwise { 'png', gif' }
-- @param dirs Table of dirs to search, otherwise { '/usr/share/pixmaps/' }
-- @tparam[opt] string size The size. If this is specified, subdirectories `x`
--   of the dirs are searched first.
function util.geticonpath(iconname, exts, dirs, size)
    exts = exts or { 'png', 'gif' }
    dirs = dirs or { '/usr/share/pixmaps/', '/usr/share/icons/hicolor/' }
    local icontypes = { 'apps', 'actions',  'categories',  'emblems',
        'mimetypes',  'status', 'devices', 'extras', 'places', 'stock' }
    for _, d in pairs(dirs) do
        local icon
        for _, e in pairs(exts) do
            icon = d .. iconname .. '.' .. e
            if util.file_readable(icon) then
                return icon
            end
            if size then
                for _, t in pairs(icontypes) do
                    icon = string.format("%s%ux%u/%s/%s.%s", d, size, size, t, iconname, e)
                    if util.file_readable(icon) then
                        return icon
                    end
                end
            end
        end
    end
end

--- Check if a file exists, is not readable and not a directory.
-- @param filename The file path.
-- @return True if file exists and is readable.
function util.file_readable(filename)
    local gfile = Gio.File.new_for_path(filename)
    local gfileinfo = gfile:query_info("standard::type,access::can-read",
                                       Gio.FileQueryInfoFlags.NONE)
    return gfileinfo and gfileinfo:get_file_type() ~= "DIRECTORY" and
        gfileinfo:get_attribute_boolean("access::can-read")
end

--- Check if a path exists, is readable and is a directory.
-- @tparam string path The directory path.
-- @treturn boolean True if dir exists and is readable.
function util.dir_readable(path)
    local gfile = Gio.File.new_for_path(path)
    local gfileinfo = gfile:query_info("standard::type,access::can-read",
                                       Gio.FileQueryInfoFlags.NONE)
    return gfileinfo and gfileinfo:get_file_type() == "DIRECTORY" and
        gfileinfo:get_attribute_boolean("access::can-read")
end

--- Check if a path is a directory.
-- @tparam string path
-- @treturn bool True if path exists and is a directory.
function util.is_dir(path)
    return Gio.File.new_for_path(path):query_file_type({}) == "DIRECTORY"
end

--- Return all subsets of a specific set.
-- This function, giving a set, will return all subset it.
-- For example, if we consider a set with value { 10, 15, 34 },
-- it will return a table containing 2^n set:
-- { }, { 10 }, { 15 }, { 34 }, { 10, 15 }, { 10, 34 }, etc.
-- @deprecated util.subsets
-- @param set A set.
-- @return A table with all subset.
-- @see gears.math
function util.subsets(set)
    util.deprecate("gears.math.subsets", {deprecated_in=5})

    return gmath.subsets(set)
end

--- Get the nearest rectangle in the given direction. Every rectangle is specified as a table
-- with 'x', 'y', 'width', 'height' keys, the same as client or screen geometries.
-- @deprecated awful.util.get_rectangle_in_direction
-- @param dir The direction, can be either "up", "down", "left" or "right".
-- @param recttbl A table of rectangle specifications.
-- @param cur The current rectangle.
-- @return The index for the rectangle in recttbl closer to cur in the given direction. nil if none found.
-- @see gears.geometry
function util.get_rectangle_in_direction(dir, recttbl, cur)
    util.deprecate("gears.geometry.rectangle.get_in_direction")

    return grect.get_in_direction(dir, recttbl, cur)
end

--- Join all tables given as parameters.
-- This will iterate all tables and insert all their keys into a new table.
-- @deprecated util.table.join
-- @param args A list of tables to join
-- @return A new table containing all keys from the arguments.
-- @see gears.table
function util.table.join(...)
    util.deprecate("gears.table.join", {deprecated_in=5})

    return gtable.join(...)
end

--- Override elements in the first table by the one in the second.
--
-- Note that this method doesn't copy entries found in `__index`.
-- @deprecated util.table.crush
-- @tparam table t the table to be overriden
-- @tparam table set the table used to override members of `t`
-- @tparam[opt=false] boolean raw Use rawset (avoid the metatable)
-- @treturn table t (for convenience)
-- @see gears.table
function util.table.crush(t, set, raw)
    util.deprecate("gears.table.crush", {deprecated_in=5})

    return gtable.crush(t, set, raw)
end

--- Pack all elements with an integer key into a new table
-- While both lua and luajit implement __len over sparse
-- table, the standard define it as an implementation
-- detail.
--
-- This function remove any non numeric keys from the value set
--
-- @deprecated util.table.from_sparse
-- @tparam table t A potentially sparse table
-- @treturn table A packed table with all numeric keys
-- @see gears.table
function util.table.from_sparse(t)
    util.deprecate("gears.table.from_sparse", {deprecated_in=5})

    return gtable.from_sparse(t)
end

--- Check if a table has an item and return its key.
-- @deprecated util.table.hasitem
-- @param t The table.
-- @param item The item to look for in values of the table.
-- @return The key were the item is found, or nil if not found.
-- @see gears.table
function util.table.hasitem(t, item)
    util.deprecate("gears.table.hasitem", {deprecated_in=5})

    return gtable.hasitem(t, item)
end

--- Split a string into multiple lines
-- @param text String to wrap.
-- @param width Maximum length of each line. Default: 72.
-- @param indent Number of spaces added before each wrapped line. Default: 0.
-- @return The string with lines wrapped to width.
function util.linewrap(text, width, indent)
    text = text or ""
    width = width or 72
    indent = indent or 0

    local pos = 1
    return text:gsub("(%s+)()(%S+)()",
        function(_, st, word, fi)
            if fi - pos > width then
                pos = st
                return "\n" .. string.rep(" ", indent) .. word
            end
        end)
end

--- Count number of lines in a string
-- @tparam string text Input string.
-- @treturn int Number of lines.
function util.linecount(text)
    return select(2, text:gsub('\n', '\n')) + 1
end

--- Get a sorted table with all integer keys from a table
-- @deprecated util.table.keys
-- @param t the table for which the keys to get
-- @return A table with keys
-- @see gears.table
function util.table.keys(t)
    util.deprecate("gears.table.keys", {deprecated_in=5})

    return gtable.keys(t)
end

--- Filter a tables keys for certain content types
-- @deprecated util.table.keys_filter
-- @param t The table to retrieve the keys for
-- @param ... the types to look for
-- @return A filtered table with keys
-- @see gears.table
function util.table.keys_filter(t, ...)
    util.deprecate("gears.table.keys_filter", {deprecated_in=5})

    return gtable.keys_filter(t, ...)
end

--- Reverse a table
-- @deprecated util.table.reverse
-- @param t the table to reverse
-- @return the reversed table
-- @see gears.table
function util.table.reverse(t)
    util.deprecate("gears.table.reverse", {deprecated_in=5})

    return gtable.reverse(t)
end

--- Clone a table
-- @deprecated util.table.clone
-- @param t the table to clone
-- @param deep Create a deep clone? (default: true)
-- @return a clone of t
-- @see gears.table
function util.table.clone(t, deep)
    util.deprecate("gears.table.clone", {deprecated_in=5})

    return gtable.clone(t, deep)
end

---
-- Returns an iterator to cycle through, starting from the first element or the
-- given index, all elements of a table that match a given criteria.
--
-- @deprecated util.table.iterate
-- @param t      the table to iterate
-- @param filter a function that returns true to indicate a positive match
-- @param start  what index to start iterating from.  Default is 1 (=> start of
-- the table)
-- @see gears.table
function util.table.iterate(t, filter, start)
    util.deprecate("gears.table.iterate", {deprecated_in=5})

    return gtable.iterate(t, filter, start)
end


--- Merge items from the one table to another one
-- @deprecated util.table.merge
-- @tparam table t the container table
-- @tparam table set the mixin table
-- @treturn table Return `t` for convenience
-- @see gears.table
function util.table.merge(t, set)
    util.deprecate("gears.table.merge", {deprecated_in=5})

    return gtable.merge(t, set)
end


-- Escape all special pattern-matching characters so that lua interprets them
-- literally instead of as a character class.
-- Source: http://stackoverflow.com/a/20778724/15690
function util.quote_pattern(s)
    -- All special characters escaped in a string: %%, %^, %$, ...
    local patternchars = '['..("%^$().[]*+-?"):gsub("(.)", "%%%1")..']'
    return string.gsub(s, patternchars, "%%%1")
end

-- Generate a pattern matching expression that ignores case.
-- @param s Original pattern matching expression.
function util.query_to_pattern(q)
    local s = util.quote_pattern(q)
    -- Poor man's case-insensitive character matching.
    s = string.gsub(s, "%a",
                    function (c)
                        return string.format("[%s%s]", string.lower(c),
                                             string.upper(c))
                    end)
    return s
end

--- Round a number to an integer.
-- @deprecated util.round
-- @tparam number x
-- @treturn integer
-- @see gears.math
function util.round(x)
    util.deprecate("gears.math.round", {deprecated_in=5})

    return gmath.round(x)
end

return util

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
