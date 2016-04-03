---------------------------------------------------------------------------
--- A specialised stack layout with a tabbar on top.
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.layout.dynamic.tabbed
---------------------------------------------------------------------------

local capi = {client = client}
local stack     = require( "awful.layout.dynamic.base_stack" )
local margins   = require( "wibox.container.margin"          )
local wibox     = require( "wibox"                           )
local timer     = require( "gears.timer"                     )
local beautiful = require( "beautiful"                       )
local common    = require( "awful.widget.common"             )
local button    = require( "awful.button"                    )
local util      = require( "awful.util"                      )

local fct = {}

local default_buttons = util.table.join(
    button({ }, 1, function(c)
        capi.client.focus = c
        c:raise()
    end)
)

local function label(c)
    local shape = beautiful.tabbar_client_shape
    local shape_border_width = beautiful.tabbar_client_shape_border_width
    local shape_border_color = beautiful.tabbar_client_shape_border_color
    local bg = capi.client.focus == c and (
            beautiful.tabbar_client_bg_active or beautiful.bg_focus
        ) or (
            beautiful.tabbar_client_bg_normal or beautiful.bg_normal
        )

    local other_args = {
        shape              = shape,
        shape_border_width = shape_border_width,
        shape_border_color = shape_border_color,
    }

    return c.name, bg, nil, c.icon, other_args
end

local function regen_client_list(self)
    local all_widgets = self:get_all_children()
    local ret = {}

    for _, w in ipairs(all_widgets) do
        if w._client then
            table.insert(ret, w._client)
        end
    end

    common.list_update(
        self._wibox.widget,
        default_buttons,
        label,
        self._data,
        ret
    )

end

--- Create a rudimentary tabbar widget
local function create_tabbar(self, _--[[widget]])
    local flex = beautiful.tabbar_vertical and
        wibox.layout.flex.vertical() or wibox.layout.flex.horizontal()

    flex:set_spacing(beautiful.tabbar_spacing or 0)
    self._wibox:set_widget(flex)
    regen_client_list(self)
end

--- Move/resize the wibox to the right spot when the layout change
local function before_draw_child(self, context, index, child, cr, width, _) --luacheck: no unused_args
    if not self._wibox then
        self._wibox = wibox {
            bg = beautiful.tabbar_bg or beautiful.bg_normal,
            fg = beautiful.tabbar_fg
        }
        create_tabbar(self, self._s:get_children())
    end

    local matrix = cr:get_matrix()

    local height = math.ceil(beautiful.get_font_height() * 1.5)

    if beautiful.tabbar_vertical then
        height = math.max(height, height * (#self:get_all_children() -1))
    end

    self._wibox.x = matrix.x0
    self._wibox.y = matrix.y0
    self._wibox.height  = height
    self._wibox.width   = width
    self._wibox.visible = #self._s:get_children() > 0

    self:set_top(height)
end

--- Hide the wibox
local function suspend(self)
    if self._wibox then
        self._wibox.visible = false
    end
    self._s:suspend()
end

--- Display the wibox
local function wake_up(self)
    if self._wibox then
        self._wibox.visible = true
    end
    self._s:wake_up()
end

--- If there is only 1 tab left, self destruct
local function remove_widgets(self, widget)
    -- Self is the stack

    self._m._private.remove_widgets(self, widget)

    -- The delayed call is not really necessary, but it is safer to avoid
    -- messing with the hierarchy in nested calls
    if #self.children <= 1 then
        timer.delayed_call(function()
            -- Look for an handler, if none if found, then there is a bug
            -- somewhere
            local handler = widget._handler
            if not handler then
                local children = self._s:get_children(true)
                for _, w in ipairs(children) do
                    if w._handler then
                        handler = w._handler
                        break
                    end
                end
            end

            if not handler then return end

            local w = self.children[1]
            handler.widget:replace_widget(self._m, w, true)
            self._wibox.visible = false
            self._wibox = nil
        end)
    end
end

--- Construct a tabbed layout
local function ctr(_, _)
    local s = stack(false)

    local m = margins(s)
    m._s    = s
    m._data = {}
    s._m    = m
    m:set_top(math.ceil(beautiful.get_font_height() * 1.5))
    m:set_widget(s)

    m._private.remove_widgets  = s.remove_widgets
    rawset(m, "suspend"       , suspend       )
    rawset(m, "wake_up"       , wake_up       )
    rawset(s, "remove_widgets", remove_widgets)

    m.before_draw_child = before_draw_child

    -- "m" is a dumb proxy of "s", it only free the space for the tabbar
    if #fct == 0 then --TODO this check is broken
        for k, f in pairs(s) do
            if type(f) == "function" and not m[k] then
                fct[k] = f
            end
        end
    end

    for name, func in pairs(fct) do
        rawset(m, name, function(_, ...) return func(s,...) end)
    end

    -- When something change, recompute the client list.
    for _, s2 in ipairs {
        "widget::swapped_forward",
        "widget::inserted_forward",
        "widget::replaced_forward",
        "widget::removed_forward",
        "widget::added_forward",
        "widget::reseted_forward",
    } do
        m:connect_signal(s2, regen_client_list)
    end

    -- Prevent further changes
    m.set_widget = nil
    rawset(m, "set_children", function(_, ...) return s:set_children(...) end)
    rawset(m, "get_children", function(_) return s:get_children() end)

    return m
end

return ctr
