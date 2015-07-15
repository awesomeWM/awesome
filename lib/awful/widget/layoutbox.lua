---------------------------------------------------------------------------
--- Layoutbox widget.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @classmod awful.widget.layoutbox
---------------------------------------------------------------------------

local setmetatable = setmetatable
local ipairs = ipairs
local button = require("awful.button")
local layout = require("awful.layout")
local tooltip = require("awful.tooltip")
local tag = require("awful.tag")
local beautiful = require("beautiful")
local imagebox = require("wibox.widget.imagebox")

local layoutbox = { mt = {} }

local function update(w, screen, tooltip)
    local layout = layout.getname(layout.get(screen))
    tooltip:set_text(layout)
    w:set_image(layout and beautiful["layout_" .. layout])
end

--- Create a layoutbox widget. It draws a picture with the current layout
-- symbol of the current tag.
-- @param screen The screen number that the layout will be represented for.
-- @return An imagebox widget configured as a layoutbox.
function layoutbox.new(screen)
    local screen = screen or 1
    local w = imagebox()
    local tooltip = tooltip({ objects = {w}, delay_show = 1 })

    update(w, screen, tooltip)

    local function update_on_tag_selection(t)
        return update(w, tag.getscreen(t), tooltip)
    end

    tag.attached_connect_signal(screen, "property::selected", update_on_tag_selection)
    tag.attached_connect_signal(screen, "property::layout", update_on_tag_selection)

    return w
end

function layoutbox.mt:__call(...)
    return layoutbox.new(...)
end

return setmetatable(layoutbox, layoutbox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
