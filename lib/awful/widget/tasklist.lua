---------------------------------------------------------------------------
--- Tasklist widget module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @classmod awful.widget.tasklist
---------------------------------------------------------------------------

-- Grab environment we need
local capi = { screen = screen,
               client = client }
local ipairs = ipairs
local setmetatable = setmetatable
local table = table
local common = require("awful.widget.common")
local beautiful = require("beautiful")
local client = require("awful.client")
local util = require("awful.util")
local tag = require("awful.tag")
local flex = require("wibox.layout.flex")
local timer = require("gears.timer")

local function get_screen(s)
    return s and screen[s]
end

local tasklist = { mt = {} }

local instances

-- Public structures
tasklist.filter = {}

local function tasklist_label(c, args, tb)
    if not args then args = {} end
    local theme = beautiful.get()
    local fg_normal = util.ensure_pango_color(args.fg_normal or theme.tasklist_fg_normal or theme.fg_normal, "white")
    local bg_normal = args.bg_normal or theme.tasklist_bg_normal or theme.bg_normal or "#000000"
    local fg_focus = util.ensure_pango_color(args.fg_focus or theme.tasklist_fg_focus or theme.fg_focus, fg_normal)
    local bg_focus = args.bg_focus or theme.tasklist_bg_focus or theme.bg_focus or bg_normal
    local fg_urgent = util.ensure_pango_color(args.fg_urgent or theme.tasklist_fg_urgent or theme.fg_urgent, fg_normal)
    local bg_urgent = args.bg_urgent or theme.tasklist_bg_urgent or theme.bg_urgent or bg_normal
    local fg_minimize = util.ensure_pango_color(args.fg_minimize or theme.tasklist_fg_minimize or theme.fg_minimize, fg_normal)
    local bg_minimize = args.bg_minimize or theme.tasklist_bg_minimize or theme.bg_minimize or bg_normal
    local bg_image_normal = args.bg_image_normal or theme.bg_image_normal
    local bg_image_focus = args.bg_image_focus or theme.bg_image_focus
    local bg_image_urgent = args.bg_image_urgent or theme.bg_image_urgent
    local bg_image_minimize = args.bg_image_minimize or theme.bg_image_minimize
    local tasklist_disable_icon = args.tasklist_disable_icon or theme.tasklist_disable_icon or false
    local font = args.font or theme.tasklist_font or theme.font or ""
    local font_focus = args.font_focus or theme.tasklist_font_focus or theme.font_focus or font or ""
    local font_minimized = args.font_minimized or theme.tasklist_font_minimized or theme.font_minimized or font or ""
    local font_urgent = args.font_urgent or theme.tasklist_font_urgent or theme.font_urgent or font or ""
    local text = ""
    local name = ""
    local bg
    local bg_image

    -- symbol to use to indicate certain client properties
    local sticky = args.sticky or theme.tasklist_sticky or "▪"
    local ontop = args.ontop or theme.tasklist_ontop or '⌃'
    local above = args.above or theme.tasklist_above or '▴'
    local below = args.below or theme.tasklist_below or '▾'
    local floating = args.floating or theme.tasklist_floating or '✈'
    local maximized = args.maximized or theme.tasklist_maximized or '<b>+</b>'
    local maximized_horizontal = args.maximized_horizontal or theme.tasklist_maximized_horizontal or '⬌'
    local maximized_vertical = args.maximized_vertical or theme.tasklist_maximized_vertical or '⬍'

    if not theme.tasklist_plain_task_name then
        if c.sticky then name = name .. sticky end

        if c.ontop then name = name .. ontop
        elseif c.above then name = name .. above
        elseif c.below then name = name .. below end

        if c.maximized then
            name = name .. maximized
        else
            if c.maximized_horizontal then name = name .. maximized_horizontal end
            if c.maximized_vertical then name = name .. maximized_vertical end
            if client.floating.get(c) then name = name .. floating end
        end
    end

    if c.minimized then
        name = name .. (util.escape(c.icon_name) or util.escape(c.name) or util.escape("<untitled>"))
    else
        name = name .. (util.escape(c.name) or util.escape("<untitled>"))
    end
    local focused = capi.client.focus == c
    -- Handle transient_for: the first parent that does not skip the taskbar
    -- is considered to be focused, if the real client has skip_taskbar.
    if not focused and capi.client.focus and capi.client.focus.skip_taskbar
        and client.get_transient_for_matching(capi.client.focus,
                                              function(cl)
                                                  return not cl.skip_taskbar
                                              end) == c then
        focused = true
    end
    if focused then
        bg = bg_focus
        text = text .. "<span color='"..fg_focus.."'>"..name.."</span>"
        bg_image = bg_image_focus
        font = font_focus
    elseif c.urgent then
        bg = bg_urgent
        text = text .. "<span color='"..fg_urgent.."'>"..name.."</span>"
        bg_image = bg_image_urgent
        font = font_urgent
    elseif c.minimized then
        bg = bg_minimize
        text = text .. "<span color='"..fg_minimize.."'>"..name.."</span>"
        bg_image = bg_image_minimize
        font = font_minimized
    else
        bg = bg_normal
        text = text .. "<span color='"..fg_normal.."'>"..name.."</span>"
        bg_image = bg_image_normal
    end
    tb:set_font(font)
    return text, bg, bg_image, not tasklist_disable_icon and c.icon or nil
end

local function tasklist_update(s, w, buttons, filter, data, style, update_function)
    local clients = {}
    for _, c in ipairs(capi.client.get()) do
        if not (c.skip_taskbar or c.hidden
            or c.type == "splash" or c.type == "dock" or c.type == "desktop")
            and filter(c, s) then
            table.insert(clients, c)
        end
    end

    local function label(c, tb) return tasklist_label(c, style, tb) end

    update_function(w, buttons, label, data, clients)
end

--- Create a new tasklist widget. The last two arguments (update_function
-- and base_widget) serve to customize the layout of the tasklist (eg. to
-- make it vertical). For that, you will need to copy the
-- awful.widget.common.list_update function, make your changes to it
-- and pass it as update_function here. Also change the base_widget if the
-- default is not what you want.
-- @param screen The screen to draw tasklist for.
-- @param filter Filter function to define what clients will be listed.
-- @param buttons A table with buttons binding to set.
-- @param style The style overrides default theme.
-- @param[opt] update_function Function to create a tag widget on each
--   update. See `awful.widget.common.list_update`.
-- @tparam[opt] table base_widget Container widget for tag widgets. Default
--   is `wibox.layout.flex.horizontal`.
-- @param base_widget.bg_normal The background color for unfocused client.
-- @param base_widget.bg_normal The background color for unfocused client.
-- @param base_widget.fg_normal The foreground color for unfocused client.
-- @param base_widget.bg_focus The background color for focused client.
-- @param base_widget.fg_focus The foreground color for focused client.
-- @param base_widget.bg_urgent The background color for urgent clients.
-- @param base_widget.fg_urgent The foreground color for urgent clients.
-- @param base_widget.bg_minimize The background color for minimized clients.
-- @param base_widget.fg_minimize The foreground color for minimized clients.
-- @param base_widget.floating Symbol to use for floating clients.
-- @param base_widget.ontop Symbol to use for ontop clients.
-- @param base_widget.above Symbol to use for clients kept above others.
-- @param base_widget.below Symbol to use for clients kept below others.
-- @param base_widget.maximized Symbol to use for clients that have been maximized (vertically and horizontally).
-- @param base_widget.maximized_horizontal Symbol to use for clients that have been horizontally maximized.
-- @param base_widget.maximized_vertical Symbol to use for clients that have been vertically maximized.
-- @param base_widget.font The font.
function tasklist.new(screen, filter, buttons, style, update_function, base_widget)
    screen = get_screen(screen)
    local uf = update_function or common.list_update
    local w = base_widget or flex.horizontal()

    local data = setmetatable({}, { __mode = 'k' })

    local queued_update = false
    function w._do_tasklist_update()
        -- Add a delayed callback for the first update.
        if not queued_update then
            timer.delayed_call(function()
                queued_update = false
                tasklist_update(screen, w, buttons, filter, data, style, uf)
            end)
            queued_update = true
        end
    end
    function w._unmanage(c)
        data[c] = nil
    end
    if instances == nil then
        instances = {}
        local function us(s)
            local i = instances[get_screen(s)]
            if i then
                for _, tlist in pairs(i) do
                    tlist._do_tasklist_update()
                end
            end
        end
        local function u()
            for s in pairs(instances) do
                us(s)
            end
        end

        tag.attached_connect_signal(nil, "property::selected", u)
        tag.attached_connect_signal(nil, "property::activated", u)
        capi.client.connect_signal("property::urgent", u)
        capi.client.connect_signal("property::sticky", u)
        capi.client.connect_signal("property::ontop", u)
        capi.client.connect_signal("property::above", u)
        capi.client.connect_signal("property::below", u)
        capi.client.connect_signal("property::floating", u)
        capi.client.connect_signal("property::maximized_horizontal", u)
        capi.client.connect_signal("property::maximized_vertical", u)
        capi.client.connect_signal("property::minimized", u)
        capi.client.connect_signal("property::name", u)
        capi.client.connect_signal("property::icon_name", u)
        capi.client.connect_signal("property::icon", u)
        capi.client.connect_signal("property::skip_taskbar", u)
        capi.client.connect_signal("property::screen", function(c, old_screen)
            us(c.screen)
            us(old_screen)
        end)
        capi.client.connect_signal("property::hidden", u)
        capi.client.connect_signal("tagged", u)
        capi.client.connect_signal("untagged", u)
        capi.client.connect_signal("unmanage", function(c)
            u(c)
            for _, i in pairs(instances) do
                for _, tlist in pairs(i) do
                    tlist._unmanage(c)
                end
            end
        end)
        capi.client.connect_signal("list", u)
        capi.client.connect_signal("focus", u)
        capi.client.connect_signal("unfocus", u)
    end
    w._do_tasklist_update()
    local list = instances[screen]
    if not list then
        list = setmetatable({}, { __mode = "v" })
        instances[screen] = list
    end
    table.insert(list, w)
    return w
end

--- Filtering function to include all clients.
-- @return true
function tasklist.filter.allscreen()
    return true
end

--- Filtering function to include the clients from all tags on the screen.
-- @param c The client.
-- @param screen The screen we are drawing on.
-- @return true if c is on screen, false otherwise
function tasklist.filter.alltags(c, screen)
    -- Only print client on the same screen as this widget
    return get_screen(c.screen) == get_screen(screen)
end

--- Filtering function to include only the clients from currently selected tags.
-- @param c The client.
-- @param screen The screen we are drawing on.
-- @return true if c is in a selected tag on screen, false otherwise
function tasklist.filter.currenttags(c, screen)
    screen = get_screen(screen)
    -- Only print client on the same screen as this widget
    if get_screen(c.screen) ~= screen then return false end
    -- Include sticky client too
    if c.sticky then return true end
    local tags = tag.gettags(screen.index)
    for _, t in ipairs(tags) do
        if t.selected then
            local ctags = c:tags()
            for _, v in ipairs(ctags) do
                if v == t then
                    return true
                end
            end
        end
    end
    return false
end

--- Filtering function to include only the minimized clients from currently selected tags.
-- @param c The client.
-- @param screen The screen we are drawing on.
-- @return true if c is in a selected tag on screen and is minimized, false otherwise
function tasklist.filter.minimizedcurrenttags(c, screen)
    screen = get_screen(screen)
    -- Only print client on the same screen as this widget
    if get_screen(c.screen) ~= screen then return false end
    -- Check client is minimized
    if not c.minimized then return false end
    -- Include sticky client
    if c.sticky then return true end
    local tags = tag.gettags(screen.index)
    for _, t in ipairs(tags) do
        -- Select only minimized clients
        if t.selected then
            local ctags = c:tags()
            for _, v in ipairs(ctags) do
                if v == t then
                    return true
                end
            end
        end
    end
    return false
end

--- Filtering function to include only the currently focused client.
-- @param c The client.
-- @param screen The screen we are drawing on.
-- @return true if c is focused on screen, false otherwise
function tasklist.filter.focused(c, screen)
    -- Only print client on the same screen as this widget
    return get_screen(c.screen) == get_screen(screen) and capi.client.focus == c
end

function tasklist.mt:__call(...)
    return tasklist.new(...)
end

return setmetatable(tasklist, tasklist.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
