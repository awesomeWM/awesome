describe("awful.permissions.client_geometry_requests", function()
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
    _G.tag = {
        connect_signal = function() end,
    }
    _G.awesome = {
        api_level      = 4,
        connect_signal = function() end,
    }
    _G.drawin = {
        set_index_miss_handler    = function() end,
        set_newindex_miss_handler = function() end
    }

    local permissions = require("awful.permissions")

    it("removes x/y/width/height when immobilized", function()
        local c = {_private={}}
        local s = stub.new(c, "geometry")

        permissions.client_geometry_requests(c, "ewmh", {})
        assert.stub(s).was_called_with(c, {})

        permissions.client_geometry_requests(c, "ewmh", {x=0, width=400})
        assert.stub(s).was_called_with(c, {x=0, width=400})

        c.immobilized_horizontal = true
        c.immobilized_vertical = false
        permissions.client_geometry_requests(c, "ewmh", {x=0, width=400})
        assert.stub(s).was_called_with(c, {})

        permissions.client_geometry_requests(c, "ewmh", {x=0, width=400, y=0})
        assert.stub(s).was_called_with(c, {y=0})

        c.immobilized_horizontal = true
        c.immobilized_vertical = true
        permissions.client_geometry_requests(c, "ewmh", {x=0, width=400, y=0})
        assert.stub(s).was_called_with(c, {})

        c.immobilized_horizontal = false
        c.immobilized_vertical = true
        local hints = {x=0, width=400, y=0}
        permissions.client_geometry_requests(c, "ewmh", hints)
        assert.stub(s).was_called_with(c, {x=0, width=400})
        -- Table passed as argument should not have been modified.
        assert.is.same(hints, {x=0, width=400, y=0})
    end)

    describe("tag", function()
        local function make_client(args)
            args = args or {}

            local ret = {
                screen = args.screen,
                sticky = args.sticky or false,
                transient_for = args.transient_for,
                _tags = args.tags or {},
            }

            function ret:tags(tags)
                if tags then
                    self._tags = tags
                end

                return self._tags
            end

            return ret
        end

        it("makes transients from sticky parents sticky", function()
            local s = {}
            local parent_tag = { screen = s }

            local parent = make_client {
                screen = s,
                sticky = true,
                tags = { parent_tag },
            }
            local child = make_client {
                screen = s,
                transient_for = parent,
            }

            permissions.tag(child)

            assert.is_true(child.sticky)
            assert.is.same({ parent_tag }, child:tags())
        end)

        it("places already-sticky transients with their parent", function()
            local s = {}
            local parent_tag = { screen = s }

            local parent = make_client {
                screen = s,
                tags = { parent_tag },
            }
            local child = make_client {
                screen = s,
                sticky = true,
                transient_for = parent,
            }

            permissions.tag(child)

            assert.is_true(child.sticky)
            assert.is.same({ parent_tag }, child:tags())
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
