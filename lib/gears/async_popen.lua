---------------------------------------------------------------------------
-- @release @AWESOME_VERSION@
-- @module gears.async_popen
---------------------------------------------------------------------------

local setmetatable = setmetatable
local util = require("awful.util")

local async_popen = { mt = {} }

-- Maps temporary filenames to callbacks
async_popen.callbacks = {}

--- Run a command asynchronously.
-- @param command The command to run
-- @param callback The function to be called when the command exits. The output of the command will be passed to it.
function async_popen.async_popen(command, callback)
    local tmpfname = os.tmpname()
    async_popen.callbacks[tmpfname] = callback
    util.spawn(string.format("bash -c '%s > %s; echo \"async_popen.process_output(\\\"%s\\\")\" | awesome-client' 2> /dev/null",
                             string.gsub(command, "'", "'\\''"),
                             tmpfname,
                             tmpfname),
               false)
end

function async_popen.process_output(tmpfname)
    if async_popen.callbacks[tmpfname] then
        local fh = io.open(tmpfname, "r")
        async_popen.callbacks[tmpfname](fh:read("*all"))
        fh:close()
        os.remove(tmpfname)
    end
end

function async_popen.mt:__call(...)
    return async_popen.async_popen(...)
end

return setmetatable(async_popen, async_popen.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
