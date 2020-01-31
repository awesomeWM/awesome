describe("awful.placement", function()
    -- tiny hack to make sure we don't get errors about connect_signal
    package.loaded["awful.screen"] = {}

    local place = require("awful.placement")

    it("awful.placement.closest_corner (top left)", function()
        local _, corner = place.closest_corner(
        {coords=function() return {x = 100, y=100} end},
        {include_sides = true, bounding_rect = {x=0, y=0, width=200, height=200}}
        )
        assert(corner == "top_left")
    end)
    it("awful.placement.closest_corner (bottom)", function()
        local _, corner = place.closest_corner(
        {coords=function() return {x = 100, y=200} end},
        {include_sides = true, bounding_rect = {x=0, y=0, width=200, height=200}}
        )
        assert(corner == "bottom")
    end)
    it("awful.placement.closest_corner (bottom right)", function()
        local _, corner = place.closest_corner(
        {coords=function() return {x = 200, y=200} end},
        {include_sides = true, bounding_rect = {x=0, y=0, width=200, height=200}}
        )
        assert(corner == "bottom_right")
    end)
end)
