---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2008 Julien Danjou
-- @copyright 2014 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local capi =
{
    awesome = awesome,
    mouse = mouse,
    client = client,
}
local lgi = require("lgi")
local Gio = lgi.Gio
local GLib = lgi.GLib
local util   = require("awful.util")

local spawn = {}


spawn.snid_buffer = {}

function spawn.on_snid_callback(c)
    local props = spawn.snid_buffer[c.startup_id]
    if props then
        c:emit_signal("spawn::completed_with_payload", props)
        spawn.snid_buffer[c.startup_id] = nil
    end
end


function spawn.on_snid_cancel(id)
    if spawn.snid_buffer[id] then
        spawn.snid_buffer[id] = nil
    end
end

--- Spawn a program.
-- See awful.rules.execute for more details
-- @param cmd The command.
-- @param sn_rules a property table, false to disable startup-notification.
-- @param callback A callback function (if the client support startup notifications)
-- @return The forked PID or an error message
-- @return The startup notification UID, if the spawn was successful
function spawn.spawn(cmd, sn_rules, callback)
    if cmd and cmd ~= "" then
        local enable_sn = (sn_rules ~= false or callback)
        if not sn_rules and callback then
            sn_rules = {callback=callback}
        elseif callback then
            sn_rules.callback = callback
        end
        local pid, snid = capi.awesome.spawn(cmd, enable_sn)
        -- The snid will be nil in case of failure
        if snid and type(sn_rules) == "table" then
            spawn.snid_buffer[snid] = sn_rules
        end
        return pid, snid
    end
    -- For consistency
    return "Error: No command to execute"
end

--- Spawn a program using the shell.
-- @param cmd The command.
function spawn.with_shell(cmd)
    if cmd and cmd ~= "" then
        cmd = { util.shell, "-c", cmd }
        return capi.awesome.spawn(cmd, false)
    end
end

--- Spawn a program and asynchronously and capture its output line by line.
-- @tparam string|table cmd The command.
-- @tparam[opt] function stdout_callback Function that is called with each line of
--              output on stdout, e.g. `stdout_callback(line)`.
-- @tparam[opt] function stderr_callback Function that is called with each line of
--              output on stderr, e.g. `stderr_callback(line)`.
-- @tparam[opt] function done_callback Function to call when no more output is
--              produced.
-- @treturn[1] Integer the PID of the forked process.
-- @treturn[2] string Error message.
function spawn.with_line_callback(cmd, stdout_callback, stderr_callback, done_callback)
    local have_stdout, have_stderr = stdout_callback ~= nil, stderr_callback ~= nil
    local pid, sn_id, stdin, stdout, stderr = capi.awesome.spawn(cmd, false, false, have_stdout, have_stderr)
    if type(pid) == "string" then
        -- Error
        return pid
    end

    local done_before = false
    local function step_done()
        if have_stdout and have_stderr and not done_before then
            done_before = true
            return
        end
        done_callback()
    end
    if have_stdout then
        spawn.read_lines(Gio.UnixInputStream.new(stdout, true),
                stdout_callback, step_done, true)
    end
    if have_stderr then
        spawn.read_lines(Gio.UnixInputStream.new(stderr, true),
                stderr_callback, step_done, true)
    end
    assert(stdin == nil)
    return pid
end

--- Read lines from a Gio input stream
-- @tparam Gio.InputStream input_stream The input stream to read from.
-- @tparam function line_callback Function that is called with each line
--         read, e.g. `line_callback(line_from_stream)`.
-- @tparam[opt] function done_callback Function that is called when the
--              operation finishes (e.g. due to end of file).
-- @tparam[opt=false] boolean close Should the stream be closed after end-of-file?
function spawn.read_lines(input_stream, line_callback, done_callback, close)
    local stream = Gio.DataInputStream.new(input_stream)
    local function done()
        if close then
            stream:close()
        end
        if done_callback then
            xpcall(done_callback, function(err)
                print(debug.traceback("Error while calling done_callback:"
                    ..  tostring(err), 2))
            end)
        end
    end
    local start_read, finish_read
    start_read = function()
        stream:read_line_async(GLib.PRIORITY_DEFAULT, nil, finish_read)
    end
    finish_read = function(obj, res)
        local line, length = obj:read_line_finish(res)
        if type(length) ~= "number" then
            -- Error
            print("Error in awful.spawn.read_lines:", tostring(length))
            done()
        elseif #line ~= length then
            -- End of file
            done()
        else
            -- Read a line
            xpcall(function()
                -- This needs tostring() for older lgi versions which returned
                -- "GLib.Bytes" instead of Lua strings (I guess)
                line_callback(tostring(line))
            end, function(err)
                print(debug.traceback("Error while calling line_callback: "
                    .. tostring(err), 2))
            end)

            -- Read the next line
            start_read()
        end
    end
    start_read()
end

--- Read a program output and returns its output as a string.
-- @param cmd The command to run.
-- @return A string with the program output, or the error if one occured.
function spawn.pread(cmd)
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

capi.awesome.connect_signal("spawn::canceled" , spawn.on_snid_cancel   )
capi.awesome.connect_signal("spawn::timeout"  , spawn.on_snid_cancel   )
capi.client.connect_signal ("manage"          , spawn.on_snid_callback )

capi.client.add_signal    ("spawn::completed_with_payload"            )

return setmetatable(spawn, { __call = function(_, ...) return spawn.spawn(...) end })
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
