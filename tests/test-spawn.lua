--- Tests for spawn

local runner = require("_runner")
local spawn = require("awful.spawn")

local lua_executable = os.getenv("LUA")
if lua_executable == nil or lua_executable == "" then
    lua_executable = "lua"
end

local spawns_done = 0
local async_spawns_done = 0
local io_spawns_done = 0
local exit_yay, exit_snd = nil, nil

-- * Using spawn with array is already covered by the test client.
-- * spawn with startup notification is covered by test-spawn-snid.lua

local tiny_client = function(class)
    return { lua_executable, "-e", [[
local lgi = require 'lgi'
local Gtk = lgi.require('Gtk', '3.0')
local class = ']]..class..[['

Gtk.init()

window = Gtk.Window {
    default_width  = 100,
    default_height = 100,
    title          = 'title',
}

window:set_wmclass(class, class)

local app = Gtk.Application {}

function app:on_activate()
    window.application = self
    window:show_all()
end

app:run {''}
]]}
end

local steps = {
    function()
        -- Test various error conditions. There are quite a number of them...
        local error_message

        error_message = spawn("this_does_not_exist_and_should_fail")
        assert(string.find(error_message, 'No such file or directory'), error_message)

        error_message = spawn({"this_does_not_exist_and_should_fail"})
        assert(string.find(error_message, 'No such file or directory'), error_message)

        error_message = spawn("foo '")
        assert(string.find(error_message, 'parse error: Text ended before matching quote was found'), error_message)

        error_message = spawn()
        assert(string.find(error_message, 'No command to execute'), error_message)

        error_message = spawn(" ")
        assert(string.find(error_message, 'Text was empty'), error_message)

        error_message = spawn("")
        assert(string.find(error_message, 'No command to execute'), error_message)

        error_message = spawn{}
        assert(string.find(error_message, 'There is nothing to execute'), error_message)

        return true
    end,

    function(count)
        if count == 1 then
            spawn.easy_async("echo yay", function(stdout)
                if stdout:match("yay") then
                    async_spawns_done = async_spawns_done + 1
                end
            end)
            spawn.easy_async_with_shell("true && echo yay", function(stdout)
                if stdout:match("yay") then
                    async_spawns_done = async_spawns_done + 1
                end
            end)
            local steps_yay = 0
            spawn.with_line_callback("echo yay", {
                                     stdout = function(line)
                                         assert(line == "yay", "line == '" .. tostring(line) .. "'")
                                         assert(steps_yay == 0)
                                         steps_yay = steps_yay + 1
                                     end,
                                     output_done = function()
                                         assert(steps_yay == 1)
                                         steps_yay = steps_yay + 1
                                         spawns_done = spawns_done + 1
                                     end,
                                     exit = function(reason, code)
                                         assert(reason == "exit")
                                         assert(exit_yay == nil)
                                         assert(code == 0)
                                         exit_yay = code
                                     end
                                 })

            -- Test that setting env vars works and that the env is cleared
            local read_line = false
            local pid, _, _, stdout = awesome.spawn({ "sh", "-c", "echo $AWESOME_SPAWN_TEST_VAR $HOME $USER" },
                    false, false, true, false, nil, { "AWESOME_SPAWN_TEST_VAR=42" })
            assert(type(pid) ~= "string", pid)
            spawn.read_lines(require("lgi").Gio.UnixInputStream.new(stdout, true),
                    function(line)
                        assert(not read_line)
                        read_line = true
                        assert(line == "42", line)
                        spawns_done = spawns_done + 1
                    end, nil, true)

            -- Test error in parse_table_array.
            pid = awesome.spawn({"true"}, false, false, true, false, nil, { 0 })
            assert(pid == 'spawn: environment parse error: Non-string argument at table index 1', pid)

            -- Test error in parse_command.
            pid = awesome.spawn({0}, false, false, true, false, nil, {})
            assert(pid == 'spawn: parse error: Non-string argument at table index 1', pid)


            local steps_count = 0
            local err_count = 0
            spawn.with_line_callback({ "sh", "-c", "printf line1\\\\nline2\\\\nline3 ; echo err >&2 ; exit 42" }, {
                                     stdout = function(line)
                                         assert(steps_count < 3)
                                         steps_count = steps_count + 1
                                         assert(line == "line" .. steps_count, "line == '" .. tostring(line) .. "'")
                                     end,
                                     stderr = function(line)
                                         assert(err_count == 0)
                                         err_count = err_count + 1
                                         assert(line == "err", "line == '" .. tostring(line) .. "'")
                                     end,
                                     output_done = function()
                                         assert(steps_count == 3)
                                         assert(err_count == 1)
                                         steps_count = steps_count + 1
                                         spawns_done = spawns_done + 1
                                     end,
                                     exit = function(reason, code)
                                         assert(reason == "exit")
                                         assert(exit_snd == nil)
                                         assert(code == 42)
                                         exit_snd = code
                                     end
                                 })

            spawn.once(tiny_client("client1"), {tag=screen[1].tags[2]})
        end
        if spawns_done == 3 then
            assert(exit_yay == 0)
            assert(exit_snd == 42)
            assert(async_spawns_done == 2)
            return true
        end
    end,
    -- Test inheriting stdio
    function(count)
        if count == 1 then
            do -- Test that DEV_NULL works and doesn't return a fd
                local read_line = false
                local pid, _, _, stdout, stderr = awesome.spawn({ 'readlink', '/proc/self/fd/2' },
                        false, false, true, "DEV_NULL")
                assert(type(pid) ~= "string", pid)
                assert(stderr == nil)
                spawn.read_lines(require("lgi").Gio.UnixInputStream.new(stdout, true),
                    function(line)
                        assert(not read_line)
                        read_line = true
                        assert(line == "/dev/null", line)
                        io_spawns_done = io_spawns_done + 1
                    end, nil, true)
            end

            do -- Test that INHERIT works and doesn't return a fd
                -- Note: if this is /dev/null, this test is useless.
                local test_stdin = require('lgi').GLib.file_read_link('/proc/self/fd/0')

                local read_line = false
                local pid, _, stdin, stdout = awesome.spawn({ 'readlink', '/proc/self/fd/0' },
                        false, "INHERIT", true, false)
                assert(type(pid) ~= "string", pid)
                assert(stdin == nil)
                spawn.read_lines(require("lgi").Gio.UnixInputStream.new(stdout, true),
                    function(line)
                        assert(not read_line)
                        read_line = true
                        assert(line == test_stdin, line)
                        io_spawns_done = io_spawns_done + 1
                    end, nil, true)
            end

            do -- Test that false doesn't return a pointer (behavior is untested - GLib defaults)
                local pid, _, stdin, stdout, stderr = awesome.spawn({"true"},
                        false, false, false, false)
                assert(type(pid) ~= "string", pid)
                assert(stdin == nil)
                assert(stdout == nil)
                assert(stderr == nil)
            end

            do -- Test that true returns a pipe
                local read_line = false
                local pid, _, stdin, stdout, stderr = awesome.spawn({ 'readlink', '/proc/self/fd/0' },
                        false, true, true, true)
                assert(type(pid) ~= "string", pid)
                assert(stdin ~= nil)
                assert(stdout ~= nil)
                assert(stderr ~= nil)
                spawn.read_lines(require("lgi").Gio.UnixInputStream.new(stdout, true),
                    function(line)
                        assert(not read_line)
                        read_line = true
                        assert(line:find("^pipe:%[[0-9]+%]$"), line)
                        io_spawns_done = io_spawns_done + 1
                    end, nil, true)
            end
        end
        if io_spawns_done == 3 then
            return true
        end
    end,
    -- Test spawn_once
    function()
        if #client.get() ~= 1 then return end

        assert(client.get()[1].class == "client1")
        assert(client.get()[1]:tags()[1] == screen[1].tags[2])

        spawn.once(tiny_client("client1"), {tag=screen[1].tags[2]})
        spawn.once(tiny_client("client1"), {tag=screen[1].tags[2]})
        return true
    end,
    function(count)
        -- Limit the odds of a race condition
        if count ~= 3 then return end

        assert(#client.get() == 1)
        assert(client.get()[1].class == "client1")
        client.get()[1]:kill()
        return true
    end,
    -- Test single_instance
    function()
        if #client.get() ~= 0 then return end

        -- This should do nothing
        spawn.once(tiny_client("client1"), {tag=screen[1].tags[2]})

        spawn.single_instance(tiny_client("client2"), {tag=screen[1].tags[3]})

        return true
    end,
    -- Test that no extra clients are created
    function()
        if #client.get() ~= 1 then return end

        assert(client.get()[1].class == "client2")
        assert(client.get()[1]:tags()[1] == screen[1].tags[3])

        -- This should do nothing
        spawn.single_instance(tiny_client("client2"), {tag=screen[1].tags[3]})
        spawn.single_instance(tiny_client("client2"), {tag=screen[1].tags[3]})

        return true
    end,
    function()
        if #client.get() ~= 1 then return end

        assert(client.get()[1].class == "client2")
        assert(client.get()[1]:tags()[1] == screen[1].tags[3])

        client.get()[1]:kill()

        return true
    end,
    -- Test that new instances can be spawned
    function()
        if #client.get() ~= 0 then return end

        spawn.single_instance(tiny_client("client2"), {tag=screen[1].tags[3]})

        return true
    end,
    -- Test raise_or_spawn
    function()
        if #client.get() ~= 1 then return end

        assert(client.get()[1].class == "client2")
        assert(client.get()[1]:tags()[1] == screen[1].tags[3])
        client.get()[1]:kill()

        assert(not spawn.raise_or_spawn(tiny_client("client3"), {
            tag=screen[1].tags[3], foo = "baz"
        }))

        return true
    end,
    -- Add more clients to test the focus
    function()
        if #client.get() ~= 1 or client.get()[1].class ~= "client3" then return end

        -- In another iteration to make sure client4 has no focus
        spawn(tiny_client("client4"), {tag = screen[1].tags[4], foo = "bar"})
        spawn(tiny_client("client4"), {tag = screen[1].tags[4], foo = "bar"})
        spawn(tiny_client("client4"), {
            tag = screen[1].tags[4], switch_to_tags= true, focus = true, foo = "bar"
        })

        return true
    end,
    function()
        if #client.get() ~= 4 then return end

        local by_class = {}
        for _, c in ipairs(client.get()) do
            by_class[c.class] = by_class[c.class] and (by_class[c.class] + 1) or 1
            if c.class == "client4" then
                assert(#c:tags() == 1)
                assert(c.foo == "bar")
                assert(c:tags()[1] == screen[1].tags[4])
            elseif c.class == "client3" then
                assert(c.foo == "baz")
                assert(c:tags()[1] == screen[1].tags[3])
            end
        end

        assert(by_class.client3 == 1)
        assert(by_class.client4 == 3)
        assert(screen[1].tags[3].selected == false)
        assert(screen[1].tags[4].selected == true )

        assert(screen[1].tags[4].selected == true)

        assert(spawn.raise_or_spawn(tiny_client("client3"), {tag=screen[1].tags[3], foo = "baz"}))

        return true
    end,
    -- Test that the client can be raised
    function()
        if #client.get() ~= 4 then return false end

        assert(client.focus.class == "client3")
        assert(screen[1].tags[3].selected == true)
        assert(screen[1].tags[4].selected == false)


        for _, c in ipairs(client.get()) do
            if c.class == "client4" then
                c:tags()[1]:view_only()
                client.focus = c
                break
            end
        end

        assert(screen[1].tags[3].selected == false)
        assert(screen[1].tags[4].selected == true )

        for _, c in ipairs(client.get()) do
            if c.class == "client3" then
                c:kill()
            end
        end

        return true
    end,
    -- Test that a new instance can be spawned
    function()
        if #client.get() ~= 3 then return end

        spawn.raise_or_spawn(tiny_client("client3"), {tag=screen[1].tags[3]})

        return true
    end,
    function()
        if #client.get() ~= 4 then return end

        -- Cleanup
        for _, c in ipairs(client.get()) do
            c:kill()
        end

        return true
    end,
    -- Test the matcher
    function()
        -- pre spawn client
        if #client.get() ~= 0 then return end
        spawn(tiny_client("client1"))

        return true
    end,
    function()
        -- test matcher
        if #client.get() ~= 1 then return end

        local matcher = function(c)
            return c.class == "client1" or c.class == "client2"
        end

        -- This should do nothing
        spawn.once(tiny_client("client2"), {tag=screen[1].tags[5]}, matcher)
        spawn.single_instance(tiny_client("client2"), {tag=screen[1].tags[5]}, matcher)

        return true
    end,
    function()
        -- clean up
        if #client.get() ~= 1 then return end
        client.get()[1]:kill()
        return true
    end,
    -- Test rules works with matcher if client doesn't support startup id
    -- Can this test be performed using '_client' module without external 'xterm' application?
    function()
        if #client.get() ~= 0 then return end

        spawn.single_instance("xterm", {tag=screen[1].tags[5]}, function(c)
            return c.class == "XTerm"
        end)

        return true
    end,
    function()
        if #client.get() ~= 1 then return end
        assert(client.get()[1]:tags()[1] == screen[1].tags[5])
        client.get()[1]:kill()
        return true
    end,
    -- Test that rules are optional
    function()
        if #client.get() ~= 0 then return end

        spawn.single_instance(tiny_client("client4"))
        spawn.once(tiny_client("client5"))

        return true
    end,
    function()
        if #client.get() ~= 2 then return end
        return true
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
