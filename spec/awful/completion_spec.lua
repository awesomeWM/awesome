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

-- vim: ft=lua:et:sw=4:ts=8:sts=4:tw=80
