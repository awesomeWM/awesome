---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local os = os
local io = io
local assert = assert
local load = loadstring or load -- v5.1 - loadstring, v5.2 - load
local loadfile = loadfile
local debug = debug
local pairs = pairs
local ipairs = ipairs
local type = type
local rtable = table
local pairs = pairs
local string = string
local capi =
{
    awesome = awesome,
    mouse = mouse
}

--- Utility module for awful
-- awful.util
local util = {}
util.table = {}

util.shell = os.getenv("SHELL") or "/bin/sh"

function util.deprecate(see)
    io.stderr:write("W: awful: function is deprecated")
    if see then
        io.stderr:write(", see " .. see)
    end
    io.stderr:write("\n")
    io.stderr:write(debug.traceback())
end

--- Strip alpha part of color.
-- @param color The color.
-- @return The color without alpha channel.
function util.color_strip_alpha(color)
    if color:len() == 9 then
        color = color:sub(1, 7)
    end
    return color
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

--- Spawn a program.
-- @param cmd The command.
-- @param sn Enable startup-notification.
-- @return The forked PID or an error message
-- @return The startup notification UID, if the spawn was successful
function util.spawn(cmd, sn)
    if cmd and cmd ~= "" then
        if sn == nil then sn = true end
        return capi.awesome.spawn(cmd, sn)
    end
end

--- Spawn a program using the shell.
-- @param cmd The command.
function util.spawn_with_shell(cmd)
    if cmd and cmd ~= "" then
        cmd = { util.shell, "-c", cmd }
        return capi.awesome.spawn(cmd, false)
    end
end

--- Read a program output and returns its output as a string.
-- @param cmd The command to run.
-- @return A string with the program output, or the error if one occured.
function util.pread(cmd)
    if cmd and cmd ~= "" then
        local f, err = io.popen(cmd, 'r')
        if f then
            local s = f:read("*all")
            f:close()
            return s
        else
            return err
        end
    end
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

--- Get the user's config or cache dir.
-- It first checks XDG_CONFIG_HOME / XDG_CACHE_HOME, but then goes with the
-- default paths.
-- @param d The directory to get (either "config" or "cache").
-- @return A string containing the requested path.
function util.getdir(d)
    if d == "config" then
        local dir = os.getenv("XDG_CONFIG_HOME")
        if dir then
            return dir .. "/awesome"
        end
        return os.getenv("HOME") .. "/.config/awesome"
    elseif d == "cache" then
        local dir = os.getenv("XDG_CACHE_HOME")
        if dir then
            return dir .. "/awesome"
        end
        return os.getenv("HOME").."/.cache/awesome"
    end
end

--- Search for an icon and return the full path.
-- It searches for the icon path under the directories given w/the right ext
-- @param iconname The name of the icon to search for.
-- @param exts Table of image extensions allowed, otherwise { 'png', gif' }
-- @param dirs Table of dirs to search, otherwise { '/usr/share/pixmaps/' }
-- @param size Optional size. If this is specified, subdirectories <size>x<size>
--             of the dirs are searched first
function util.geticonpath(iconname, exts, dirs, size)
    exts = exts or { 'png', 'gif' }
    dirs = dirs or { '/usr/share/pixmaps/' }
    for _, d in pairs(dirs) do
        for _, e in pairs(exts) do
            local icon
            if size then
                icon = string.format("%s%ux%u/%s.%s",
                       d, size, size, iconname, e)
                if util.file_readable(icon) then
                    return icon
                end
            end
            icon = d .. iconname .. '.' .. e
            if util.file_readable(icon) then
                return icon
            end
        end
    end
end

--- Check if file exists and is readable.
-- @param filename The file path
-- @return True if file exists and readable.
function util.file_readable(filename)
    local file = io.open(filename)
    if file then
        io.close(file)
        return true
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

-- Return true whether rectangle B is in the right direction
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

-- Calculate distance between two points.
-- i.e: if we want to move to the right, we will take the right border
-- of the currently focused screen and the left side of the checked screen.
-- @param dir The direction.
-- @param gA The first rectangle.
-- @param gB The second rectangle.
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

-- Get the nearest rectangle in the given direction. Every rectangle is specified as a table
-- with 'x', 'y', 'width', 'height' keys, the same as client or screen geometries.
-- @param dir The direction, can be either "up", "down", "left" or "right".
-- @param recttbl A table of rectangle specifications.
-- @param cur The current rectangle.
-- @return The index for the rectangle in recttbl closer to cur in the given direction. nil if none found.
function util.get_rectangle_in_direction(dir, recttbl, cur)
    local dist, dist_min
    local target = nil

    -- We check each object
    for i, rect in ipairs(recttbl) do
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
    for i, t in pairs({...}) do
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
    local text = text or ""
    local width = width or 72
    local indent = indent or 0

    local pos = 1
    return text:gsub("(%s+)()(%S+)()",
        function(sp, st, word, fi)
            if fi - pos > width then
                pos = st
                return "\n" .. string.rep(" ", indent) .. word
            end
        end)
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
    local deep = deep == nil and true or deep
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

return util

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
