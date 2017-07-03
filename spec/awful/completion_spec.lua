local compl = require("awful.completion")
local shell = function(...)
    local command, pos, matches = compl.shell(...)
    return {command, pos, matches}
end
local gfs = require("gears.filesystem")
local Gio = require("lgi").Gio
local GLib = require("lgi").GLib

local has_bash = GLib.find_program_in_path("bash")
local has_zsh = GLib.find_program_in_path("zsh")

if not has_bash and not has_zsh then
    print('Skipping spec/awful/completion_spec.lua: bash and zsh are not installed.')
    return
end

local test_dir

--- Get and create a temporary test dir based on `pat`, where %d gets replaced by
-- the current PID.
local function get_test_dir()
    local tfile = Gio.File.new_tmp("awesome-tests-XXXXXX")
    local path = tfile:get_path()
    tfile:delete()
    gfs.make_directories(path)
    return path
end

describe("awful.completion.shell in empty directory", function()
    local orig_popen = io.popen

    setup(function()
        test_dir = get_test_dir()
        io.popen = function(...)  --luacheck: ignore
            return orig_popen(string.format('cd %s && ', test_dir) .. ...)
        end
    end)

    teardown(function()
        assert.True(os.remove(test_dir))
        io.popen = orig_popen  --luacheck: ignore
    end)

    if has_bash then
        it("handles completion of true (bash)", function()
            assert.same(shell('true', 5, 1, 'bash'), {'true', 5, {'true'}})
        end)
    end
    if has_zsh then
        it("handles completion of true (zsh)", function()
            assert.same(shell('true', 5, 1, 'zsh'), {'true', 5, {'true'}})
        end)
    end
    it("handles completion of true (nil)", function()
        assert.same(shell('true', 5, 1, nil), {'true', 5, {'true'}})
    end)
end)

describe("awful.completion.shell", function()
    local orig_popen = io.popen

    setup(function()
        test_dir = get_test_dir()
        os.execute(string.format(
            'cd %s && touch localcommand && chmod +x localcommand', test_dir))
        io.popen = function(...)  --luacheck: ignore
            return orig_popen(string.format('cd %s && ', test_dir) .. ...)
        end
    end)

    teardown(function()
        assert.True(os.remove(test_dir .. '/localcommand'))
        assert.True(os.remove(test_dir))
        io.popen = orig_popen  --luacheck: ignore
    end)

    if has_bash then
        it("handles empty input (bash)", function()
            assert.same(shell('', 1, 1, 'bash'), {'', 1})
        end)
    end
    if has_zsh then
        it("handles empty input (zsh)", function()
            assert.same(shell('', 1, 1, 'zsh'), {'', 1})
        end)
    end
    it("handles empty input (nil)", function()
        assert.same(shell('', 1, 1, nil), {'', 1})
    end)

    if has_bash then
        it("completes local command (bash)", function()
            assert.same(shell('./localcomm', 12, 1, 'bash'), {'./localcommand', 15, {'./localcommand'}})
        end)
    end
    if has_zsh then
        it("completes local command (zsh)", function()
            assert.same(shell('./localcomm', 12, 1, 'zsh'), {'./localcommand', 15, {'./localcommand'}})
        end)
    end

    if has_bash then
        it("completes local file (bash)", function()
            assert.same(shell('ls ', 4, 1, 'bash'), {'ls localcommand', 16, {'localcommand'}})
        end)
    end
    if has_zsh then
        it("completes local file (zsh)", function()
            assert.same(shell('ls ', 4, 1, 'zsh'), {'ls localcommand', 16, {'localcommand'}})
        end)
    end
end)

describe("awful.completion.shell handles $SHELL", function()
    local orig_getenv = os.getenv
    local gdebug = require("gears.debug")
    local orig_print_warning
    local print_warning_message
    local os_getenv_shell

    setup(function()
        os.getenv = function(name)  -- luacheck: ignore
            if name == 'SHELL' then return os_getenv_shell end
            return orig_getenv(name)
        end

        orig_print_warning = gdebug.print_warning
        gdebug.print_warning = function(message)
            print_warning_message = message
        end
    end)

    teardown(function()
        os.getenv = orig_getenv  --luacheck: ignore
        gdebug.print_warning = orig_print_warning
    end)

    before_each(function()
        compl.default_shell = nil
        print_warning_message = nil
    end)

    it("falls back to bash if unset", function()
        assert.same(shell('true', 5, 1, nil), {'true', 5, {'true'}})
        assert.same(print_warning_message,
                    'SHELL not set in environment, falling back to bash.')
        assert.same(compl.default_shell, "bash")
    end)

    it("uses zsh from path", function()
        os_getenv_shell = '/opt/bin/zsh'
        assert.same(shell('true', 5, 1, nil), {'true', 5, {'true'}})
        assert.same(compl.default_shell, "zsh")
        assert.is_nil(print_warning_message)
    end)

    it("uses bash for unknown", function()
        os_getenv_shell = '/dev/null'
        assert.same(shell('true', 5, 1, nil), {'true', 5, {'true'}})
        assert.same(compl.default_shell, "bash")
        assert.is_nil(print_warning_message)
    end)
end)

-- vim: ft=lua:et:sw=4:ts=8:sts=4:tw=80
