----------------------------------------------------------------------------
--- Notification library
--
-- @author koniu &lt;gkusnierz@gmail.com&gt;
-- @copyright 2008 koniu
-- @release @AWESOME_VERSION@
-- @module naughty
----------------------------------------------------------------------------

-- Package environment
local pairs = pairs
local table = table
local type = type
local string = string
local pcall = pcall
local capi = { screen = screen,
               awesome = awesome }
local timer = require("gears.timer")
local button = require("awful.button")
local screen = require("awful.screen")
local util = require("awful.util")
local bt = require("beautiful")
local wibox = require("wibox")
local surface = require("gears.surface")
local cairo = require("lgi").cairo
local dpi = require("beautiful").xresources.apply_dpi

local function get_screen(s)
    return s and capi.screen[s]
end

local naughty = {}

--[[--
Naughty configuration - a table containing common popup settings.

@table naughty.config
@tfield[opt=apply_dpi(4)] int padding Space between popups and edge of the
  workarea.
@tfield[opt=apply_dpi(1)] int spacing Spacing between popups.
@tfield[opt={"/usr/share/pixmaps/"}] table icon_dirs List of directories
  that will be checked by `getIcon()`.
@tfield[opt={ "png", "gif" }] table icon_formats List of formats that will be
  checked by `getIcon()`.
@tfield[opt] function notify_callback Callback used to modify or reject
notifications, e.g.
    naughty.config.notify_callback = function(args)
        args.text = 'prefix: ' .. args.text
        return args
    end

@tfield table presets Notification presets.  See `config.presets`.

@tfield table defaults Default values for the params to `notify()`.  These can
  optionally be overridden by specifying a preset.  See `config.defaults`.

--]]
--
naughty.config = {
    padding = dpi(4),
    spacing = dpi(1),
    icon_dirs = { "/usr/share/pixmaps/", },
    icon_formats = { "png", "gif" },
    notify_callback = nil,
}

--- Notification presets for `naughty.notify`.
-- This holds presets for different purposes.  A preset is a table of any
-- parameters for `notify()`, overriding the default values
-- (`naughty.config.defaults`).
--
-- You have to pass a reference of a preset in your `notify()` as the `preset`
-- argument.
--
-- The presets `"low"`, `"normal"` and `"critical"` are used for notifications
-- over DBUS.
--
-- @table config.presets
-- @tfield table low The preset for notifications with low urgency level.
-- @tfield[opt=5] int low.timeout
-- @tfield[opt=empty] table normal The default preset for every notification without a
--   preset that will also be used for normal urgency level.
-- @tfield table critical The preset for notifications with a critical urgency
--   level.
-- @tfield[opt="#ff0000"] string critical.bg
-- @tfield[opt="#ffffff"] string critical.fg
-- @tfield[opt=0] string critical.timeout
naughty.config.presets = {
    low = {
        timeout = 5
    },
    normal = {},
    critical = {
        bg = "#ff0000",
        fg = "#ffffff",
        timeout = 0,
    }
}

--- Defaults for `naughty.notify`.
--
-- @table config.defaults
-- @tfield[opt=5] int timeout
-- @tfield[opt=""] string text
-- @tfield[opt] int screen Defaults to `awful.screen.focused`.
-- @tfield[opt=true] boolean ontop
-- @tfield[opt=apply_dpi(5)] int margin
-- @tfield[opt=apply_dpi(1)] int border_width
-- @tfield[opt="top_right"] string position
naughty.config.defaults = {
    timeout = 5,
    text = "",
    screen = nil,
    ontop = true,
    margin = dpi(5),
    border_width = dpi(1),
    position = "top_right"
}

naughty.notificationClosedReason = {
    silent = -1,
    expired = 1,
    dismissedByUser = 2,
    dismissedByCommand = 3,
    undefined = 4
}

-- Counter for the notifications
-- Required for later access via DBUS
local counter = 1

-- True if notifying is suspended
local suspended = false

--- Index of notifications per screen and position.
-- See config table for valid 'position' values.
-- Each element is a table consisting of:
--
-- @field box Wibox object containing the popup
-- @field height Popup height
-- @field width Popup width
-- @field die Function to be executed on timeout
-- @field id Unique notification id based on a counter
-- @table notifications
naughty.notifications = { suspended = { } }
for s = 1, capi.screen.count() do
    naughty.notifications[get_screen(s)] = {
        top_left = {},
        top_middle = {},
        top_right = {},
        bottom_left = {},
        bottom_middle = {},
        bottom_right = {},
    }
end

--- Notification state
function naughty.is_suspended()
    return suspended
end

--- Suspend notifications
function naughty.suspend()
    suspended = true
end

--- Resume notifications
function naughty.resume()
    suspended = false
    for _, v in pairs(naughty.notifications.suspended) do
        v.box.visible = true
        if v.timer then v.timer:start() end
    end
    naughty.notifications.suspended = { }
end

--- Toggle notification state
function naughty.toggle()
    if suspended then
        naughty.resume()
    else
        naughty.suspend()
    end
end

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
    idx = idx or #naughty.notifications[s][position] + 1
    width = width or naughty.notifications[s][position][idx].width

    -- calculate x
    if position:match("left") then
        v.x = ws.x + naughty.config.padding
    elseif position:match("middle") then
        v.x = (ws.width / 2) - (width / 2)
    else
        v.x = ws.x + ws.width - (width + naughty.config.padding)
    end

    -- calculate existing popups' height
    local existing = 0
    for i = 1, idx-1, 1 do
        existing = existing + naughty.notifications[s][position][i].height + naughty.config.spacing
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
            local n = naughty.notifications[s][position][i]
            if n.timeout > 0 then
                return n
            end
        end
        -- Fallback to first one.
        return naughty.notifications[s][position][1]
    end

    -- if positioned outside workarea, destroy oldest popup and recalculate
    if v.y + height > ws.y + ws.height or v.y < ws.y then
        naughty.destroy(find_old_to_replace())
        idx = idx - 1
        v = get_offset(s, position, idx, width, height)
    end
    if not v.idx then v.idx = idx end

    return v
end

--- Re-arrange notifications according to their position and index - internal
--
-- @return None
local function arrange(s)
    for p in pairs(naughty.notifications[s]) do
        for i,notification in pairs(naughty.notifications[s][p]) do
            local offset = get_offset(s, p, i, notification.width, notification.height)
            notification.box:geometry({ x = offset.x, y = offset.y })
            notification.idx = offset.idx
        end
    end
end

--- Destroy notification by notification object
--
-- @param notification Notification object to be destroyed
-- @param reason One of the reasons from notificationClosedReason
-- @return True if the popup was successfully destroyed, nil otherwise
function naughty.destroy(notification, reason)
    if notification and notification.box.visible then
        if suspended then
            for k, v in pairs(naughty.notifications.suspended) do
                if v.box == notification.box then
                    table.remove(naughty.notifications.suspended, k)
                    break
                end
            end
        end
        local scr = notification.screen
        table.remove(naughty.notifications[scr][notification.position], notification.idx)
        if notification.timer then
            notification.timer:stop()
        end
        notification.box.visible = false
        arrange(scr)
        if notification.destroy_cb and reason ~= naughty.notificationClosedReason.silent then
            notification.destroy_cb(reason or naughty.notificationClosedReason.undefined)
        end
        return true
    end
end

--- Get notification by ID
--
-- @param id ID of the notification
-- @return notification object if it was found, nil otherwise
function naughty.getById(id)
    -- iterate the notifications to get the notfications with the correct ID
    for s = 1, capi.screen.count() do
        s = get_screen(s)
        for p in pairs(naughty.notifications[s]) do
            for _, notification in pairs(naughty.notifications[s][p]) do
                if notification.id == id then
                    return notification
                 end
            end
        end
    end
end

--- Install expiration timer for notification object.
-- @tparam notification notification Notification object.
-- @tparam number timeout Time in seconds to be set as expiration timeout.
local function set_timeout(notification, timeout)
    local die = function (reason)
        naughty.destroy(notification, reason)
    end
    if timeout > 0 then
        local timer_die = timer { timeout = timeout }
        timer_die:connect_signal("timeout", function() die(naughty.notificationClosedReason.expired) end)
        if not suspended then
            timer_die:start()
        end
        notification.timer = timer_die
    end
    notification.die = die
end

--- Set new notification timeout.
-- @tparam notification notification Notification object, which timer is to be reset.
-- @tparam number new_timeout Time in seconds after which notification disappears.
-- @return None.
function naughty.reset_timeout(notification, new_timeout)
    if notification.timer then notification.timer:stop() end

    local timeout = new_timeout or notification.timeout
    set_timeout(notification, timeout)
    notification.timeout = timeout

    notification.timer:start()
end

--- Escape and set title and text for notification object.
-- @tparam notification notification Notification object.
-- @tparam string title Title of notification.
-- @tparam string text Main text of notification.
-- @return None.
local function set_text(notification, title, text)
    local escape_pattern = "[<>&]"
    local escape_subs = { ['<'] = "&lt;", ['>'] = "&gt;", ['&'] = "&amp;" }

    local textbox = notification.textbox

    local function setMarkup(pattern, replacements)
        return textbox:set_markup_silently(string.format('<b>%s</b>%s', title, text:gsub(pattern, replacements)))
    end
    local function setText()
        textbox:set_text(string.format('%s %s', title, text))
    end

    -- Since the title cannot contain markup, it must be escaped first so that
    -- it is not interpreted by Pango later.
    title = title:gsub(escape_pattern, escape_subs)
    -- Try to set the text while only interpreting <br>.
    if not setMarkup("<br.->", "\n") then
        -- That failed, escape everything which might cause an error from pango
        if not setMarkup(escape_pattern, escape_subs) then
            -- Ok, just ignore all pango markup. If this fails, we got some invalid utf8
            if not pcall(setText) then
                textbox:set_markup("<i>&lt;Invalid markup or UTF8, cannot display message&gt;</i>")
            end
        end
    end
end

--- Replace title and text of an existing notification.
-- @tparam notification notification Notification object, which contents are to be replaced.
-- @tparam string new_title New title of notification. If not specified, old title remains unchanged.
-- @tparam string new_text New text of notification. If not specified, old text remains unchanged.
-- @return None.
function naughty.replace_text(notification, new_title, new_text)
    local title = new_title

    if title then title = title .. "\n" else title = "" end

    set_text(notification, title, new_text)
end

--- Create a notification.
--
-- @tab args The argument table containing any of the arguments below.
-- @string[opt=""] args.text Text of the notification.
-- @string[opt] args.title Title of the notification.
-- @int[opt=5] args.timeout Time in seconds after which popup expires.
--   Set 0 for no timeout.
-- @int[opt] args.hover_timeout Delay in seconds after which hovered popup disappears.
-- @tparam[opt=focused] integer|screen args.screen Target screen for the notification.
-- @string[opt="top_right"] args.position Corner of the workarea displaying the popups.
--   Values: `"top_right"`, `"top_left"`, `"bottom_left"`,
--   `"bottom_right"`, `"top_middle"`, `"bottom_middle"`.
-- @bool[opt=true] args.ontop Boolean forcing popups to display on top.
-- @int[opt=auto] args.height Popup height.
-- @int[opt=auto] args.width Popup width.
-- @string[opt=beautiful.font or awesome.font] args.font Notification font.
-- @string[opt] args.icon Path to icon.
-- @int[opt] args.icon_size Desired icon size in px.
-- @string[opt=`beautiful.fg_focus` or `'#ffffff'`] args.fg Foreground color.
-- @string[opt=`beautiful.bg_focus` or `'#535d6c'`] args.bg Background color.
-- @int[opt=1] args.border_width Border width.
-- @string[opt=`beautiful.border_focus` or `'#535d6c'`] args.border_color Border color.
-- @tparam[opt] func args.run Function to run on left click.  The notification
--   object will be passed to it as an argument.
--   You need to call e.g.
--   `notification.die(naughty.notificationClosedReason.dismissedByUser)` from
--   there to dismiss the notification yourself.
-- @tparam[opt] func args.destroy Function to run when notification is destroyed.
-- @tparam[opt] table args.preset Table with any of the above parameters.
--   Note: Any parameters specified directly in args will override ones defined
--   in the preset.
-- @tparam[opt] int args.replaces_id Replace the notification with the given ID.
-- @tparam[opt] func args.callback Function that will be called with all arguments.
--   The notification will only be displayed if the function returns true.
--   Note: this function is only relevant to notifications sent via dbus.
-- @tparam[opt] table args.actions Mapping that maps a string to a callback when this
--   action is selected.
-- @usage naughty.notify({ title = "Achtung!", text = "You're idling", timeout = 0 })
-- @return The notification object
function naughty.notify(args)
    if naughty.config.notify_callback then
        args = naughty.config.notify_callback(args)
        if not args then return end
    end

    -- gather variables together
    local preset = util.table.join(naughty.config.defaults or {},
        args.preset or naughty.config.presets.normal or {})
    local timeout = args.timeout or preset.timeout
    local icon = args.icon or preset.icon
    local icon_size = args.icon_size or preset.icon_size
    local text = args.text or preset.text
    local title = args.title or preset.title
    local s = get_screen(args.screen or preset.screen or screen.focused())
    local ontop = args.ontop or preset.ontop
    local width = args.width or preset.width
    local height = args.height or preset.height
    local hover_timeout = args.hover_timeout or preset.hover_timeout
    local opacity = args.opacity or preset.opacity
    local margin = args.margin or preset.margin
    local border_width = args.border_width or preset.border_width
    local position = args.position or preset.position
    local actions = args.actions
    local destroy_cb = args.destroy

    -- beautiful
    local beautiful = bt.get()
    local font = args.font or preset.font or beautiful.font or capi.awesome.font
    local fg = args.fg or preset.fg or beautiful.fg_normal or '#ffffff'
    local bg = args.bg or preset.bg or beautiful.bg_normal or '#535d6c'
    local border_color = args.border_color or preset.border_color or beautiful.bg_focus or '#535d6c'
    local notification = { screen = s, destroy_cb = destroy_cb, timeout = timeout }

    -- replace notification if needed
    if args.replaces_id then
        local obj = naughty.getById(args.replaces_id)
        if obj then
            -- destroy this and ...
            naughty.destroy(obj, naughty.notificationClosedReason.silent)
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

    if title then title = title .. "\n" else title = "" end

    -- hook destroy
    set_timeout(notification, timeout)
    local die = notification.die

    local run = function ()
        if args.run then
            args.run(notification)
        else
            die(naughty.notificationClosedReason.dismissedByUser)
        end
    end

    local hover_destroy = function ()
        if hover_timeout == 0 then
            die(naughty.notificationClosedReason.expired)
        else
            if notification.timer then notification.timer:stop() end
            notification.timer = timer { timeout = hover_timeout }
            notification.timer:connect_signal("timeout", function() die(naughty.notificationClosedReason.expired) end)
            notification.timer:start()
        end
    end

    -- create textbox
    local textbox = wibox.widget.textbox()
    local marginbox = wibox.layout.margin()
    marginbox:set_margins(margin)
    marginbox:set_widget(textbox)
    textbox:set_valign("middle")
    textbox:set_font(font)

    notification.textbox = textbox

    set_text(notification, title, text)

    local actionslayout = wibox.layout.fixed.vertical()
    local actions_max_width = 0
    local actions_total_height = 0
    if actions then
        for action, callback in pairs(actions) do
            local actiontextbox = wibox.widget.textbox()
            local actionmarginbox = wibox.layout.margin()
            actionmarginbox:set_margins(margin)
            actionmarginbox:set_widget(actiontextbox)
            actiontextbox:set_valign("middle")
            actiontextbox:set_font(font)
            actiontextbox:set_markup(string.format('<b>%s</b>', action))
            -- calculate the height and width
            local w, h = actiontextbox:get_preferred_size(s)
            local action_height = h + 2 * margin
            local action_width = w + 2 * margin

            actionmarginbox:buttons(util.table.join(
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

    -- create iconbox
    local iconbox = nil
    local iconmargin = nil
    local icon_w, icon_h = 0, 0
    if icon then
        -- Is this really an URI instead of a path?
        if type(icon) == "string" and string.sub(icon, 1, 7) == "file://" then
            icon = string.sub(icon, 8)
        end
        -- try to guess icon if the provided one is non-existent/readable
        if type(icon) == "string" and not util.file_readable(icon) then
            icon = util.geticonpath(icon, naughty.config.icon_formats, naughty.config.icon_dirs, icon_size) or icon
        end

        -- is the icon file readable?
        icon = surface.load_uncached(icon)

        -- if we have an icon, use it
        if icon then
            iconbox = wibox.widget.imagebox()
            iconmargin = wibox.layout.margin(iconbox, margin, margin, margin, margin)
            if icon_size then
                local scaled = cairo.ImageSurface(cairo.Format.ARGB32, icon_size, icon_size)
                local cr = cairo.Context(scaled)
                cr:scale(icon_size / icon:get_height(), icon_size / icon:get_width())
                cr:set_source_surface(icon, 0, 0)
                cr:paint()
                icon = scaled
            end
            iconbox:set_resize(false)
            iconbox:set_image(icon)
            icon_w = icon:get_width()
            icon_h = icon:get_height()
        end
    end

    -- create container wibox
    notification.box = wibox({ fg = fg,
                               bg = bg,
                               border_color = border_color,
                               border_width = border_width,
                               type = "notification" })

    if hover_timeout then notification.box:connect_signal("mouse::enter", hover_destroy) end

    -- calculate the width
    if not width then
        local w, _ = textbox:get_preferred_size(s.index)
        width = w + (iconbox and icon_w + 2 * margin or 0) + 2 * margin
    end

    if width < actions_max_width then
        width = actions_max_width
    end

    -- calculate the height
    if not height then
        local w = width - (iconbox and icon_w + 2 * margin or 0) - 2 * margin
        local h = textbox:get_height_for_width(w, s.index)
        if iconbox and icon_h + 2 * margin > h + 2 * margin then
            height = icon_h + 2 * margin
        else
            height = h + 2 * margin
        end
    end

    height = height + actions_total_height

    -- crop to workarea size if too big
    local workarea = s.workarea
    if width > workarea.width - 2 * (border_width or 0) - 2 * (naughty.config.padding or 0) then
        width = workarea.width - 2 * (border_width or 0) - 2 * (naughty.config.padding or 0)
    end
    if height > workarea.height - 2 * (border_width or 0) - 2 * (naughty.config.padding or 0) then
        height = workarea.height - 2 * (border_width or 0) - 2 * (naughty.config.padding or 0)
    end

    -- set size in notification object
    notification.height = height + 2 * (border_width or 0)
    notification.width = width + 2 * (border_width or 0)

    -- position the wibox
    local offset = get_offset(s, notification.position, nil, notification.width, notification.height)
    notification.box.ontop = ontop
    notification.box:geometry({ width = width,
                                height = height,
                                x = offset.x,
                                y = offset.y })
    notification.box.opacity = opacity
    notification.box.visible = true
    notification.idx = offset.idx

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
    layout:buttons(util.table.join(button({ }, 1, run),
                                   button({ }, 3, function()
                                        die(naughty.notificationClosedReason.dismissedByUser)
                                    end)))

    -- insert the notification to the table
    table.insert(naughty.notifications[s][notification.position], notification)

    if suspended then
        notification.box.visible = false
        table.insert(naughty.notifications.suspended, notification)
    end

    -- return the notification
    return notification
end

return naughty

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
