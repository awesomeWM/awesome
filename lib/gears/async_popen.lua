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
    util.spawn_with_shell(
        string.format("%s > %s; echo 'async_popen.process_output(\"%s\")' | awesome-client",
                      command,
                      tmpfname,
                      tmpfname))
end

function async_popen.process_output(tmpfname)
    if async_popen.callbacks[tmpfname] then
        local fh = io.open(tmpfname, "r")
        async_popen.callbacks[tmpfname](fh:read("*all"))
        fh:close()
        os.remove(tmpfname)
    end
end

--- Run a command synchronously and return its output. If the given timeout expires, kill the command and return nil.
-- @param command The command to run
-- @param timeout The maximum time the command will be run
-- @return If the command did not time out, return its output, otherwise return nil.
function async_popen.sync_popen_with_timeout(command, timeout)
    local fh =
        io.popen(string.format("timeout %d sh -c '(%s) || true'",
                               -- ^without the "|| true" if the given command returns 124 this function will think that it timed out
                               timeout,
                               string.gsub(command, "'", "'\\''")))
    local output = fh:read("*all")
    local _, _, rc = fh:close() -- exit status of timeout
    if rc ~= 124 then -- the command didn't time out
        return output
    end
end

function async_popen.mt:__call(...)
    return async_popen.async_popen(...)
end

return setmetatable(async_popen, async_popen.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
