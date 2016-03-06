---------------------------------------------------------------------------
--- Utility module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.util
---------------------------------------------------------------------------

-- Grab environment we need
local os = os
local io = io
local assert = assert
local load = loadstring or load -- luacheck: globals loadstring (compatibility with Lua 5.1)
local loadfile = loadfile
local debug = debug
local pairs = pairs
local ipairs = ipairs
local type = type
local rtable = table
local string = string
local lgi = require("lgi")
local Gio = require("lgi").Gio
local Pango = lgi.Pango
local capi =
{
    awesome = awesome,
    mouse = mouse
}
local gears_debug = require("gears.debug")
local floor = math.floor

local util = {}
util.table = {}

util.shell = os.getenv("SHELL") or "/bin/sh"

local displayed_deprecations = {}
--- Display a deprecation notice, but only once per traceback.
-- @param[opt] see The message to a new method / function to use.
function util.deprecate(see)
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
        msg = msg .. ", see " .. see
    end
    gears_debug.print_warning(msg .. ".\n" .. tb)
end

--- Get a valid color for Pango markup
-- @param color The color.
-- @tparam string fallback The color to return if the first is invalid. (default: black)
-- @treturn string color if it is valid, else fallback.
function util.ensure_pango_color(color, fallback)
    color = tostring(color)
    return Pango.Color.parse(Pango.Color(), color) and color or fallback or "black"
end

--- Make i cycle.
-- @param t A length. Must be greater than zero.
-- @param i An absolute index to fit into #t.
-- @return An integer in (1, t) or nil if t is less than or equal to zero.
function util.cycle(t, i)
    if t < 1 then return end
    i = i % t
    if i == 0 then
        i = t
    end
    return i
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

local xml_entity_chars = { lt = "<", gt = ">", nbsp = " ", quot = "\"", apos = "'", ndash = "-", mdash = "-", amp = "&" };
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
    return "@AWESOME_THEMES_PATH@" .. "/"
end

--- Get the path to the directory where our icons are installed.
-- @return A string with the requested path with a slash at the end.
function util.get_awesome_icon_dir()
    return "@AWESOME_ICON_PATH@" .. "/"
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
    local file = io.open(filename)
    if file then
        local _, _, code = file:read(1)
        io.close(file)
        if code == 21 then
            -- "Is a directory".
            return false
        end
        return true
    end
    return false
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
    local file = io.open(path)
    if file then
        if not file:read(0) -- Not a regular file (might include empty ones).
            and file:seek("end") ~= 0 then  -- And not a file with size 0.
            io.close(file)
            return true
        end
        io.close(file)
    end
    return false
end

local function subset_mask_apply(mask, set)
    local ret = {}
    for i = 1, #set do
        if mask[i] then
            rtable.insert(ret, set[i])
        end
    end
    return ret
end

local function subset_next(mask)
    local i = 1
    while i <= #mask and mask[i] do
        mask[i] = false
        i = i + 1
    end

    if i <= #mask then
        mask[i] = 1
        return true
    end
    return false
end

--- Return all subsets of a specific set.
-- This function, giving a set, will return all subset it.
-- For example, if we consider a set with value { 10, 15, 34 },
-- it will return a table containing 2^n set:
-- { }, { 10 }, { 15 }, { 34 }, { 10, 15 }, { 10, 34 }, etc.
-- @param set A set.
-- @return A table with all subset.
function util.subsets(set)
    local mask = {}
    local ret = {}
    for i = 1, #set do mask[i] = false end

    -- Insert the empty one
    rtable.insert(ret, {})

    while subset_next(mask) do
        rtable.insert(ret, subset_mask_apply(mask, set))
    end
    return ret
end

--- Return true whether rectangle B is in the right direction
-- compared to rectangle A.
-- @param dir The direction.
-- @param gA The geometric specification for rectangle A.
-- @param gB The geometric specification for rectangle B.
-- @return True if B is in the direction of A.
local function is_in_direction(dir, gA, gB)
    if dir == "up" then
        return gA.y > gB.y
    elseif dir == "down" then
        return gA.y < gB.y
    elseif dir == "left" then
        return gA.x > gB.x
    elseif dir == "right" then
        return gA.x < gB.x
    end
    return false
end

--- Calculate distance between two points.
-- i.e: if we want to move to the right, we will take the right border
-- of the currently focused screen and the left side of the checked screen.
-- @param dir The direction.
-- @param _gA The first rectangle.
-- @param _gB The second rectangle.
-- @return The distance between the screens.
local function calculate_distance(dir, _gA, _gB)
    local gAx = _gA.x
    local gAy = _gA.y
    local gBx = _gB.x
    local gBy = _gB.y

    if dir == "up" then
        gBy = _gB.y + _gB.height
    elseif dir == "down" then
        gAy = _gA.y + _gA.height
    elseif dir == "left" then
        gBx = _gB.x + _gB.width
    elseif dir == "right" then
        gAx = _gA.x + _gA.width
    end

    return math.sqrt(math.pow(gBx - gAx, 2) + math.pow(gBy - gAy, 2))
end

--- Get the nearest rectangle in the given direction. Every rectangle is specified as a table
-- with 'x', 'y', 'width', 'height' keys, the same as client or screen geometries.
-- @param dir The direction, can be either "up", "down", "left" or "right".
-- @param recttbl A table of rectangle specifications.
-- @param cur The current rectangle.
-- @return The index for the rectangle in recttbl closer to cur in the given direction. nil if none found.
function util.get_rectangle_in_direction(dir, recttbl, cur)
    local dist, dist_min
    local target = nil

    -- We check each object
    for i, rect in pairs(recttbl) do
        -- Check geometry to see if object is located in the right direction.
        if is_in_direction(dir, cur, rect) then
            -- Calculate distance between current and checked object.
            dist = calculate_distance(dir, cur, rect)

            -- If distance is shorter then keep the object.
            if not target or dist < dist_min then
                target = i
                dist_min = dist
            end
        end
    end
    return target
end

--- Join all tables given as parameters.
-- This will iterate all tables and insert all their keys into a new table.
-- @param args A list of tables to join
-- @return A new table containing all keys from the arguments.
function util.table.join(...)
    local ret = {}
    for _, t in pairs({...}) do
        if t then
            for k, v in pairs(t) do
                if type(k) == "number" then
                    rtable.insert(ret, v)
                else
                    ret[k] = v
                end
            end
        end
    end
    return ret
end

--- Override elements in the first table by the one in the second
-- @tparam table t the table to be overriden
-- @tparam table set the table used to override members of `t`
-- @treturn table t (for convenience)
function util.table.crush(t, set)
    for k, v in pairs(set) do
        t[k] = v
    end

    return t
end

--- Pack all elements with an integer key into a new table
-- While both lua and luajit implement __len over sparse
-- table, the standard define it as an implementation
-- detail.
--
-- This function remove any non numeric keys from the value set
--
-- @tparam table t A potentially sparse table
-- @treturn table A packed table with all numeric keys
function util.table.from_sparse(t)
    local keys= {}
    for k in pairs(t) do
        if type(k) == "number" then
            keys[#keys+1] = k
        end
    end

    table.sort(keys)

    local ret = {}
    for _,v in ipairs(keys) do
        ret[#ret+1] = t[v]
    end

    return ret
end

--- Check if a table has an item and return its key.
-- @param t The table.
-- @param item The item to look for in values of the table.
-- @return The key were the item is found, or nil if not found.
function util.table.hasitem(t, item)
    for k, v in pairs(t) do
        if v == item then
            return k
        end
    end
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
-- @param t the table for which the keys to get
-- @return A table with keys
function util.table.keys(t)
    local keys = { }
    for k, _ in pairs(t) do
        rtable.insert(keys, k)
    end
    rtable.sort(keys, function (a, b)
        return type(a) == type(b) and a < b or false
    end)
    return keys
end

--- Filter a tables keys for certain content types
-- @param t The table to retrieve the keys for
-- @param ... the types to look for
-- @return A filtered table with keys
function util.table.keys_filter(t, ...)
    local keys = util.table.keys(t)
    local keys_filtered = { }
    for _, k in pairs(keys) do
        for _, et in pairs({...}) do
            if type(t[k]) == et then
                rtable.insert(keys_filtered, k)
                break
            end
        end
    end
    return keys_filtered
end

--- Reverse a table
-- @param t the table to reverse
-- @return the reversed table
function util.table.reverse(t)
    local tr = { }
    -- reverse all elements with integer keys
    for _, v in ipairs(t) do
        rtable.insert(tr, 1, v)
    end
    -- add the remaining elements
    for k, v in pairs(t) do
        if type(k) ~= "number" then
            tr[k] = v
        end
    end
    return tr
end

--- Clone a table
-- @param t the table to clone
-- @param deep Create a deep clone? (default: true)
-- @return a clone of t
function util.table.clone(t, deep)
    deep = deep == nil and true or deep
    local c = { }
    for k, v in pairs(t) do
        if deep and type(v) == "table" then
            c[k] = util.table.clone(v)
        else
            c[k] = v
        end
    end
    return c
end

---
-- Returns an iterator to cycle through, starting from the first element or the
-- given index, all elements of a table that match a given criteria.
--
-- @param t      the table to iterate
-- @param filter a function that returns true to indicate a positive match
-- @param start  what index to start iterating from.  Default is 1 (=> start of
-- the table)
function util.table.iterate(t, filter, start)
    local count  = 0
    local index  = start or 1
    local length = #t

    return function ()
        while count < length do
            local item = t[index]
            index = util.cycle(#t, index + 1)
            count = count + 1
            if filter(item) then return item end
        end
    end
end


--- Merge items from the one table to another one
-- @tparam table t the container table
-- @tparam table set the mixin table
-- @treturn table Return `t` for convenience
function util.table.merge(t, set)
    for _, v in ipairs(set) do
        table.insert(t, v)
    end
    return t
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
-- @tparam number x
-- @treturn integer
function util.round(x)
    return floor(x + 0.5)
end

return util

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
