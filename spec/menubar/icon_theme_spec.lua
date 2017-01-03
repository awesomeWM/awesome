---------------------------------------------------------------------------
-- @author Kazunobu Kuriyama
-- @copyright 2015 Kazunobu Kuriyama
---------------------------------------------------------------------------

local os = os
local string = string
local icon_theme = require("menubar.icon_theme")

local base_directories = {
    (os.getenv("SOURCE_DIRECTORY") or '.') .. "/spec/menubar/icons",
    (os.getenv("SOURCE_DIRECTORY") or '.') .. "/icons"
}

describe("menubar.icon_theme find_icon_path", function()
    local obj
    before_each(function()
        obj = icon_theme("awesome", base_directories)
    end)

    -- Invalid arguments for `icon_name`
    it("nil", function()
        assert.is_nil(obj:find_icon_path(nil))
    end)
    it ('""', function()
        assert.is.equal(obj:find_icon_path(""), nil)
    end)

    -- hierarchical folder (icon theme) cases
    for _, v in ipairs({16, 32, 48, 64}) do
        it(v, function()
            local expected = string.format("%s/awesome/%dx%d/apps/awesome.png",
                                           base_directories[1], v, v)
            assert.is.same(expected, obj:find_icon_path("awesome", v))
        end)
    end

    -- hierarchical folder (icon theme) cases w/ fallback
    for _, v in ipairs({16, 32, 48, 64}) do
        it(v, function()
            local expected = string.format("%s/fallback.png",
                                           base_directories[1])
            assert.is.same(expected, obj:find_icon_path("fallback", v))
        end)
    end

    -- flat folder/fallback cases
    local fallback_cases = {
        "awesome16",
        "awesome32",
        "awesome48",
        "awesome64"
    }
    for _, v in ipairs(fallback_cases) do
        it(v, function()
            local expected = string.format("%s/%s.png",
                                           base_directories[2], v)
            assert.is.same(expected, obj:find_icon_path(v))
       end)
    end
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
