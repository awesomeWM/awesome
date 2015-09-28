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

local boxes = nil

local function update(w, screen)
    local layout = layout.getname(layout.get(screen))
    w._layoutbox_tooltip:set_text(layout or "[no name]")
    w:set_image(layout and beautiful["layout_" .. layout])
end

local function update_from_tag(t)
    local screen = tag.getscreen(t)
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
    local screen = screen or 1

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
