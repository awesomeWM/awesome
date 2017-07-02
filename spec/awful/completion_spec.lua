local compl = require("awful.completion")
local shell = function(...)
    local command, pos, matches = compl.shell(...)
    return {command, pos, matches}
end
local gfs = require("gears.filesystem")
local Gio = require("lgi").Gio
local GLib = require("lgi").GLib
local lfs = require("lfs")

local has_bash = GLib.find_program_in_path("bash")
local has_zsh = GLib.find_program_in_path("zsh")

if not has_bash and not has_zsh then
    print('Skipping spec/awful/completion_spec.lua: bash and zsh are not installed.')
    return
end

local orig_path_env = GLib.getenv("PATH")
local required_commands = {"bash", "zsh", "sort"}

-- Change PATH
local function get_test_path_dir()
    local test_path = Gio.File.new_tmp("awesome-tests-path-XXXXXX")
    test_path:delete()
    test_path:make_directory()

    for _, cmd in ipairs(required_commands) do
        local target = GLib.find_program_in_path(cmd)
        if target then
            test_path:get_child(cmd):make_symbolic_link(target)
        end
    end

    test_path = test_path:get_path()
    GLib.setenv("PATH", test_path, true)
    return test_path
end

-- Reset PATH
local function remove_test_path_dir(test_path)
    GLib.setenv("PATH", orig_path_env, true)
    for _, cmd in ipairs(required_commands) do
        os.remove(test_path .. "/" .. cmd)
    end
    os.remove(test_path)
end

local test_dir
local test_path

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
    local orig_dir = lfs.currentdir()

    setup(function()
        test_dir = get_test_dir()
        lfs.chdir(test_dir)
        test_path = get_test_path_dir()
    end)

    teardown(function()
        assert.True(os.remove(test_dir))
        remove_test_path_dir(test_path)
        lfs.chdir(orig_dir)
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
    local orig_dir = lfs.currentdir()

    setup(function()
        test_dir = get_test_dir()
        gfs.make_directories(test_dir .. '/true')
        Gio.File.new_for_path(test_dir .. '/true/with_file'):create(Gio.FileCreateFlags.NONE);
        gfs.make_directories(test_dir .. '/just_a_directory')
        Gio.File.new_for_path(test_dir .. '/just_a_directory/with_file'):create(Gio.FileCreateFlags.NONE);
        -- Chaotic order is intended!
        gfs.make_directories(test_dir .. '/ambiguous_dir_a')
        gfs.make_directories(test_dir .. '/ambiguous_dir_e')
        Gio.File.new_for_path(test_dir .. '/ambiguous_dir_e/with_file'):create(Gio.FileCreateFlags.NONE);
        gfs.make_directories(test_dir .. '/ambiguous_dir_c')
        Gio.File.new_for_path(test_dir .. '/ambiguous_dir_c/with_file'):create(Gio.FileCreateFlags.NONE);
        gfs.make_directories(test_dir .. '/ambiguous_dir_d')
        gfs.make_directories(test_dir .. '/ambiguous_dir_b')
        gfs.make_directories(test_dir .. '/ambiguous_dir_f')
        os.execute(string.format(
            'cd %s && touch localcommand && chmod +x localcommand', test_dir))
        lfs.chdir(test_dir)
        test_path = get_test_path_dir()
    end)

    teardown(function()
        assert.True(os.remove(test_dir .. '/ambiguous_dir_a'))
        assert.True(os.remove(test_dir .. '/ambiguous_dir_b'))
        assert.True(os.remove(test_dir .. '/ambiguous_dir_c/with_file'))
        assert.True(os.remove(test_dir .. '/ambiguous_dir_c'))
        assert.True(os.remove(test_dir .. '/ambiguous_dir_d'))
        assert.True(os.remove(test_dir .. '/ambiguous_dir_e/with_file'))
        assert.True(os.remove(test_dir .. '/ambiguous_dir_e'))
        assert.True(os.remove(test_dir .. '/ambiguous_dir_f'))
        assert.True(os.remove(test_dir .. '/just_a_directory/with_file'))
        assert.True(os.remove(test_dir .. '/just_a_directory'))
        assert.True(os.remove(test_dir .. '/localcommand'))
        assert.True(os.remove(test_dir .. '/true/with_file'))
        assert.True(os.remove(test_dir .. '/true'))
        assert.True(os.remove(test_dir))
        remove_test_path_dir(test_path)
        lfs.chdir(orig_dir)
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
            assert.same(shell('ls l', 5, 1, 'bash'), {'ls localcommand', 16, {'localcommand'}})
        end)
    end
    if has_zsh then
        it("completes local file (zsh)", function()
            assert.same(shell('ls l', 5, 1, 'zsh'), {'ls localcommand', 16, {'localcommand'}})
        end)
    end

    if has_bash then
        it("completes command regardless of local directory (bash)", function()
            assert.same(shell('true', 5, 1, 'bash'), {'true', 5, {'true'}})
        end)
    end
    if has_zsh then
        it("completes command regardless of local directory (zsh)", function()
            assert.same(shell('true', 5, 1, 'zsh'), {'true', 5, {'true'}})
        end)
    end
    it("completes command regardless of local directory (nil)", function()
        assert.same(shell('true', 5, 1, nil), {'true', 5, {'true'}})
    end)

    if has_bash then
        it("completes local directories starting with ./ (bash)", function()
            assert.same(shell('./just', 7, 1, 'bash'), {'./just_a_directory/', 20, {'./just_a_directory/'}})
            assert.same(shell('./t', 4, 1, 'bash'), {'./true/', 8, {'./true/'}})
        end)
    end
    if has_zsh then
        it("completes local directories starting with ./ (zsh, non-empty)", function()
            assert.same(shell('./just', 7, 1, 'zsh'), {'./just_a_directory/', 20, {'./just_a_directory/'}})
            assert.same(shell('./t', 4, 1, 'zsh'), {'./true/', 8, {'./true/'}})
        end)
    end
    --]]
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

        test_path = get_test_path_dir()
    end)

    teardown(function()
        os.getenv = orig_getenv  --luacheck: ignore
        gdebug.print_warning = orig_print_warning
        remove_test_path_dir(test_path)
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
