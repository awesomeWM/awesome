---------------------------------------------------------------------------
--- Layoutbox widget.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @classmod awful.widget.layoutbox
---------------------------------------------------------------------------

local setmetatable = setmetatable
local capi = { screen = screen, tag = tag }
local layout = require("awful.layout")
local tooltip = require("awful.tooltip")
local beautiful = require("beautiful")
local wibox = require("wibox")
local surface = require("gears.surface")

local function get_screen(s)
    return s and capi.screen[s]
end

local layoutbox = { mt = {} }

local boxes = nil

local function update(w, screen)
    screen = get_screen(screen)
    local name = layout.getname(layout.get(screen))
    w._layoutbox_tooltip:set_text(name or "[no name]")

    local img = surface.load_silently(beautiful["layout_" .. name], false)
    w.imagebox.image = img
    w.textbox.text   = img and "" or name
end

local function update_from_tag(t)
    local screen = get_screen(t.screen)
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
        boxes = setmetatable({}, { __mode = "kv" })
        capi.tag.connect_signal("property::selected", update_from_tag)
        capi.tag.connect_signal("property::layout", update_from_tag)
        capi.tag.connect_signal("property::screen", function()
            for s, w in pairs(boxes) do
                if s.valid then
                    update(w, s)
                end
            end
        end)
        layoutbox.boxes = boxes
    end

    -- Do we already have a layoutbox for this screen?
    local w = boxes[screen]
    if not w then
        w = wibox.widget {
            {
                id     = "imagebox",
                widget = wibox.widget.imagebox
            },
            {
                id     = "textbox",
                widget = wibox.widget.textbox
            },
            layout = wibox.layout.fixed.horizontal
        }

        w._layoutbox_tooltip = tooltip {objects = {w}, delay_show = 1}

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
