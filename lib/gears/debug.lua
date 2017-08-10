---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @module gears.debug
---------------------------------------------------------------------------

local tostring = tostring
local print = print
local type = type
local pairs = pairs

local debug = {}

--- Given a table (or any other data) return a string that contains its
-- tag, value and type. If data is a table then recursively call `dump_raw`
-- on each of its values.
-- @param data Value to inspect.
-- @param shift Spaces to indent lines with.
-- @param tag The name of the value.
-- @tparam[opt=10] int depth Depth of recursion.
-- @return a string which contains tag, value, value type and table key/value
-- pairs if data is a table.
local function dump_raw(data, shift, tag, depth)
    depth = depth == nil and 10 or depth or 0
    local result = ""

    if tag then
        result = result .. tostring(tag) .. " : "
    end

    if type(data) == "table" and depth > 0 then
        shift = (shift or "") .. "  "
        result = result .. tostring(data)
        for k, v in pairs(data) do
            result = result .. "\n" .. shift .. dump_raw(v, shift, k, depth - 1)
        end
    else
        result = result .. tostring(data) .. " (" .. type(data) .. ")"
        if depth == 0 and type(data) == "table" then
            result = result .. " […]"
        end
    end

    return result
end

--- Inspect the value in data.
-- @param data Value to inspect.
-- @param tag The name of the value.
-- @tparam[opt] int depth Depth of recursion.
-- @return string A string that contains the expanded value of data.
function debug.dump_return(data, tag, depth)
    return dump_raw(data, nil, tag, depth)
end

--- Print the table (or any other value) to the console.
-- @param data Table to print.
-- @param tag The name of the table.
-- @tparam[opt] int depth Depth of recursion.
function debug.dump(data, tag, depth)
    print(debug.dump_return(data, tag, depth))
end

--- Print an warning message
-- @tparam string message The warning message to print
function debug.print_warning(message)
    io.stderr:write(os.date("%Y-%m-%d %T W: awesome: ") .. tostring(message) .. "\n")
end

--- Print an error message
-- @tparam string message The error message to print
function debug.print_error(message)
    io.stderr:write(os.date("%Y-%m-%d %T E: awesome: ") .. tostring(message) .. "\n")
end

local displayed_deprecations = {}
--- Display a deprecation notice, but only once per traceback.
--
-- This function also emits the `debug::deprecate` signal on the `awesome`
-- global object.
--
-- @param[opt] see The message to a new method / function to use.
-- @tparam table args Extra arguments
-- @tparam boolean args.raw Print the message as-is without the automatic context
-- @tparam integer args.deprecated_in Print the message only when Awesome's
--   version is equal to or greater than deprecated_in.
function debug.deprecate(see, args)
    args = args or {}
    if args.deprecated_in then
        local dep_ver = "v" .. tostring(args.deprecated_in)
        if awesome.version < dep_ver then
            return
        end
    end
    local tb = _G.debug.traceback()
    if displayed_deprecations[tb] then
        return
    end
    displayed_deprecations[tb] = true

    -- Get function name/desc from caller.
    local info = _G.debug.getinfo(2, "n")
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
    debug.print_warning(msg .. ".\n" .. tb)

    if awesome then
        awesome.emit_signal("debug::deprecation", msg, see, args)
    end
end

--- Create a class proxy with deprecation messages.
-- This is useful when a class has moved somewhere else.
-- @tparam table fallback The new class
-- @tparam string old_name The old class name
-- @tparam string new_name The new class name
-- @treturn table A proxy class.
function debug.deprecate_class(fallback, old_name, new_name)
    local message = old_name.." has been renamed to "..new_name

    local function call(_,...)
        debug.deprecate(message, {raw = true})

        return fallback(...)
    end

    local function index(_, k)
        debug.deprecate(message, {raw = true})

        return fallback[k]
    end

    local function newindex(_, k, v)
        debug.deprecate(message, {raw = true})

        fallback[k] = v
    end

    return setmetatable({}, {__call = call, __index = index, __newindex  = newindex})
end

return debug

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
