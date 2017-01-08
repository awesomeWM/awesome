---------------------------------------------------------------------------
--- Spawning of programs.
--
-- This module provides methods to start programs and supports startup
-- notifications, which allows for callbacks and applying properties to the
-- program after it has been launched.  This requires currently that the
-- applicaton supports them.
--
-- **Rules of thumb when a shell is needed**:
--
-- * A shell is required when the commands contain `&&`, `;`, `||`, `&` or
--  any other unix shell language syntax
-- * When shell variables are defined as part of the command
-- * When the command is a shell alias
--
-- Note that a shell is **not** a terminal emulator. A terminal emulator is
-- something like XTerm, Gnome-terminal or Konsole. A shell is something like
-- `bash`, `zsh`, `busybox sh` or `Debian ash`.
--
-- If you wish to open a process in a terminal window, check that your terminal
-- emulator supports the common `-e` option. If it does, then something like
-- this should work:
--
--    awful.spawn(terminal.." -e my_command")
--
-- Note that some terminals, such as rxvt-unicode (urxvt) support full commands
-- using quotes, while other terminal emulators require to use quoting.
--
-- **Understanding clients versus PID versus commands versus class**:
--
-- A *process* has a *PID* (process identifier). It can have 0, 1 or many
-- *window*s.
--
-- A *command* if what is used to start *process*(es). It has no direct relation
-- with *process*, *client* or *window*. When a command is executed, it will
-- usually start a *process* which keeps running until it exits. This however is
-- not always the case as some applications use scripts as command and others
-- use various single-instance mechanisms (usually client/server) and merge
-- with an existing process.
--
-- A *client* corresponds to a *window*. It is owned by a process. It can have
-- both a parent and one or many children. A *client* has a *class*, an
-- *instance*, a *role*, and a *type*. See `client.class`, `client.instance`,
-- `client.role` and `client.type` for more information about these properties.
--
-- **The startup notification protocol**:
--
-- The startup notification protocol is an optional specification implemented
-- by X11 applications to bridge the chain of knowledge between the moment a
-- program is launched to the moment its window (client) is shown. It can be
-- found [on the FreeDesktop.org website](https://www.freedesktop.org/wiki/Specifications/startup-notification-spec/).
--
-- Awesome has support for the various events that are part of the protocol, but
-- the most useful is the identifier, usually identified by its `SNID` acronym in
-- the documentation. It isn't usually necessary to even know it exists, as it
-- is all done automatically. However, if more control is required, the
-- identifier can be specified by an environment variable called
-- `DESKTOP_STARTUP_ID`. For example, let us consider execution of the following
-- command:
--
--    DESKTOP_STARTUP_ID="something_TIME$(date '+%s')" my_command
--
-- This should (if the program correctly implements the protocol) result in
-- `c.startup_id` to at least match `something`.
-- This identifier can then be used in `awful.rules` to configure the client.
--
-- Awesome can automatically set the `DESKTOP_STARTUP_ID` variable. This is used
-- by `awful.spawn` to specify additional rules for the startup. For example:
--
--    awful.spawn("urxvt -e maxima -name CALCULATOR", {
--        floating  = true,
--        tag       = mouse.screen.selected_tag,
--        placement = awful.placement.bottom_right,
--    })
--
-- This can also be used from the command line:
--
--    awesome-client 'awful=require("awful");
--      awful.spawn("urxvt -e maxima -name CALCULATOR", {
--        floating  = true,
--        tag       = mouse.screen.selected_tag,
--        placement = awful.placement.bottom_right,
--      })'
--
-- **Getting a command's output**:
--
-- First, do **not** use `io.popen` **ever**. It is synchronous. Synchronous
-- functions **block everything** until they are done. All visual applications
-- lock (as Awesome no longer responds), you will probably lose some keyboard
-- and mouse events and will have higher latency when playing games. This is
-- also true when reading files synchronously, but this is another topic.
--
-- Awesome provides a few ways to get output from commands. One is to use the
-- `Gio` libraries directly. This is usually very complicated, but gives a lot
-- of control on the command execution.
--
-- This modules provides `with_line_callback` and `easy_async` for convenience.
-- First, lets add this bash command to `rc.lua`:
--
--    local noisy = [[bash -c '
--      for I in $(seq 1 5); do
--        date
--        echo err >&2
--        sleep 2
--      done
--    ']]
--
-- It prints a bunch of junk on the standard output (*stdout*) and error
-- (*stderr*) streams. This command would block Awesome for 10 seconds if it
-- were executed synchronously, but will not block it at all using the
-- asynchronous functions.
--
-- `with_line_callback` will execute the callbacks every time a new line is
-- printed by the command:
--
--    awful.spawn.with_line_callback(noisy, {
--        stdout = function(line)
--            naughty.notify { text = "LINE:"..line }
--        end,
--        stderr = function(line)
--            naughty.notify { text = "ERR:"..line}
--        end,
--    })
--
-- If only the full output is needed, then `easy_async` is the right choice:
--
--    awful.spawn.easy_async(noisy, function(stdout, stderr, reason, exit_code)
--        naughty.notify { text = stdout }
--    end)
--
-- **Default applications**:
--
-- If the intent is to open a file/document, then it is recommended to use the
-- following standard command. The default application will be selected
-- according to the [Shared MIME-info Database](https://specifications.freedesktop.org/shared-mime-info-spec/shared-mime-info-spec-latest.html)
-- specification. The `xdg-utils` package provided by most distributions
-- includes the `xdg-open` command:
--
--    awful.spawn({"xdg-open", "/path/to/file"})
--
-- Awesome **does not** manage, modify or otherwise influence the database
-- for default applications. For information about how to do this, consult the
-- [ARCH Linux Wiki](https://wiki.archlinux.org/index.php/default_applications).
--
-- If you wish to change how the default applications behave, then consult the
-- [Desktop Entry](https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html)
-- specification.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2008 Julien Danjou
-- @copyright 2014 Emmanuel Lepage Vallee
-- @module awful.spawn
---------------------------------------------------------------------------

local capi =
{
    awesome = awesome,
    mouse = mouse,
    client = client,
}
local lgi = require("lgi")
local Gio = lgi.Gio
local util   = require("awful.util")
local protected_call = require("gears.protected_call")

local spawn = {}


local end_of_file
do
    -- API changes, bug fixes and lots of fun. Figure out how a EOF is signalled.
    local input
    if not pcall(function()
        -- No idea when this API changed, but some versions expect a string,
        -- others a table with some special(?) entries
        input = Gio.DataInputStream.new(Gio.MemoryInputStream.new_from_data(""))
    end) then
        input = Gio.DataInputStream.new(Gio.MemoryInputStream.new_from_data({}))
    end
    local line, length = input:read_line()
    if not line then
        -- Fixed in 2016: NULL on the C side is transformed to nil in Lua
        end_of_file = function(arg)
            return not arg
        end
    elseif tostring(line) == "" and #line ~= length then
        -- "Historic" behaviour for end-of-file:
        -- - NULL is turned into an empty string
        -- - The length variable is not initialized
        -- It's highly unlikely that the uninitialized variable has value zero.
        -- Use this hack to detect EOF.
        end_of_file = function(arg1, arg2)
            return #arg1 ~= arg2
        end
    else
        assert(tostring(line) == "", "Cannot determine how to detect EOF")
        -- The above uninitialized variable was fixed and thus length is
        -- always 0 when line is NULL in C. We cannot tell apart an empty line and
        -- EOF in this case.
        require("gears.debug").print_warning("Cannot reliably detect EOF on an "
                .. "GIOInputStream with this LGI version")
        end_of_file = function(arg)
            return tostring(arg) == ""
        end
    end
end

spawn.snid_buffer = {}

function spawn.on_snid_callback(c)
    local entry = spawn.snid_buffer[c.startup_id]
    if entry then
        local props = entry[1]
        local callback = entry[2]
        c:emit_signal("spawn::completed_with_payload", props, callback)
        spawn.snid_buffer[c.startup_id] = nil
    end
end

function spawn.on_snid_cancel(id)
    if spawn.snid_buffer[id] then
        spawn.snid_buffer[id] = nil
    end
end

--- Spawn a program, and optionally apply properties and/or run a callback.
--
-- Applying properties or running a callback requires the program/client to
-- support startup notifications.
--
-- See `awful.rules.execute` for more details about the format of `sn_rules`.
--
-- @tparam string|table cmd The command.
-- @tparam[opt=true] table|boolean sn_rules A table of properties to be applied
--   after startup; `false` to disable startup notifications.
-- @tparam[opt] function callback A callback function to be run after startup.
-- @treturn[1] integer The forked PID.
-- @treturn[1] ?string The startup notification ID, if `sn` is not false, or
--   a `callback` is provided.
-- @treturn[2] string Error message.
function spawn.spawn(cmd, sn_rules, callback)
    if cmd and cmd ~= "" then
        local enable_sn = (sn_rules ~= false or callback)
        enable_sn = not not enable_sn -- Force into a boolean.
        local pid, snid = capi.awesome.spawn(cmd, enable_sn)
        -- The snid will be nil in case of failure
        if snid then
            sn_rules = type(sn_rules) ~= "boolean" and sn_rules or {}
            spawn.snid_buffer[snid] = { sn_rules, { callback } }
        end
        return pid, snid
    end
    -- For consistency
    return "Error: No command to execute"
end

--- Spawn a program using the shell.
-- This calls `cmd` with `$SHELL -c` (via `awful.util.shell`).
-- @tparam string cmd The command.
function spawn.with_shell(cmd)
    if cmd and cmd ~= "" then
        cmd = { util.shell, "-c", cmd }
        return capi.awesome.spawn(cmd, false)
    end
end

--- Spawn a program and asynchronously capture its output line by line.
-- @tparam string|table cmd The command.
-- @tab callbacks Table containing callbacks that should be invoked on
--   various conditions.
-- @tparam[opt] function callbacks.stdout Function that is called with each
--   line of output on stdout, e.g. `stdout(line)`.
-- @tparam[opt] function callbacks.stderr Function that is called with each
--   line of output on stderr, e.g. `stderr(line)`.
-- @tparam[opt] function callbacks.output_done Function to call when no more
--   output is produced.
-- @tparam[opt] function callbacks.exit Function to call when the spawned
--   process exits. This function gets the exit reason and code as its
--   arguments.
--   The reason can be "exit" or "signal".
--   For "exit", the second argument is the exit code.
--   For "signal", the second argument is the signal causing process
--   termination.
-- @treturn[1] Integer the PID of the forked process.
-- @treturn[2] string Error message.
function spawn.with_line_callback(cmd, callbacks)
    local stdout_callback, stderr_callback, done_callback, exit_callback =
        callbacks.stdout, callbacks.stderr, callbacks.output_done, callbacks.exit
    local have_stdout, have_stderr = stdout_callback ~= nil, stderr_callback ~= nil
    local pid, _, stdin, stdout, stderr = capi.awesome.spawn(cmd,
            false, false, have_stdout, have_stderr, exit_callback)
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
        if done_callback then
            done_callback()
        end
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

--- Asynchronously spawn a program and capture its output.
-- (wraps `spawn.with_line_callback`).
-- @tparam string|table cmd The command.
-- @tab callback Function with the following arguments
-- @tparam string callback.stdout Output on stdout.
-- @tparam string callback.stderr Output on stderr.
-- @tparam string callback.exitreason Exit Reason.
-- The reason can be "exit" or "signal".
-- @tparam integer callback.exitcode Exit code.
-- For "exit" reason it's the exit code.
-- For "signal" reason — the signal causing process termination.
-- @treturn[1] Integer the PID of the forked process.
-- @treturn[2] string Error message.
-- @see spawn.with_line_callback
function spawn.easy_async(cmd, callback)
    local stdout = ''
    local stderr = ''
    local exitcode, exitreason
    local function parse_stdout(str)
        stdout = stdout .. str .. "\n"
    end
    local function parse_stderr(str)
        stderr = stderr .. str .. "\n"
    end
    local function done_callback()
        return callback(stdout, stderr, exitreason, exitcode)
    end
    local exit_callback_fired = false
    local output_done_callback_fired = false
    local function exit_callback(reason, code)
        exitcode = code
        exitreason = reason
        exit_callback_fired = true
        if output_done_callback_fired then
            return done_callback()
        end
    end
    local function output_done_callback()
        output_done_callback_fired = true
        if exit_callback_fired then
            return done_callback()
        end
    end
    return spawn.with_line_callback(
        cmd, {
        stdout=parse_stdout,
        stderr=parse_stderr,
        exit=exit_callback,
        output_done=output_done_callback
    })
end

local function read_stream(stream, line_callback, done_callback, close)
    while true do
        local line, length = stream:async_read_line()

        if type(length) ~= "number" then
            -- Error
            print("Error in awful.spawn.read_lines:", tostring(length))
            break
        elseif end_of_file(line, length) then
            -- End of file
            break
        else
            -- Read a line
            -- This needs tostring() for older lgi versions which returned
            -- "GLib.Bytes" instead of Lua strings (I guess)
            protected_call(line_callback, tostring(line))
        end
    end

    if close then
        stream:async_close()
    end
    if done_callback then
        protected_call(done_callback)
    end
end

local r = read_stream
local function read_stream(...)
    protected_call(r, ...)
end

--- Read lines from a Gio input stream
-- @tparam Gio.InputStream input_stream The input stream to read from.
-- @tparam function line_callback Function that is called with each line
--   read, e.g. `line_callback(line_from_stream)`.
-- @tparam[opt] function done_callback Function that is called when the
--   operation finishes (e.g. due to end of file).
-- @tparam[opt=false] boolean close Should the stream be closed after end-of-file?
function spawn.read_lines(input_stream, line_callback, done_callback, close)
    local stream = Gio.DataInputStream.new(input_stream)
    Gio.Async.start(read_stream)(stream, line_callback, done_callback, close)
end

capi.awesome.connect_signal("spawn::canceled" , spawn.on_snid_cancel   )
capi.awesome.connect_signal("spawn::timeout"  , spawn.on_snid_cancel   )
capi.client.connect_signal ("manage"          , spawn.on_snid_callback )

return setmetatable(spawn, { __call = function(_, ...) return spawn.spawn(...) end })
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
