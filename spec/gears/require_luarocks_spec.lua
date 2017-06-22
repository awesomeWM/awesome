---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local io_mock

-- Mock require(): It simply returns from require_results, or errors out if
-- nothing found.
local require_results = {}
local own_require = function(module)
    return assert(require_results[module], "tried to require " .. tostring(module))
end

local original_path, original_cpath = package.path, package.cpath

describe("gears.require_luarocks", function()
    local function load_require_luarocks()
        -- Make sure the module is reloaded and starts in its initial state
        package.loaded["gears.require_luarocks"] = nil

        -- Replace require and io for the module with our mocks
        local original_require = require
        local original_io = io
        if io_mock then
            _G.io = io_mock
        end
        _G.require = own_require

        local require_luarocks = original_require("gears.require_luarocks")

        _G.io = original_io
        _G.require = original_require

        return require_luarocks
    end

    describe("require", function()
        local wrote_to_stderr
        before_each(function()
            wrote_to_stderr = false
            io_mock = {
                stderr = {
                    write = function()
                        wrote_to_stderr = true
                    end
                }
            }
        end)

        local called
        local function mock_add_luarocks_path(require_luarocks)
            called = false
            require_luarocks.add_luarocks_path = function()
                called = true
            end
        end

        it("successful require", function()
            local require_luarocks = load_require_luarocks()
            mock_add_luarocks_path(require_luarocks)

            local result = {}
            require_results.foo = result
            assert.is.equal(result, require_luarocks.require("foo"))
            assert.is_false(called)
            assert.is_false(wrote_to_stderr)
        end)

        it("unsuccessful require", function()
            local require_luarocks = load_require_luarocks()
            mock_add_luarocks_path(require_luarocks)

            assert.has_error(function()
                require_luarocks.require("bar")
            end)
            assert.is_true(called)
            assert.is_true(wrote_to_stderr)
        end)

        it("successful after add_luarocks_path", function()
            local require_luarocks = load_require_luarocks()

            local result = {}
            require_luarocks.add_luarocks_path = function()
                require_results.baz = result
                called = true
            end
            assert.is.equal(result, require_luarocks.require("baz"))
            assert.is_true(called)
            assert.is_true(wrote_to_stderr)
        end)
    end)

    describe("add_luarocks_path", function()
        -- Mock io as needed
        local popen_results, open_count
        before_each(function()
            popen_results = {}
            open_count = 0
            io_mock = {
                popen = function(cmd)
                    local result = popen_results[cmd]
                    if result == false then
                        -- Simulate popen failing
                        return nil
                    end
                    assert(result, "popen(" .. tostring(cmd) .. ") unexpected")
                    local self
                    open_count = open_count + 1
                    self = {
                        read = function(self2, arg)
                            assert(self == self2)
                            assert(arg == "*a", arg)
                            return result
                        end,
                        close = function()
                            open_count = open_count - 1
                        end
                    }
                    return self
                end
            }
        end)

        after_each(function()
            package.path = original_path
            package.cpath = original_cpath
        end)

        it("with luarocks", function()
            local require_luarocks = load_require_luarocks()

            -- First time it is called: Should actually do something
            popen_results["luarocks path --lr-path"] = "lr-path\n"
            popen_results["luarocks path --lr-cpath"] = "lr-cpath\n"
            require_luarocks.add_luarocks_path()

            assert.is.equal(original_path .. ";lr-path", package.path)
            assert.is.equal(original_cpath .. ";lr-cpath", package.cpath)
            assert.is.equal(0, open_count)

            -- Second time: Should be a no-op
            popen_results = {}
            require_luarocks.add_luarocks_path()

            assert.is.equal(original_path .. ";lr-path", package.path)
            assert.is.equal(original_cpath .. ";lr-cpath", package.cpath)
            assert.is.equal(0, open_count)
        end)

        it("without luarocks", function()
            local require_luarocks = load_require_luarocks()

            -- If LuaRocks is not available: Should do nothing
            popen_results["luarocks path --lr-path"] = ""
            popen_results["luarocks path --lr-cpath"] = ""
            require_luarocks.add_luarocks_path()

            assert.is.equal(original_path, package.path)
            assert.is.equal(original_cpath, package.cpath)
            assert.is.equal(0, open_count)
        end)

        it("popen fails", function()
            local require_luarocks = load_require_luarocks()

            -- If LuaRocks is not available: Should do nothing
            popen_results["luarocks path --lr-path"] = false
            popen_results["luarocks path --lr-cpath"] = false
            require_luarocks.add_luarocks_path()

            assert.is.equal(original_path, package.path)
            assert.is.equal(original_cpath, package.cpath)
            assert.is.equal(0, open_count)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
