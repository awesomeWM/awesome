local compl = require("awful.completion")
local shell = function(...)
    local command, pos, matches = compl.shell(...)
    return {command, pos, matches}
end
local gfs = require("gears.filesystem")

describe("awful.completion.shell in empty directory", function()
    local orig_popen = io.popen

    setup(function()
        gfs.make_directories('/tmp/awesome-tests/empty')
        io.popen = function(...)  --luacheck: ignore
            return orig_popen('cd /tmp/awesome-tests/empty && ' .. ...)
        end
    end)

    teardown(function()
        os.remove('/tmp/awesome-tests/empty')
        io.popen = orig_popen  --luacheck: ignore
    end)

    it("handles completion of true", function()
        assert.same(shell('true', 5, 1, 'zsh'), {'true', 5, {'true'}})
        assert.same(shell('true', 5, 1, 'bash'), {'true', 5, {'true'}})
        assert.same(shell('true', 5, 1, nil), {'true', 5, {'true'}})
    end)
end)

describe("awful.completion.shell", function()
    local orig_popen = io.popen

    setup(function()
        gfs.make_directories('/tmp/awesome-tests/dir')
        os.execute('cd /tmp/awesome-tests/dir && touch localcommand && chmod +x localcommand')
        io.popen = function(...)  --luacheck: ignore
            return orig_popen('cd /tmp/awesome-tests/dir && ' .. ...)
        end
    end)

    teardown(function()
        os.remove('/tmp/awesome-tests/dir')
        io.popen = orig_popen  --luacheck: ignore
    end)

    it("handles empty input", function()
        assert.same(shell('', 1, 1, 'zsh'), {'', 1})
        assert.same(shell('', 1, 1, 'bash'), {'', 1})
        assert.same(shell('', 1, 1, nil), {'', 1})
    end)

    it("completes local command", function()
        assert.same(shell('./localcomm', 12, 1, 'zsh'), {'./localcommand', 15, {'./localcommand'}})
        assert.same(shell('./localcomm', 12, 1, 'bash'), {'./localcommand', 15, {'./localcommand'}})
    end)

    it("completes local file", function()
        assert.same(shell('ls ', 4, 1, 'zsh'), {'ls localcommand', 16, {'localcommand'}})
        assert.same(shell('ls ', 4, 1, 'bash'), {'ls localcommand', 16, {'localcommand'}})
    end)
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
