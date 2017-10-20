describe("awful.ewmh.client_geometry_requests", function()
    package.loaded["awful.client"] = {}
    package.loaded["awful.layout"] = {}
    package.loaded["awful.screen"] = {}
    package.loaded["awful.tag"] = {}
    package.loaded["gears.timer"] = {}
    _G.client = {
        connect_signal = function() end,
        get = function() return {} end,
    }
    _G.screen = {
        connect_signal = function() end,
    }

    local ewmh = require("awful.ewmh")

    it("removes x/y/width/height when immobilized", function()
        local c = {}
        local s = stub.new(c, "geometry")

        ewmh.client_geometry_requests(c, "ewmh", {})
        assert.stub(s).was_called_with(c, {})

        ewmh.client_geometry_requests(c, "ewmh", {x=0, width=400})
        assert.stub(s).was_called_with(c, {x=0, width=400})

        c.immobilized_horizontal = true
        c.immobilized_vertical = false
        ewmh.client_geometry_requests(c, "ewmh", {x=0, width=400})
        assert.stub(s).was_called_with(c, {})

        ewmh.client_geometry_requests(c, "ewmh", {x=0, width=400, y=0})
        assert.stub(s).was_called_with(c, {y=0})

        c.immobilized_horizontal = true
        c.immobilized_vertical = true
        ewmh.client_geometry_requests(c, "ewmh", {x=0, width=400, y=0})
        assert.stub(s).was_called_with(c, {})

        c.immobilized_horizontal = false
        c.immobilized_vertical = true
        local hints = {x=0, width=400, y=0}
        ewmh.client_geometry_requests(c, "ewmh", hints)
        assert.stub(s).was_called_with(c, {x=0, width=400})
        -- Table passed as argument should not have been modified.
        assert.is.same(hints, {x=0, width=400, y=0})
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
