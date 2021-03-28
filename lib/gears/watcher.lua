--- Fetch information at a specific interval.
--
-- @author Emmanuel Lepage-Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2020 Emmanuel Lepage-Vallee
-- @classmod gears.watcher

local capi = {awesome = awesome}
local protected_call = require("gears.protected_call")
local gtimer = require("gears.timer")
local gtable = require("gears.table")
local lgi = require("lgi")
local Gio = lgi.Gio
local GLib = lgi.GLib

local module = {}

-- This is awful.util.shell
module._shell = os.getenv("SHELL") or "/bin/sh"

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

module._end_of_file = end_of_file

function module._read_lines(input_stream, line_callback, done_callback, close)
    local stream = Gio.DataInputStream.new(input_stream)
    local function done()
        if close then
            stream:close()
        end
        stream:set_buffer_size(0)
        if done_callback then
            protected_call(done_callback)
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
        elseif end_of_file(line, length) then
            -- End of file
            done()
        else
            -- Read a line
            -- This needs tostring() for older lgi versions which returned
            -- "GLib.Bytes" instead of Lua strings (I guess)
            protected_call(line_callback, tostring(line))

            -- Read the next line
            start_read()
        end
    end
    start_read()
end

function module._with_line_callback(cmd, callbacks)
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
        module._read_lines(Gio.UnixInputStream.new(stdout, true),
                stdout_callback, step_done, true)
    end
    if have_stderr then
        module._read_lines(Gio.UnixInputStream.new(stderr, true),
                stderr_callback, step_done, true)
    end
    assert(stdin == nil)
    return pid
end

function module._easy_async(cmd, callback)
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
    return module._with_line_callback(
        cmd, {
        stdout=parse_stdout,
        stderr=parse_stderr,
        exit=exit_callback,
        output_done=output_done_callback
    })
end

function module._read_async(path, callback, fail_callback)
    local cancel = Gio.Cancellable()

    Gio.File.new_for_path(path):load_contents_async(cancel, function(file, task)
        local content = file:load_contents_finish(task)
        if content then
            callback(path, content)
        elseif fail_callback then
            fail_callback(path)
        end
    end)

    return cancel
end

-- Posix files and commands always end with a newline.
-- It is nearly always unwanted, so we strip it by default.
local function remove_posix_extra_newline(content)
    -- Remove the trailing `\n`
    if content:sub(-1) == '\n' then
        content = content:sub(1, -2)
    end

    return content
end

--- Make sure we sort transactions in a way obsolete ones are
-- not used.

local function add_transaction(self, transaction)
    table.insert(self._private.transactions, transaction)
end

local function remove_transaction(self, transaction)

    -- Too late, abort.
    for _, cancel in pairs(transaction.pending) do
        cancel:cancel()
    end

    for k, t in ipairs(self._private.transactions) do
        if t == transaction then
            table.remove(self._private.transactions, k)
        end
    end

end

-- Keys can also be labels or object, # wont work.
local function count_files(files)
    local ret = 0

    for _ in pairs(files) do
        ret = ret + 1
    end

    return ret
end

-- When there is multiple files, we need to wait
-- until are of them are read.
local function gen_file_transaction(self)
    if count_files(self.files) == 0 then return nil end

    local ret = {
        counter = count_files(self.files),
        files   = gtable.clone(self.files, false),
        filter  = self.filter,
        labels  = {},
        content = {},
        failed  = {},
        pending = {},

    }

    local function finished(file, content)
        assert(not ret.content[file])
        self:emit_signal("file::acquired", file, content)
        ret.pending[file] = nil

        ret.counter = ret.counter - 1
        ret.content[file] = content

        if ret.counter > 0 then return end

        self:emit_signal("files::acquired", ret.content, ret.failed)

        -- Make the final table using the stored keys.
        local contents = {}

        for path, ctn in pairs(ret.content) do
            if self.strip_newline then
                ctn = remove_posix_extra_newline(ctn)
            end

            contents[self._private.file_keys[path]] = ctn

            if self.labels_as_properties and type(ret.labels[path]) == "string" then
                local val

                if ret.filter then
                    val = ret.filter(ret.labels[path], ctn)
                else
                    val = ctn
                end

                -- Make sure the signals are not fired for nothing.
                if val ~= self[ret.labels[path]] then
                    self[ret.labels[path]] = val
                end
            end
        end

        local ctn = count_files(ret.files) == 1 and contents[next(contents)] or contents

        if ret.filter and not self.labels_as_properties then
            self._private.value = ret.filter(ctn, ret.failed)
        else
            self._private.value = ctn
        end

        self:emit_signal("property::value", self._private.value)

        remove_transaction(self, ret)
    end

    local function read_error(file)
        ret.pending[file] = nil
        table.insert(ret.failed, file)
        self:emit_signal("file::failed", file)
    end

    for label, file in pairs(ret.files) do
        ret.labels[file] = label

        local cancel = module._read_async(file, finished, read_error)

        ret.pending[file] = cancel
    end

    return ret
end

local modes_start, modes_abort = {}, {}

modes_start["none"] = function() --[[nop]] end
modes_abort["none"] = function() --[[nop]] end

modes_start["files"] = function(self)
    if not self._private.init then return end

    local t = gen_file_transaction(self)

    add_transaction(self, t)
end

modes_abort["files"] = function(self)
    for _, t in ipairs(self._private.transactions) do
        remove_transaction(self, t)
    end
end

modes_start["command"] = function(self)
    if not self._private.init then return end

    local com = self._private.command

    if self._private.shell then
        assert(
            type(com) == "string",
            "When using `gears.watcher` with `shell = true`, "..
            "the command must be a string"
        )

        com = {module._shell, '-c', com}
    end

    module._easy_async(com, function(lines, stderr, err, errcode)
        if self.strip_newline then
            lines = remove_posix_extra_newline(lines)
        end

        if self.filter then
            self._private.value = self.filter(lines, stderr, err, errcode)
        else
            self._private.value = lines
        end
    end)
end

modes_abort["command"] = function()
    --TODO
end

function module:_set_declarative_handler(parent, key, ids)
    assert(type(key) == "string", "A watcher can only be attached to properties")

    table.insert(self._private.targets, {
        ids = ids,
        parent = parent,
        property = key
    })

    if self._private.value then
        parent[key] = self._private.value
    end
end

--- Abort the current transactions.
--
-- If files are currently being read or commands executed,
-- abort it. This does prevent new transaction from being
-- started. Use `:stop()` for that.
--
-- @method abort
-- @see stop

function module:abort()
    modes_abort[self._private.mode](self)
end

--- Emitted when a file is read.
--
-- @signal file::acquired
-- @tparam string path The path.
-- @tparam string content The file content.
-- @see files::acquired
-- @see file::failed

--- When reading a file failed.
-- @signal file::failed
-- @tparam string path The path.

--- Emitted when all files are read.
--
-- @signal files::acquired
-- @tparam table contents Path as keys and content as values.
-- @tparam table failed The list of files which could not be read.

--- A file path.
--
-- @DOC_text_gears_watcher_simple_EXAMPLE@
--
-- @property file
-- @tparam string file
-- @propemits true false
-- @see files

--- A list or map of files.
--
-- It is often necessary to query multiple files. When reading from `proc`,
-- some data is split across multiple files, such as the battery charge.
--
-- This property accepts 2 format. One uses is a plain table of paths while
-- the other is a label->path map. Depending on what is being read, both make
-- sense.
--
-- **Simple path list:**
--
-- @DOC_text_gears_watcher_files1_EXAMPLE@
--
-- **With labels:**
--
-- @DOC_text_gears_watcher_files2_EXAMPLE@
--
-- @property files
-- @tparam table files
-- @propemits true false
-- @see labels_as_properties

function module:get_file()
    return self._private.files[1]
end

function module:set_file(path)
    self:set_files({path})
end

function module:get_files()
    return self._private.files
end

function module:set_files(paths)
    self:abort()

    self._private.files = paths or {}
    self._private.mode  = "files"

    self:emit_signal("property::files", self._private.files   )
    self:emit_signal("property::file" , select(2, next(self._private.files)))

    -- It is possible to give names to each files. For modules like
    -- battery widgets, which require reading multiple long paths from
    -- `proc`, it makes the user code more readable.
    self._private.file_keys = {}

    for k, v in pairs(self._private.files) do
        self._private.file_keys[v] = type(k) == "number" and v or k
    end

    modes_start["files"](self)
end

--- Add a file to the files list.
--
-- @method append_file
-- @tparam string path The path.
-- @tparam[opt] string The key.

function module:append_file(path, key)
    self:abort()

    if self._private.files[path] then return end

    key = key or (#self._private.files + 1)

    self._private.mode  = "files"

    self._private.files[key] = path
    self._private.file_keys[path] = key

    self:emit_signal("property::files", self._private.files   )
    self:emit_signal("property::file" , select(2, next(self._private.files)))

    modes_start["files"](self)
end

--- Remove a file to the files list.
--
-- @method remove_file
-- @tparam string path The path or the key.

--- A filter to post-process the file or command.
--
-- It can be used, for example, to convert the string to a number or turn
-- the various file content into the final value. The filter argument
-- depend on various `gears.watcher` properties (as documented below).
--
-- **The callback parameters for a single file:**
--   (1) The file content (string)
--
-- **The callback parameters for a multiple files (paths):**
--   (1) Tables with the paths as keys and string content as value.
--
-- **The callback parameters for a multiple files (labels):**
--   (1) Tables with the keys as keys and string content as value.
--
-- **The callback when `labels_as_properties` is true:
--   (1) The label name
--   (2) The content
--
-- **The callback parameters for a command:**
--   (1) Stdout as first parameter
--   (2) Stderr as second parameter
--   (3) Exitreason
--   (4) Exitcode
--
-- @property filter
-- @tparam function filter
-- @propemits true false

function module:get_filter()
    return self._private.filter
end

function module:set_filter(filter)
    self:abort()

    self._private.filter = filter

    self:emit_signal("property::filter", filter)

    modes_start[self._private.mode](self)
end

--- A command.
--
-- If you plan to use pipes or any shell features, do not
-- forget to also set `shell` to `true`.
--
-- @DOC_text_gears_watcher_command1_EXAMPLE@
--
-- @property command
-- @tparam table|string command
-- @propemits true false

function module:get_command()
    return self._private.command
end

function module:set_command(command)
    self._private.command = command
    self._private.mode    = "command"
    self:emit_signal("property::command")
    modes_start["command"](self)
end

--- Use a shell when calling the command.
--
-- This means you can use `|`, `&&`, `$?` in your command.
--
-- @DOC_text_gears_watcher_command2_EXAMPLE@
---
-- @property shell
-- @tparam[opt=false] boolean|string shell
-- @propemits true false

function module:get_shell()
    return self._private.shell or false
end

function module:set_shell(shell)
    self._private.shell = shell
    self:emit_signal("property::shell")
end

--- In files mode, when paths have labels, apply them as properties.
--
-- @DOC_wibox_widget_declarative_watcher_EXAMPLE@
--
-- @property labels_as_properties
-- @tparam[opt=false] boolean labels_as_properties
-- @see files

--- The interval between the content refresh.
--
-- (in seconds)
--
-- @property interval
-- @tparam number interval
-- @see gears.timer.timeout
-- @propemits true false

-- There is not get_timeout/set_timeout, so we can't make aliases.
module.get_interval = gtimer.get_timeout
module.set_interval = gtimer.set_timeout

--- The current value of the watcher.
--
-- If there is no filter, this will be a string. If a filter is used,
-- then it is whatever it returns.
--
-- @property value
-- @propemits false false

function module:get_value()
    return self._private.value
end

--- Strip the extra trailing newline.
--
-- All posix compliant text file and commands end with a newline.
-- Most of the time, this is inconvinient, so `gears.watcher` removes
-- them by default. Set this to `false` if this isn't the desired
-- behavior.
--
-- @property strip_newline
-- @tparam[opt=true] boolean strip_newline
-- @propemits true false

function module:get_strip_newline()
    return self._private.newline
end

function module:set_strip_newline(value)
    self._private.newline = value
    self:emit_signal("property::strip_newline", value)
end

--- Start the timer.
-- @method start
-- @emits start
-- @baseclass gears.timer
-- @see stop

--- Stop the timer.
-- @method stop
-- @emits stop
-- @baseclass gears.timer
-- @see abort
-- @see start

--- The timer is started.
-- @property started
-- @tparam boolean started
-- @propemits false false
-- @baseclass gears.timer

--- Restart the timer.
-- This is equivalent to stopping the timer if it is running and then starting
-- it.
-- @method again
-- @baseclass gears.timer
-- @emits start
-- @emits stop

--- Create a new `gears.watcher` object.
--
-- @constructorfct gears.watcher
-- @tparam table args
-- @tparam string args.file A file path.
-- @tparam table args.files A list or map of files.
-- @tparam function args.filter A filter to post-process the file or command.
-- @tparam table|string args.command A command (without a shell).
-- @tparam[opt=false] boolean args.shell Use a shell when calling the command.
-- @param args.initial_value The value to use before the first "real" value is acquired.
-- @tparam number args.interval The interval between the content refresh.
-- @tparam boolean args.labels_as_properties Set the file labels as properties.
-- @tparam boolean args.started The timer is started.

local function new(_, args)
    local ret = gtimer()
    ret.timeout = 5000

    local newargs = gtable.clone(args or {}, false)

    ret._private.mode = "none"
    ret._private.transactions = {}
    ret._private.targets = {}
    ret._private.files = {}

    if newargs.autostart == nil then
        newargs.autostart = true
    end

    if newargs.strip_newline == nil then
        newargs.strip_newline = true
    end

    gtable.crush(ret, module , true )
    gtable.crush(ret, newargs, false)

    ret.shell = args.shell
    -- ret:set_shell(args.shell)

    local function value_callback()
        for _, target in ipairs(ret._private.targets) do
            target.parent[target.property] = ret.value
        end
    end

    local function update_callback()
        modes_start[ret._private.mode](ret)
    end

    ret:connect_signal("property::value", value_callback)
    ret:connect_signal("timeout", update_callback)

    if args.initial_value then
        ret._private.value = args.initial_value
    end

    ret._private.init = true

    if newargs.autostart then
        ret:start()
        modes_start[ret._private.mode](ret)
    end

    return ret
end

--@DOC_object_COMMON@

return setmetatable(module, {__call = new})
