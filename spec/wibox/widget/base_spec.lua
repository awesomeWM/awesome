---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2016 Uli Schlachter
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local no_parent = base.no_parent_I_know_what_I_am_doing

describe("wibox.widget.base", function()
    local widget1, widget2
    before_each(function()
        widget1 = base.make_widget()
        widget2 = base.make_widget()
        widget1.layout = function()
            return { base.place_widget_at(widget2, 0, 0, 1, 1) }
        end
    end)

    describe("caches", function()
        it("garbage collectable", function()
            local alive = setmetatable({ widget1, widget2 }, { __mode = "kv" })
            assert.is.equal(2, #alive)

            widget1, widget2 = nil, nil
            collectgarbage("collect")
            assert.is.equal(0, #alive)
        end)

        it("simple cache clear", function()
            local alive = setmetatable({ widget1, widget2 }, { __mode = "kv" })
            base.layout_widget(no_parent, { "fake context" }, widget1, 20, 20)
            assert.is.equal(2, #alive)

            widget1, widget2 = nil, nil
            collectgarbage("collect")
            assert.is.equal(0, #alive)
        end)

        it("self-reference cache clear", function()
            widget2.widget = widget1

            local alive = setmetatable({ widget1, widget2 }, { __mode = "kv" })
            base.layout_widget(no_parent, { "fake context" }, widget1, 20, 20)
            assert.is.equal(2, #alive)

            widget1, widget2 = nil, nil
            collectgarbage("collect")
            assert.is.equal(0, #alive)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
