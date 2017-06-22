---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
-- @module gears.require_luarocks
---------------------------------------------------------------------------

local require = require
local io = io

local require_luarocks = {}

local luarocks_set_up = false

--- Make sure that libraries installed through LuaRocks can be require()d.
-- This function does the equivalent of running `luarocks path` before starting
-- Lua. If LuaRocks is not installed, this might print an error on stderr.
function require_luarocks.add_luarocks_path()
    if luarocks_set_up then
        return
    end
    luarocks_set_up = true

    -- Run the given command and add its output to package[key]
    local function read_and_add(command, key)
        -- Using io.popen blocks awesome and is almost always a bad idea,
        -- because lgi offers better alternatives. Here, we might not have lgi
        -- yet and so we must resort to popen.
        local out = io.popen(command)
        if not out then
            return
        end
        -- Read output and remove trailing newline
        local result = out:read("*a"):sub(1, -2)
        out:close()
        if result ~= "" then
            package[key] = package[key] .. ";" .. result
        end
    end
    read_and_add("luarocks path --lr-path", "path")
    read_and_add("luarocks path --lr-cpath", "cpath")
end

--- Try to load a module and if that fails, try to load it through LuaRocks.
-- This function works just like Lua's normal `require` function, but calls
-- @{add_luarocks_path} when needed.
-- @tparam string module the name of the module to load.
function require_luarocks.require(module)
    if luarocks_set_up then
        return require(module)
    end
    local success, result = pcall(require, module)
    if success then
        return result
    end
    io.stderr:write("Failed to require(\"" .. tostring(module) .. "\"),"
        .. " checking if this module is available through LuaRocks\n")
    require_luarocks.add_luarocks_path()
    return require(module)
end

return require_luarocks

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
