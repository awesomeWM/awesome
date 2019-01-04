----------------------------------------------------------------------------
--- A notification popup widget.
--
-- This is the legacy notification widget. It was the default until Awesome
-- v4.3 but is now being deprecated in favor of a more flexible widget.
--
-- The reason for this is/was that this widget is inflexible and mutate the
-- state of the notification object in a way that hinder other notification
-- widgets.
--
-- If no other notification widget is specified, Awesome fallback to this
-- widget.
--
--@DOC_naughty_actions_EXAMPLE@
--
-- @author koniu &lt;gkusnierz@gmail.com&gt;
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2008 koniu
-- @copyright 2017 Emmanuel Lepage Vallee
-- @classmod naughty.layout.legacy
----------------------------------------------------------------------------

local capi      = { screen = screen, awesome = awesome }
local naughty   = require("naughty.core")
local screen    = require("awful.screen")
local button    = require("awful.button")
local beautiful = require("beautiful")
local surface   = require("gears.surface")
local gtable    = require("gears.table")
local wibox     = require("wibox")
local gfs       = require("gears.filesystem")
local timer     = require("gears.timer")
local gmath     = require("gears.math")
local cairo     = require("lgi").cairo
local util      = require("awful.util")

local function get_screen(s)
    return s and capi.screen[s]
end

-- This is a copy of the table found in `naughty.core`. The reason the copy
-- exists is to make sure there is only unidirectional coupling between the
-- legacy widget (this class) and `naughty.core`. Exposing the "raw"
-- notification list is also a bad design and might cause indices and position
-- corruption. While it cannot be removed from the public API (yet), it can at
-- least be blacklisted internally.
local current_notifications = setmetatable({}, {__mode = "k"})

screen.connect_for_each_screen(function(s)
    current_notifications[s] = {
        top_left = {},
        top_middle = {},
        top_right = {},
        bottom_left = {},
        bottom_middle = {},
        bottom_right = {},
    }
end)

-- Counter for the notifications
-- Required for later access via DBUS
local counter = 1

--- Evaluate desired position of the notification by index - internal
--
-- @param s Screen to use
-- @param position top_right | top_left | bottom_right | bottom_left
--   | top_middle | bottom_middle
-- @param idx Index of the notification
-- @param[opt] width Popup width.
-- @param height Popup height
-- @return Absolute position and index in { x = X, y = Y, idx = I } table
local function get_offset(s, position, idx, width, height)
    s = get_screen(s)
    local ws = s.workarea
    local v = {}
    idx = idx or #current_notifications[s][position] + 1
    width = width or current_notifications[s][position][idx].width

    -- calculate x
    if position:match("left") then
        v.x = ws.x + naughty.config.padding
    elseif position:match("middle") then
        v.x = ws.x + (ws.width / 2) - (width / 2)
    else
        v.x = ws.x + ws.width - (width + naughty.config.padding)
    end

    -- calculate existing popups' height
    local existing = 0
    for i = 1, idx-1, 1 do
        local n = current_notifications[s][position][i]

        -- `n` will not nil when there is too many notifications to fit in `s`
        if n then
            existing = existing + n.height + naughty.config.spacing
        end
    end

    -- calculate y
    if position:match("top") then
        v.y = ws.y + naughty.config.padding + existing
    else
        v.y = ws.y + ws.height - (naughty.config.padding + height + existing)
    end

    -- Find old notification to replace in case there is not enough room.
    -- This tries to skip permanent notifications (without a timeout),
    -- e.g. critical ones.
    local find_old_to_replace = function()
        for i = 1, idx-1 do
            local n = current_notifications[s][position][i]
            if n.timeout > 0 then
                return n
            end
        end
        -- Fallback to first one.
        return current_notifications[s][position][1]
    end

    -- if positioned outside workarea, destroy oldest popup and recalculate
    if v.y + height > ws.y + ws.height or v.y < ws.y then
        local n = find_old_to_replace()
        if n then
            n:destroy(naughty.notification_closed_reason.too_many_on_screen)
        end
        v = get_offset(s, position, idx, width, height)
    end

    return v
end

local escape_pattern = "[<>&]"
local escape_subs    = { ['<'] = "&lt;", ['>'] = "&gt;", ['&'] = "&amp;" }

-- Cache the markup
local function set_escaped_text(self)
    local text, title = self.text or "", self.title or ""

    if title then title = title .. "\n" else title = "" end

    local textbox = self.textbox

    local function set_markup(pattern, replacements)
        return textbox:set_markup_silently(string.format('<b>%s</b>%s', title, text:gsub(pattern, replacements)))
    end

    local function set_text()
        textbox:set_text(string.format('%s %s', title, text))
    end

    -- Since the title cannot contain markup, it must be escaped first so that
    -- it is not interpreted by Pango later.
    title = title:gsub(escape_pattern, escape_subs)
    -- Try to set the text while only interpreting <br>.
    if not set_markup("<br.->", "\n") then
        -- That failed, escape everything which might cause an error from pango
        if not set_markup(escape_pattern, escape_subs) then
            -- Ok, just ignore all pango markup. If this fails, we got some invalid utf8
            if not pcall(set_text) then
                textbox:set_markup("<i>&lt;Invalid markup or UTF8, cannot display message&gt;</i>")
            end
        end
    end
end

naughty.connect_signal("property::text" ,set_escaped_text)
naughty.connect_signal("property::title",set_escaped_text)


--- Re-arrange notifications according to their position and index - internal
--
-- @return None
local function arrange(s)
    -- {} in case the screen has been deleted
    for p in pairs(current_notifications[s] or {}) do
        for i,notification in pairs(current_notifications[s][p]) do
            local offset = get_offset(s, p, i, notification.width, notification.height)
            notification.box:geometry({ x = offset.x, y = offset.y })
        end
    end
end

local function update_size(notification)
    local n = notification
    local s = n.size_info
    local width = s.width
    local height = s.height
    local margin = s.margin

    -- calculate the width
    if not width then
        local w, _ = n.textbox:get_preferred_size(n.screen)
        width = w + (n.iconbox and s.icon_w + 2 * margin or 0) + 2 * margin
    end

    if width < s.actions_max_width then
        width = s.actions_max_width
    end

    if s.max_width then
        width = math.min(width, s.max_width)
    end


    -- calculate the height
    if not height then
        local w = width - (n.iconbox and s.icon_w + 2 * margin or 0) - 2 * margin
        local h = n.textbox:get_height_for_width(w, n.screen)
        if n.iconbox and s.icon_h + 2 * margin > h + 2 * margin then
            height = s.icon_h + 2 * margin
        else
            height = h + 2 * margin
        end
    end

    height = height + s.actions_total_height

    if s.max_height then
        height = math.min(height, s.max_height)
    end

    -- crop to workarea size if too big
    local workarea = n.screen.workarea
    local border_width = s.border_width or 0
    local padding = naughty.config.padding or 0
    if width > workarea.width - 2*border_width - 2*padding then
        width = workarea.width - 2*border_width - 2*padding
    end
    if height > workarea.height - 2*border_width - 2*padding then
        height = workarea.height - 2*border_width - 2*padding
    end

    -- set size in notification object
    n.height = height + 2*border_width
    n.width = width + 2*border_width
    local offset = get_offset(n.screen, n.position, n.idx, n.width, n.height)
    n.box:geometry({
        width = width,
        height = height,
        x = offset.x,
        y = offset.y,
    })

    -- update positions of other notifications
    arrange(n.screen)
end


local function cleanup(self, _ --[[reason]], keep_visible)
    -- It is not a legacy notification
    if not self.box then return end

    local scr = self.screen

    assert(current_notifications[scr][self.position][self.idx] == self)
    table.remove(current_notifications[scr][self.position], self.idx)

    if (not keep_visible) or (not scr) then
        self.box.visible = false
    end

    arrange(scr)
end

naughty.connect_signal("destroyed", cleanup)

--- The default notification GUI handler.
--
-- To disable this handler, use:
--
--    naughty.disconnect_signal(
--        "request::display", naughty.default_notification_handler
--    )
--
-- It looks like:
--
--@DOC_naughty_actions_EXAMPLE@
--
-- @tparam table notification The `naughty.notification` object.
-- @tparam table args Any arguments passed to the `naughty.notify` function,
--  including, but not limited to all `naughty.notification` properties.
-- @signalhandler naughty.default_notification_handler
function naughty.default_notification_handler(notification, args)

    -- If request::display is called more than once, simply make sure the wibox
    -- is visible.
    if notification.box then
        notification.box.visible = true
        return
    end

    local preset = notification.preset
    local text   = args.text or preset.text
    local title  = args.title or preset.title
    local s      = get_screen(args.screen or preset.screen or screen.focused())

    if not s then
        local err = "naughty.notify: there is no screen available to display the following notification:"
        err = string.format("%s title='%s' text='%s'", err, tostring(title or ""), tostring(text or ""))
        require("gears.debug").print_warning(err)
        return
    end

    local timeout       = args.timeout or preset.timeout
    local icon          = args.icon or preset.icon
    local icon_size     = args.icon_size or preset.icon_size
        or beautiful.notification_icon_size
    local ontop         = args.ontop or preset.ontop
    local hover_timeout = args.hover_timeout or preset.hover_timeout
    local position      = args.position or preset.position
    local actions       = args.actions
    local destroy_cb    = args.destroy

    notification.screen     = s
    notification.destroy_cb = destroy_cb
    notification.timeout    = timeout

    -- beautiful
    local font = args.font or preset.font or beautiful.notification_font or
        beautiful.font or capi.awesome.font
    local fg = args.fg or preset.fg or
        beautiful.notification_fg or beautiful.fg_normal or '#ffffff'
    local bg = args.bg or preset.bg or
        beautiful.notification_bg or beautiful.bg_normal or '#535d6c'
    local border_color = args.border_color or preset.border_color or
        beautiful.notification_border_color or beautiful.bg_focus or '#535d6c'
    local border_width = args.border_width or preset.border_width or
        beautiful.notification_border_width
    local shape = args.shape or preset.shape or
        beautiful.notification_shape
    local width = args.width or preset.width or
        beautiful.notification_width
    local height = args.height or preset.height or
        beautiful.notification_height
    local max_width = args.max_width or preset.max_width or
        beautiful.notification_max_width
    local max_height = args.max_height or preset.max_height or
        beautiful.notification_max_height
    local margin = args.margin or preset.margin or
        beautiful.notification_margin
    local opacity = args.opacity or preset.opacity or
        beautiful.notification_opacity

    -- replace notification if needed
    local reuse_box
    if args.replaces_id then
        local obj = naughty.get_by_id(args.replaces_id)
        if obj then
            -- destroy this and ...
            naughty.destroy(obj, naughty.notification_closed_reason.silent, true)
            reuse_box = obj.box
        end
        -- ... may use its ID
        if args.replaces_id <= counter then
            notification.id = args.replaces_id
        else
            counter = counter + 1
            notification.id = counter
        end
    else
        -- get a brand new ID
        counter = counter + 1
        notification.id = counter
    end

    notification.position = position

    -- hook destroy
    notification.timeout = timeout
    local die = notification.die

    local run = function ()
        if args.run then
            args.run(notification)
        else
            die(naughty.notification_closed_reason.dismissed_by_user)
        end
    end

    local hover_destroy = function ()
        if hover_timeout == 0 then
            die(naughty.notification_closed_reason.expired)
        else
            if notification.timer then notification.timer:stop() end
            notification.timer = timer { timeout = hover_timeout }
            notification.timer:connect_signal("timeout", function() die(naughty.notification_closed_reason.expired) end)
            notification.timer:start()
        end
    end

    -- create textbox
    local textbox = wibox.widget.textbox()
    local marginbox = wibox.container.margin()
    marginbox:set_margins(margin)
    marginbox:set_widget(textbox)
    textbox:set_valign("middle")
    textbox:set_font(font)

    notification.textbox = textbox
    set_escaped_text(notification)

    local actionslayout = wibox.layout.fixed.vertical()
    local actions_max_width = 0
    local actions_total_height = 0
    if actions then
        for action, callback in pairs(actions) do
            local actiontextbox = wibox.widget.textbox()
            local actionmarginbox = wibox.container.margin()
            actionmarginbox:set_margins(margin)
            actionmarginbox:set_widget(actiontextbox)
            actiontextbox:set_valign("middle")
            actiontextbox:set_font(font)
            actiontextbox:set_markup(string.format('☛ <u>%s</u>', action))
            -- calculate the height and width
            local w, h = actiontextbox:get_preferred_size(s)
            local action_height = h + 2 * margin
            local action_width = w + 2 * margin

            actionmarginbox:buttons(gtable.join(
                button({ }, 1, callback),
                button({ }, 3, callback)
                ))
            actionslayout:add(actionmarginbox)

            actions_total_height = actions_total_height + action_height
            if actions_max_width < action_width then
                actions_max_width = action_width
            end
        end
    end

    local size_info = {
        width = width,
        height = height,
        max_width = max_width,
        max_height = max_height,
        margin = margin,
        border_width = border_width,
        actions_max_width = actions_max_width,
        actions_total_height = actions_total_height,
    }

    -- create iconbox
    local iconbox = nil
    local iconmargin = nil
    if icon then
        -- Is this really an URI instead of a path?
        if type(icon) == "string" and string.sub(icon, 1, 7) == "file://" then
            icon = string.sub(icon, 8)
            -- urldecode URI path
            icon = string.gsub(icon, "%%(%x%x)", function(x) return string.char(tonumber(x, 16)) end )
        end
        -- try to guess icon if the provided one is non-existent/readable
        if type(icon) == "string" and not gfs.file_readable(icon) then
            icon = util.geticonpath(icon, naughty.config.icon_formats, naughty.config.icon_dirs, icon_size) or icon
        end

        -- is the icon file readable?
        local had_icon = type(icon) == "string"
        icon = surface.load_uncached_silently(icon)

        -- if we have an icon, use it
        if icon then
            iconbox = wibox.widget.imagebox()
            iconmargin = wibox.container.margin(iconbox, margin, margin, margin, margin)

            if max_height and icon:get_height() > max_height then
                icon_size = icon_size and math.min(max_height, icon_size) or max_height
            end

            if max_width and icon:get_width() > max_width then
                icon_size = icon_size and math.min(max_width, icon_size) or max_width
            end

            if icon_size and (icon:get_height() > icon_size or icon:get_width() > icon_size) then
                size_info.icon_scale_factor = icon_size / math.max(icon:get_height(),
                                          icon:get_width())

                size_info.icon_w = icon:get_width () * size_info.icon_scale_factor
                size_info.icon_h = icon:get_height() * size_info.icon_scale_factor

                local scaled =
                    cairo.ImageSurface(cairo.Format.ARGB32,
                        gmath.round(size_info.icon_w),
                        gmath.round(size_info.icon_h))

                local cr = cairo.Context(scaled)
                cr:scale(size_info.icon_scale_factor, size_info.icon_scale_factor)
                cr:set_source_surface(icon, 0, 0)
                cr:paint()
                icon = scaled
            else
                size_info.icon_w = icon:get_width ()
                size_info.icon_h = icon:get_height()
            end
            iconbox:set_resize(false)
            iconbox:set_image(icon)
        elseif had_icon then
            require("gears.debug").print_warning("Naughty: failed to load icon "..
                (args.icon or preset.icon).. "(title: "..title..")")
        end
    end
    notification.iconbox = iconbox

    -- create container wibox
    notification.box = wibox({ fg = fg,
                               bg = bg,
                               border_color = border_color,
                               border_width = border_width,
                               shape_border_color = shape and border_color,
                               shape_border_width = shape and border_width,
                               shape = shape,
                               type = "notification" })

    if reuse_box then
        notification.box = reuse_box
    end

    if hover_timeout then notification.box:connect_signal("mouse::enter", hover_destroy) end

    notification.size_info = size_info

    -- position the wibox
    update_size(notification)
    notification.box.ontop = ontop
    notification.box.opacity = opacity
    notification.box.visible = true

    -- populate widgets
    local layout = wibox.layout.fixed.horizontal()
    if iconmargin then
        layout:add(iconmargin)
    end
    layout:add(marginbox)

    local completelayout = wibox.layout.fixed.vertical()
    completelayout:add(layout)
    completelayout:add(actionslayout)
    notification.box:set_widget(completelayout)

    -- Setup the mouse events
    layout:buttons(gtable.join(button({}, 1, nil, run),
                                   button({}, 3, nil, function()
                                        die(naughty.notification_closed_reason.dismissed_by_user)
                                    end)))

    -- insert the notification to the table
    table.insert(current_notifications[s][notification.position], notification)

    if naughty.suspended and not args.ignore_suspend then
        notification.box.visible = false
    end
end

naughty.connect_signal("request::display", naughty.default_notification_handler)
