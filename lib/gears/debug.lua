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
    io.stderr:write(os.date("%Y-%m-%d %T W: ") .. tostring(message) .. "\n")
end

--- Print an error message
-- @tparam string message The error message to print
function debug.print_error(message)
    io.stderr:write(os.date("%Y-%m-%d %T E: ") .. tostring(message) .. "\n")
end

return debug

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
