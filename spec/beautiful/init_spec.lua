---------------------------------------------------------------------------
-- @author
-- @copyright 2019
---------------------------------------------------------------------------

local beautiful = require("beautiful")
local gdebug = require("gears.debug")

describe("beautiful init", function()
    local dir = (os.getenv("SOURCE_DIRECTORY") or '.') .. "/spec/beautiful/tests/"
    local shim

    -- Check beautiful.get_font and get_merged_font
    it('Check beautiful.get_font', function()
        assert.is_same(beautiful.get_font("Monospace Bold 12"),
            beautiful.get_merged_font("Monospace 12", "Bold"))
    end)

    -- Check beautiful.get_font_height
    it('Check beautiful.get_font_height', function()
        -- TODO: Will requires lgi-dpi
    end)

    -- Check beautiful.init
    it('Check beautiful.init', function()
        -- Check the error messages (needs a shim)
        shim = gdebug.print_error
        gdebug.print_error = function(message) error(message) end

        assert.has_error(function() beautiful.init({}) end,
            "beautiful: error loading theme: got an empty table")
        assert.has_error(function() beautiful.init(dir .. "Bad_1.lua") end,
            "beautiful: error loading theme: got an empty table from: " .. dir .. "Bad_1.lua")
        assert.has_error(function() beautiful.init(dir .. "Bad_2.lua") end,
            "beautiful: error loading theme: got a function from: " .. dir .. "Bad_2.lua")
        assert.has_error(function() beautiful.init(dir .. "Bad_3.lua") end,
            "beautiful: error loading theme: got a number from: " .. dir .. "Bad_3.lua")
        assert.has_error(function() beautiful.init(dir .. "Bad_4.lua") end,
            "beautiful: error loading theme: got a nil from: " .. dir .. "Bad_4.lua")
        assert.has_error(function() beautiful.init(dir .. "Bad_5.lua") end,
            "beautiful: error loading theme: got a nil from: " .. dir .. "Bad_5.lua")
        assert.has_error(function() beautiful.init(dir .. "NO_FILE") end,
            "beautiful: error loading theme: got a nil from: " .. dir .. "NO_FILE")

        assert.has_no_error(function() beautiful.init({ font = "Monospace Bold 12" }) end, "")
        assert.has_no_error(function() beautiful.init(dir .. "Good.lua") end, "")

        -- Check the return values (remove the shim)
        gdebug.print_error = function() end

        assert.is_nil(beautiful.init({}))
        assert.is_nil(beautiful.init(dir .. "Bad_1.lua"))
        assert.is_nil(beautiful.init(dir .. "Bad_2.lua"))
        assert.is_nil(beautiful.init(dir .. "Bad_3.lua"))
        assert.is_nil(beautiful.init(dir .. "Bad_4.lua"))
        assert.is_nil(beautiful.init(dir .. "Bad_5.lua"))
        assert.is_nil(beautiful.init(dir .. "NO_FILE"))

        gdebug.print_error = shim

        assert.is_true(beautiful.init({ font = "Monospace Bold 12" }))
        assert.is_true(beautiful.init(dir .. "Good.lua"))
    end)

    -- Check beautiful.get
    it('Check beautiful.get', function()
        assert.is_same(beautiful.get(), { font = "Monospace Bold 12" })
    end)

    -- Check getting a beautiful.value
    it('Check getting a beautiful.value', function()
        assert.is_same(beautiful.font, "Monospace Bold 12")
    end)

    -- Check setting a beautiful.value
    it('Check setting a beautiful.value', function()
        beautiful.font = "Monospace Bold 10"
        assert.is_same(beautiful.font, "Monospace Bold 10")
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
