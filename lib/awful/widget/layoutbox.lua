---------------------------------------------------------------------------
--- Layoutbox widget.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @classmod awful.widget.layoutbox
---------------------------------------------------------------------------

local setmetatable = setmetatable
local capi = { screen = screen }
local layout = require("awful.layout")
local tooltip = require("awful.tooltip")
local tag = require("awful.tag")
local beautiful = require("beautiful")
local imagebox = require("wibox.widget.imagebox")

local function get_screen(s)
    return s and capi.screen[s]
end

local layoutbox = { mt = {} }

local boxes = nil

local function update(w, screen)
    screen = get_screen(screen)
    local name = layout.getname(layout.get(screen and screen.index))
    w._layoutbox_tooltip:set_text(name or "[no name]")
    w:set_image(name and beautiful["layout_" .. name])
end

local function update_from_tag(t)
    local screen = get_screen(tag.getscreen(t))
    local w = boxes[screen]
    if w then
        update(w, screen)
    end
end

--- Create a layoutbox widget. It draws a picture with the current layout
-- symbol of the current tag.
-- @param screen The screen number that the layout will be represented for.
-- @return An imagebox widget configured as a layoutbox.
function layoutbox.new(screen)
    screen = get_screen(screen or 1)

    -- Do we already have the update callbacks registered?
    if boxes == nil then
        boxes = setmetatable({}, { __mode = "v" })
        tag.attached_connect_signal(nil, "property::selected", update_from_tag)
        tag.attached_connect_signal(nil, "property::layout", update_from_tag)
        layoutbox.boxes = boxes
    end

    -- Do we already have a layoutbox for this screen?
    local w = boxes[screen]
    if not w then
        w = imagebox()
        w._layoutbox_tooltip = tooltip({ objects = {w}, delay_show = 1 })

        update(w, screen)
        boxes[screen] = w
    end

    return w
end

function layoutbox.mt:__call(...)
    return layoutbox.new(...)
end

return setmetatable(layoutbox, layoutbox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
