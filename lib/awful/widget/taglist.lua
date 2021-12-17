---------------------------------------------------------------------------
--- Taglist widget module for awful.
--
-- Here is a more advanced example of how to extent the `taglist`. It provides:
--
-- * mouse "hover" color
-- * an extra index field
-- * a powerline look and feel
--
--@DOC_wibox_awidget_taglist_indexed_EXAMPLE@
--
-- As demonstrated in the example above, there are a few "shortcuts" to avoid
-- re-inventing the wheel. By setting the predefined roles as widget `id`s,
-- `awful.widget.common` will do most of the work to update the values
-- automatically. All of them are optional. The supported roles are:
--
-- * `icon_role`: A `wibox.widget.imagebox`
-- * `text_role`: A `wibox.widget.textbox`
-- * `background_role`: A `wibox.container.background`
-- * `text_margin_role`: A `wibox.container.margin`
-- * `icon_margin_role`: A `wibox.container.margin`
--
-- `awful.widget.common` also has 2 callbacks to give more control over the widget:
--
-- * `create_callback`: Called once after the widget instance is created
-- * `update_callback`: Called every time the data is refreshed
--
-- Both callback have the same parameters:
--
-- * `self`: The widget instance (*widget*).
-- * `t`: The tag (*tag*)
-- * `index`: The widget position in the list (*number*)
-- * `tags`: The list of tag, in order (*table*)
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @widgetmod awful.widget.taglist
---------------------------------------------------------------------------

-- Grab environment we need
local capi = { screen = screen,
               awesome = awesome,
               client = client }
local setmetatable = setmetatable
local pairs = pairs
local ipairs = ipairs
local table = table
local common = require("awful.widget.common")
local tag = require("awful.tag")
local beautiful = require("beautiful")
local fixed = require("wibox.layout.fixed")
local timer = require("gears.timer")
local gcolor = require("gears.color")
local gstring = require("gears.string")
local gdebug = require("gears.debug")
local base = require("wibox.widget.base")
local gtable = require("gears.table")

local function get_screen(s)
    return s and capi.screen[s]
end

local taglist = { mt = {} }
taglist.filter, taglist.source = {}, {}

--- The tag list main foreground (text) color.
-- @beautiful beautiful.taglist_fg_focus
-- @param[opt=beautiful.fg_focus] color
-- @see gears.color

--- The tag list main background color.
-- @beautiful beautiful.taglist_bg_focus
-- @param[opt=beautiful.bg_focus] color
-- @see gears.color

--- The tag list urgent elements foreground (text) color.
-- @beautiful beautiful.taglist_fg_urgent
-- @param[opt=beautiful.fg_urgent] color
-- @see gears.color

--- The tag list urgent elements background color.
-- @beautiful beautiful.taglist_bg_urgent
-- @param[opt=beautiful.bg_urgent] color
-- @see gears.color

--- The tag list occupied elements background color.
-- @beautiful beautiful.taglist_bg_occupied
-- @param color
-- @see gears.color

--- The tag list occupied elements foreground (text) color.
-- @beautiful beautiful.taglist_fg_occupied
-- @param color
-- @see gears.color

--- The tag list empty elements background color.
-- @beautiful beautiful.taglist_bg_empty
-- @param color
-- @see gears.color

--- The tag list empty elements foreground (text) color.
-- @beautiful beautiful.taglist_fg_empty
-- @param color
-- @see gears.color

--- The tag list volatile elements background color.
-- @beautiful beautiful.taglist_bg_volatile
-- @param color
-- @see gears.color

--- The tag list volatile elements foreground (text) color.
-- @beautiful beautiful.taglist_fg_volatile
-- @param color
-- @see gears.color

--- The selected elements background image.
-- @beautiful beautiful.taglist_squares_sel
-- @param surface
-- @see gears.surface

--- The unselected elements background image.
-- @beautiful beautiful.taglist_squares_unsel
-- @param surface
-- @see gears.surface

--- The selected empty elements background image.
-- @beautiful beautiful.taglist_squares_sel_empty
-- @param surface
-- @see gears.surface

--- The unselected empty elements background image.
-- @beautiful beautiful.taglist_squares_unsel_empty
-- @param surface
-- @see gears.surface

--- If the background images can be resized.
-- @beautiful beautiful.taglist_squares_resize
-- @param boolean

--- Do not display the tag icons, even if they are set.
-- @beautiful beautiful.taglist_disable_icon
-- @param boolean

--- The taglist font.
-- @beautiful beautiful.taglist_font
-- @param string

--- The space between the taglist elements.
-- @beautiful beautiful.taglist_spacing
-- @tparam[opt=0] number spacing The spacing between tags.

--- The main shape used for the elements.
-- This will be the fallback for state specific shapes.
-- To get a shape for the whole taglist, use `wibox.container.background`.
-- @beautiful beautiful.taglist_shape
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape
-- @see beautiful.taglist_shape_empty
-- @see beautiful.taglist_shape_focus
-- @see beautiful.taglist_shape_urgent
-- @see beautiful.taglist_shape_volatile

--- The shape elements border width.
-- @beautiful beautiful.taglist_shape_border_width
-- @param[opt=0] number
-- @see wibox.container.background

--- The elements shape border color.
-- @beautiful beautiful.taglist_shape_border_color
-- @param color
-- @see gears.color

--- The shape used for the empty elements.
-- @beautiful beautiful.taglist_shape_empty
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- The shape used for the empty elements border width.
-- @beautiful beautiful.taglist_shape_border_width_empty
-- @param[opt=0] number
-- @see wibox.container.background

--- The empty elements shape border color.
-- @beautiful beautiful.taglist_shape_border_color_empty
-- @param color
-- @see gears.color

--- The shape used for the selected elements.
-- @beautiful beautiful.taglist_shape_focus
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- The shape used for the selected elements border width.
-- @beautiful beautiful.taglist_shape_border_width_focus
-- @param[opt=0] number
-- @see wibox.container.background

--- The selected elements shape border color.
-- @beautiful beautiful.taglist_shape_border_color_focus
-- @param color
-- @see gears.color

--- The shape used for the urgent elements.
-- @beautiful beautiful.taglist_shape_urgent
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- The shape used for the urgent elements border width.
-- @beautiful beautiful.taglist_shape_border_width_urgent
-- @param[opt=0] number
-- @see wibox.container.background

--- The urgents elements shape border color.
-- @beautiful beautiful.taglist_shape_border_color_urgent
-- @param color
-- @see gears.color

--- The shape used for the volatile elements.
-- @beautiful beautiful.taglist_shape_volatile
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- The shape used for the volatile elements border width.
-- @beautiful beautiful.taglist_shape_border_width_volatile
-- @param[opt=0] number
-- @see wibox.container.background

--- The volatile elements shape border color.
-- @beautiful beautiful.taglist_shape_border_color_volatile
-- @param color
-- @see gears.color

local instances = nil

function taglist.taglist_label(t, args)
    if not args then args = {} end
    local theme = beautiful.get()
    local fg_focus = args.fg_focus or theme.taglist_fg_focus or theme.fg_focus
    local bg_focus = args.bg_focus or theme.taglist_bg_focus or theme.bg_focus
    local fg_urgent = args.fg_urgent or theme.taglist_fg_urgent or theme.fg_urgent
    local bg_urgent = args.bg_urgent or theme.taglist_bg_urgent or theme.bg_urgent
    local bg_occupied = args.bg_occupied or theme.taglist_bg_occupied
    local fg_occupied = args.fg_occupied or theme.taglist_fg_occupied
    local bg_empty = args.bg_empty or theme.taglist_bg_empty
    local fg_empty = args.fg_empty or theme.taglist_fg_empty
    local bg_volatile = args.bg_volatile or theme.taglist_bg_volatile
    local fg_volatile = args.fg_volatile or theme.taglist_fg_volatile
    local taglist_squares_sel = args.squares_sel or theme.taglist_squares_sel
    local taglist_squares_unsel = args.squares_unsel or theme.taglist_squares_unsel
    local taglist_squares_sel_empty = args.squares_sel_empty or theme.taglist_squares_sel_empty
    local taglist_squares_unsel_empty = args.squares_unsel_empty or theme.taglist_squares_unsel_empty
    local taglist_squares_resize = theme.taglist_squares_resize or args.squares_resize or "true"
    local taglist_disable_icon = args.taglist_disable_icon or theme.taglist_disable_icon or false
    local font = args.font or theme.taglist_font or theme.font
    local text = nil
    local sel = capi.client.focus
    local bg_color = nil
    local fg_color = nil
    local bg_image
    local icon
    local shape              = args.shape or theme.taglist_shape
    local shape_border_width = args.shape_border_width or theme.taglist_shape_border_width
    local shape_border_color = args.shape_border_color or theme.taglist_shape_border_color
    local icon_size = args.icon_size or theme.taglist_icon_size
    -- TODO: Re-implement bg_resize
    local bg_resize = false -- luacheck: ignore
    local is_selected = false
    local cls = t:clients()

    if sel and taglist_squares_sel then
        -- Check that the selected client is tagged with 't'.
        local seltags = sel:tags()
        for _, v in ipairs(seltags) do
            if v == t then
                bg_image = taglist_squares_sel
                bg_resize = taglist_squares_resize == "true"
                is_selected = true
                break
            end
        end
    end
    if #cls == 0 and t.selected and taglist_squares_sel_empty then
        bg_image = taglist_squares_sel_empty
        bg_resize = taglist_squares_resize == "true"
    elseif not is_selected then
        if #cls > 0 then
            if taglist_squares_unsel then
                bg_image = taglist_squares_unsel
                bg_resize = taglist_squares_resize == "true"
            end
            if bg_occupied then bg_color = bg_occupied end
            if fg_occupied then fg_color = fg_occupied end
        else
            if taglist_squares_unsel_empty then
                bg_image = taglist_squares_unsel_empty
                bg_resize = taglist_squares_resize == "true"
            end
            if bg_empty then bg_color = bg_empty end
            if fg_empty then fg_color = fg_empty end

            if args.shape_empty or theme.taglist_shape_empty then
                shape = args.shape_empty or theme.taglist_shape_empty
            end

            if args.shape_border_width_empty or theme.taglist_shape_border_width_empty then
                shape_border_width = args.shape_border_width_empty or theme.taglist_shape_border_width_empty
            end

            if args.shape_border_color_empty or theme.taglist_shape_border_color_empty then
                shape_border_color = args.shape_border_color_empty or theme.taglist_shape_border_color_empty
            end
        end
    end
    if t.selected then
        bg_color = bg_focus
        fg_color = fg_focus

        if args.shape_focus or theme.taglist_shape_focus then
            shape = args.shape_focus or theme.taglist_shape_focus
        end

        if args.shape_border_width_focus or theme.taglist_shape_border_width_focus then
            shape_border_width = args.shape_border_width_focus or theme.taglist_shape_border_width_focus
        end

        if args.shape_border_color_focus or theme.taglist_shape_border_color_focus then
            shape_border_color = args.shape_border_color_focus or theme.taglist_shape_border_color_focus
        end

    elseif tag.getproperty(t, "urgent") then
        if bg_urgent then bg_color = bg_urgent end
        if fg_urgent then fg_color = fg_urgent end

        if args.shape_urgent or theme.taglist_shape_urgent then
            shape = args.shape_urgent or theme.taglist_shape_urgent
        end

        if args.shape_border_width_urgent or theme.taglist_shape_border_width_urgent then
            shape_border_width = args.shape_border_width_urgent or theme.taglist_shape_border_width_urgent
        end

        if args.shape_border_color_urgent or theme.taglist_shape_border_color_urgent then
            shape_border_color = args.shape_border_color_urgent or theme.taglist_shape_border_color_urgent
        end

    elseif t.volatile then
        if bg_volatile then bg_color = bg_volatile end
        if fg_volatile then fg_color = fg_volatile end

        if args.shape_volatile or theme.taglist_shape_volatile then
            shape = args.shape_volatile or theme.taglist_shape_volatile
        end

        if args.shape_border_width_volatile or theme.taglist_shape_border_width_volatile then
            shape_border_width = args.shape_border_width_volatile or theme.taglist_shape_border_width_volatile
        end

        if args.shape_border_color_volatile or theme.taglist_shape_border_color_volatile then
            shape_border_color = args.shape_border_color_volatile or theme.taglist_shape_border_color_volatile
        end
    end

    if not tag.getproperty(t, "icon_only") then
        text = "<span font_desc='"..(font or "").."'>"
        if fg_color then
            text = text .. "<span color='" .. gcolor.ensure_pango_color(fg_color) ..
                "'>" .. (gstring.xml_escape(t.name) or "") .. "</span>"
        else
            text = text .. (gstring.xml_escape(t.name) or "")
        end
        text = text .. "</span>"
    end
    if not taglist_disable_icon then
        if t.icon then
            icon = t.icon
        end
    end

    local other_args = {
        shape              = shape,
        shape_border_width = shape_border_width,
        shape_border_color = shape_border_color,
        icon_size          = icon_size,
    }

    return text, bg_color, bg_image, not taglist_disable_icon and icon or nil, other_args
end

-- Remove some callback boilerplate from the user provided templates.
local function create_callback(w, t)
    common._set_common_property(w, "tag", t)
end

local function taglist_update(s, self, buttons, filter, data, style, update_function, args)
    local tags = {}

    local source = self._private.source or taglist.source.for_screen or nil
    local list   = source and source(s, args) or s.tags

    for _, t in ipairs(list) do
        if not tag.getproperty(t, "hide") and filter(t) then
            table.insert(tags, t)
        end
    end

    if self._private.last_count ~= #tags then
        self:emit_signal("property::count", #tags, self._private.last_count)
        self._private.last_count = #tags
    end

    local function label(c) return taglist.taglist_label(c, style) end

    update_function(self._private.base_layout, buttons, label, data, tags, {
        widget_template = self._private.widget_template,
        create_callback = create_callback,
    })
end

--- The taglist screen.
--
-- @property screen
-- @tparam screen screen

--- Set the taglist layout.
--
-- @property base_layout
-- @tparam[opt=wibox.layout.fixed.horizontal] wibox.layout base_layout
-- @see wibox.layout.fixed.horizontal

--- The current number of tags.
--
-- @property count
-- @readonly
-- @tparam number The number of tags.
-- @propemits true false

function taglist:set_base_layout(layout)
    self._private.base_layout = base.make_widget_from_value(
        layout or fixed.horizontal
    )

    local spacing = self._private.style.spacing or beautiful.taglist_spacing

    if self._private.base_layout.set_spacing and spacing then
        self._private.base_layout:set_spacing(spacing)
    end

    assert(self._private.base_layout.is_widget)

    self._do_taglist_update()

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::base_layout", layout)
end

function taglist:get_count()
    if not self._private.last_count then
        self._do_taglist_update_now()
    end

    return self._private.last_count
end

function taglist:layout(_, width, height)
    if self._private.base_layout then
        return { base.place_widget_at(self._private.base_layout, 0, 0, width, height) }
    end
end

function taglist:fit(context, width, height)
    if not self._private.base_layout then
        return 0, 0
    end

    return base.fit_widget(self, context, self._private.base_layout, width, height)
end

--- An alternative function to configure the content.
--
-- You should use `widget_template` if it fits your use case first. This
-- API is very low level.
--
-- @property update_function
-- @tparam function update_function.

--- A function to restrict the content of the taglist.
--
-- @property filter
-- @tparam function filter
-- @see source
-- @see awful.widget.taglist.filter.noempty
-- @see awful.widget.taglist.filter.selected
-- @see awful.widget.taglist.filter.all
-- @see awful.widget.taglist.source.for_screen

--- The function used to gather the group of tags.
--
-- @property source
-- @tparam function source
-- @see filter
-- @see awful.widget.taglist.source.for_screen

--- A templete used to genetate the individual tag widgets.
--
-- @property widget_template
-- @tparam table widget_template

for _, prop in ipairs { "filter", "update_function", "widget_template", "source", "screen" } do
    taglist["set_"..prop] = function(self, value)
        if value == self._private[prop] then return end

        self._private[prop] = value

        self._do_taglist_update()

        self:emit_signal("widget::layout_changed")
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("property::"..prop, value)
    end

    taglist["get_"..prop] = function(self)
        return self._private[prop]
    end
end

--- Create a new taglist widget. The last two arguments (update_function
-- and layout) serve to customize the layout of the taglist (eg. to
-- make it vertical). For that, you will need to copy the
-- awful.widget.common.list_update function, make your changes to it
-- and pass it as update_function here. Also change the layout if the
-- default is not what you want.
-- @tparam table args
-- @tparam screen args.screen The screen to draw taglist for.
-- @tparam function[opt=nil] args.filter Filter function to define what clients will be listed.
-- @tparam table args.buttons A table with buttons binding to set.
-- @tparam[opt] function args.update_function Function to create a tag widget on each
--   update. See `awful.widget.common`.
-- @tparam[opt] widget args.layout Optional layout widget for tag widgets. Default
--   is wibox.layout.fixed.horizontal().
-- @tparam[opt=awful.widget.taglist.source.for_screen] function args.source The
--  function used to generate the list of tag.
-- @tparam[opt] table args.widget_template A custom widget to be used for each tag
-- @tparam[opt={}] table args.style The style overrides default theme.
-- @tparam[opt=nil] string|pattern args.style.fg_focus
-- @tparam[opt=nil] string|pattern args.style.bg_focus
-- @tparam[opt=nil] string|pattern args.style.fg_urgent
-- @tparam[opt=nil] string|pattern args.style.bg_urgent
-- @tparam[opt=nil] string|pattern args.style.bg_occupied
-- @tparam[opt=nil] string|pattern args.style.fg_occupied
-- @tparam[opt=nil] string|pattern args.style.bg_empty
-- @tparam[opt=nil] string|pattern args.style.fg_empty
-- @tparam[opt=nil] string|pattern args.style.bg_volatile
-- @tparam[opt=nil] string|pattern args.style.fg_volatile
-- @tparam[opt=nil] string args.style.squares_sel
-- @tparam[opt=nil] string args.style.squares_unsel
-- @tparam[opt=nil] string args.style.squares_sel_empty
-- @tparam[opt=nil] string args.style.squares_unsel_empty
-- @tparam[opt=nil] string args.style.squares_resize
-- @tparam[opt=nil] string args.style.disable_icon
-- @tparam[opt=nil] string args.style.font
-- @tparam[opt=nil] number args.style.spacing The spacing between tags.
-- @tparam[opt] string args.style.squares_sel A user provided image for selected squares.
-- @tparam[opt] string args.style.squares_unsel A user provided image for unselected squares.
-- @tparam[opt] string args.style.squares_sel_empty A user provided image for selected squares for empty tags.
-- @tparam[opt] string args.style.squares_unsel_empty A user provided image for unselected squares for empty tags.
-- @tparam[opt] boolean args.style.squares_resize True or false to resize squares.
-- @tparam string|pattern args.style.bg_focus The background color for focused client.
-- @tparam string|pattern args.style.fg_focus The foreground color for focused client.
-- @tparam string|pattern args.style.bg_urgent The background color for urgent clients.
-- @tparam string|pattern args.style.fg_urgent The foreground color for urgent clients.
-- @tparam string args.style.font The font.
-- @param filter **DEPRECATED** use args.filter
-- @param buttons **DEPRECATED** use args.buttons
-- @param style **DEPRECATED** use args.style
-- @param update_function **DEPRECATED** use args.update_function
-- @param base_widget **DEPRECATED** use args.base_layout
-- @constructorfct awful.widget.taglist
function taglist.new(args, filter, buttons, style, update_function, base_widget)

    local screen = nil

    local argstype = type(args)

    -- Detect the old function signature
    if argstype == "number" or argstype == "screen" or
      (argstype == "table" and args.index and args == capi.screen[args.index]) then
        gdebug.deprecate("The `screen` parameter is deprecated, use `args.screen`.",
            {deprecated_in=5})

        screen = get_screen(args)
        args = {}
    end

    assert(type(args) == "table")

    for k, v in pairs { filter          = filter,
                        buttons         = buttons,
                        style           = style,
                        update_function = update_function,
                        layout          = base_widget
    } do
        gdebug.deprecate("The `awful.widget.taglist()` `"..k
            .."` parameter is deprecated, use `args."..k.."`.",
        {deprecated_in=5})
        args[k] = v
    end

    screen = screen or get_screen(args.screen)

    local uf = args.update_function or common.list_update

    local w = base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(w, taglist, true)

    gtable.crush(w._private, {
        style           = args.style or {},
        buttons         = args.buttons,
        filter          = args.filter,
        update_function = args.update_function,
        widget_template = args.widget_template,
        source          = args.source,
        screen          = screen
    })

    local data = setmetatable({}, { __mode = 'k' })

    local queued_update = {}

    function w._do_taglist_update_now()
        if w._private.screen.valid then
            taglist_update(
                w._private.screen, w, w._private.buttons, w._private.filter, data, args.style, uf, args
            )
        end
        queued_update[w._private.screen] = false
    end

    function w._do_taglist_update()
        -- Add a delayed callback for the first update.
        if not queued_update[w._private.screen] then
            timer.delayed_call(w._do_taglist_update_now)
            queued_update[w._private.screen] = true
        end
    end
    if instances == nil then
        instances = setmetatable({}, { __mode = "k" })
        local function u(s)
            local i = instances[get_screen(s)]
            if i then
                for _, tlist in pairs(i) do
                    tlist._do_taglist_update()
                end
            else
                -- No screen? Update all taglists
                for _, list in pairs(instances) do
                    for _, tlist in pairs(list) do
                        tlist._do_taglist_update()
                    end
                end
            end
        end
        local uc = function (c) return u(c.screen) end
        local ut = function (t) return u(t.screen) end
        capi.client.connect_signal("property::active", uc)
        tag.attached_connect_signal(nil, "property::selected", ut)
        tag.attached_connect_signal(nil, "property::icon", ut)
        tag.attached_connect_signal(nil, "property::hide", ut)
        tag.attached_connect_signal(nil, "property::name", ut)
        tag.attached_connect_signal(nil, "property::activated", ut)
        tag.attached_connect_signal(nil, "property::screen", ut)
        tag.attached_connect_signal(nil, "property::index", ut)
        tag.attached_connect_signal(nil, "property::urgent", ut)
        capi.client.connect_signal("property::screen", function(c, old_screen)
            u(c.screen)
            u(old_screen)
        end)
        capi.client.connect_signal("tagged", uc)
        capi.client.connect_signal("untagged", uc)
        capi.client.connect_signal("request::unmanage", uc)
        capi.screen.connect_signal("removed", function(s)
            instances[get_screen(s)] = nil
        end)
    end

    w:set_base_layout(args.base_layout or args.layout)

    w._do_taglist_update()
    local list = instances[screen]
    if not list then
        list = setmetatable({}, { __mode = "v" })
        instances[screen] = list
    end
    table.insert(list, w)
    return w
end

--- Filtering function to include all nonempty tags on the screen.
-- @param t The tag.
-- @return true if t is not empty, else false
-- @filterfunction awful.widget.taglist.filter.noempty
function taglist.filter.noempty(t)
    return #t:clients() > 0 or t.selected
end

--- Filtering function to include selected tags on the screen.
-- @param t The tag.
-- @return true if t is not empty, else false
-- @filterfunction awful.widget.taglist.filter.selected
function taglist.filter.selected(t)
    return t.selected
end

--- Filtering function to include all tags on the screen.
-- @return true
-- @filterfunction awful.widget.taglist.filter.all
function taglist.filter.all()
    return true
end

--- All tags for the defined screen ordered by indices.
--
-- This is the default source.
--
-- @sourcefunction awful.widget.taglist.source.for_screen
-- @see screen
function taglist.source.for_screen(s)
    return s.tags
end

function taglist.mt:__call(...)
    return taglist.new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(taglist, taglist.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
