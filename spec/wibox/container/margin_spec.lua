---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local base = require('wibox.widget.base')
local margin = require("wibox.container.margin")
local imagebox = require("wibox.widget.imagebox")
local utils = require("wibox.test_utils")

describe("wibox.container.margin", function()
    it("common interfaces", function()
        utils.test_container(margin())
    end)

    describe("composite widgets", function()
        it("can be wrapped with child", function()
            local widget_name = "test_widget"
            local new = function()
                local ret = base.make_widget_declarative {
                    {
                        id = "img",
                        widget = imagebox,
                    },
                    widget = margin,
                }

                ret.widget_name = widget_name

                return ret
            end

            local widget = base.make_widget_declarative {
                widget = new,
            }

            assert.is.equal(
                widget_name,
                widget.widget_name,
                "Widget name doesn't match"
            )
            local children = widget:get_children()
            assert.is_not.Nil(children, "Widget doesn't have children")
            assert.is.equal(
                1,
                #children,
                "Widget should have exactly one child"
            )
            assert.is.True(
                children[1].is_widget,
                "Child widget should be a valid widget"
            )
            assert.is.equal(
                widget.img,
                children[1],
                "Child widget should match the id accessor"
            )
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
