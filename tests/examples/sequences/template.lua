
-- This template provides a timeline like display for tag changes.
-- Basically a taglist with some pretty dots on the side. Note that the way it
-- copy tags is fragile and probably not really supportable.

local file_path, image_path = ...
require("_common_template")(...)
local capi = {client = client, screen = screen}
local Pango = require("lgi").Pango
local cairo = require("lgi").cairo
local PangoCairo = require("lgi").PangoCairo
require("awful.screen")
require("awful.tag")
local floating_l = require("awful.layout.suit.floating")
local taglist = require("awful.widget.taglist")
local gtable = require("gears.table")
local shape = require("gears.shape")
local color = require("gears.color")
local wibox = require("wibox")
local beautiful = require("beautiful")

local bar_size, radius = 18, 2
local screen_scale_factor = 5

local history = {}

local module = {}

-- Draw a mouse cursor at [x,y]
local function draw_mouse(cr, x, y)
    cr:set_source_rgb(1, 0, 0)
    cr:move_to(x, y)
    cr:rel_line_to( 0, 10)
    cr:rel_line_to( 3, -2)
    cr:rel_line_to( 3,  4)
    cr:rel_line_to( 2,  0)
    cr:rel_line_to(-3, -4)
    cr:rel_line_to( 4,  0)
    cr:close_path()
    cr:fill()
end

-- Instead of returning the maximum size, return the preferred one.
local function fixed_fit2(self, context, orig_width, orig_height)
    local width, height = orig_width, orig_height
    local used_in_dir, used_max = 0, 0

    for _, v in pairs(self._private.widgets) do
        local w, h = wibox.widget.base.fit_widget(self, context, v, width, height)

        -- Catch bugs before they crash Chrome (Firefox handles this better)
        assert(w < 5000)
        assert(h < 5000)

        local in_dir, max
        if self._private.dir == "y" then
            max, in_dir = w, h
            height = height - in_dir
        else
            in_dir, max = w, h
            width = width - in_dir
        end
        if max > used_max then
            used_max = max
        end
        used_in_dir = used_in_dir + in_dir
    end

    local spacing = self._private.spacing * (#self._private.widgets-1)

    -- Catch bugs before they crash Chrome (Firefox handles this better)
    assert(used_max    < 9000)
    assert(used_in_dir < 9000)

    if self._private.dir == "y" then
        return used_max, used_in_dir + spacing
    end
    return used_in_dir + spacing, used_max
end

-- Imported from the Collision module.
local function draw_lines()
    local ret = wibox.widget.base.make_widget()

    function ret:fit()
        local pager_w = math.max(root.size()/screen_scale_factor, 60)

        --FIXME support multiple screens.
        local w = (#screen[1].tags * pager_w)
            + ((#screen[1].tags - 1)*5)

        return w, self.widget_pos and #self.widget_pos*6 or 30
    end

    function ret:draw(_, cr, _, h)
        if (not self.widget_pos) or (not self.pager_pos) then return end

        cr:set_line_width(1)
        cr:set_source(color.change_opacity(beautiful.fg_normal, 0.3))

        local count = #self.widget_pos

        for k, t in ipairs(self.widget_pos) do
            local point1 = {x = t.widget_x, y = 0, width = t.widget_width, height = 1}
            local point2 = {x = t.pager_x, y = 0, width = t.pager_width, height = h}
            assert(point1.x and point1.width)
            assert(point2.x and point2.width)

            local dx = (point1.x == 0 and radius or 0) + (point1.width and point1.width/2 or 0)

            cr:move_to(bar_size+dx+point1.x, point1.y+2*radius)
            cr:line_to(bar_size+dx+point1.x, point2.y+(count-k)*((h-2*radius)/count)+2*radius)
            cr:line_to(point2.x+point2.width/2, point2.y+(count-k)*((h-2*radius)/count)+2*radius)
            cr:line_to(point2.x+point2.width/2, point2.y+point2.height)
            cr:stroke()

            cr:arc(bar_size+dx+point1.x, point1.y+2*radius, radius, 0, 2*math.pi)
            cr:fill()

            cr:arc(point2.x+point2.width/2, point2.y+point2.height-radius, radius, 0, 2*math.pi)
            cr:fill()
        end
    end

    return ret
end

local function gen_vertical_line(args)
    args = args or {}

    local w = wibox.widget.base.make_widget()

    function w:draw(_, cr, w2, h)
        cr:set_source(color.change_opacity(beautiful.fg_normal, 0.5))

        if args.begin then
            cr:rectangle(w2/2-0.5, h/2, 1, h/2)
        elseif args.finish then
            cr:rectangle(w2/2-0.5, 0, 1, h/2)
        else
            cr:rectangle(w2/2-0.5, 0, 1, h)
        end

        cr:fill()

        if args.dot then
            cr:arc(w2/2, args.center and h/2 or w2/2 ,bar_size/4, 0, 2*math.pi)
            cr:set_source_rgb(1,1,1)
            cr:fill_preserve()
            cr:set_source(color.change_opacity(beautiful.fg_normal, 0.5))
            cr:stroke()
        end
    end

    function w:fit()
        return bar_size, bar_size
    end

    return w
end

local function gen_taglist_layout_proxy(tags, w2, name)
    local l = wibox.layout.fixed.horizontal()
    l.fit = fixed_fit2

    local layout = l.layout

    l.layout = function(self,context, width, height)
        local ret = layout(self,context, width, height)

        for k, v in ipairs(ret) do
            tags[k][name.."_x"    ] = v._matrix.x0
            tags[k][name.."_width"] = v._width
        end

        if w2 then
            w2[name.."_pos"] = tags

            if not w2[name.."_configured"] then
                rawset(w2, name.."_configured", true)
                w2:emit_signal("widget::redraw_needed")
                w2:emit_signal("widget::layout_changed")
            end
        end

        return ret
    end

    return l
end

local function gen_fake_taglist_wibar(tags, w2)
    local layout = gen_taglist_layout_proxy(tags, w2, "widget")
    local w = wibox.widget {
        {
            {
                forced_height = bar_size,
                forced_width  = bar_size,
                image         = beautiful.awesome_icon,
                widget        = wibox.widget.imagebox,
            },
            taglist {
                forced_height = 14,
                forced_width  = 300,
                layout        = layout,
                screen        = screen[1],
                filter        = taglist.filter.all,
                source        = function() return tags end,
            },
            fit = fixed_fit2,
            layout = wibox.layout.fixed.horizontal,
        },
        bg     = beautiful.bg_normal,
        widget = wibox.container.background
    }

    -- Make sure it nevers goes unbounded by accident.
    local w3, h3 = w:fit({dpi=96}, 9999, 9999)
    assert(w3 < 5000 and h3 < 5000)

    return w
end

local function gen_cls(c,results)
    local ret = setmetatable({},{__index = function(_, i)
            local ret2 = c[i]
            if type(ret2) == "function" then
                if i == "geometry" then
                    return function(_, val)
                        if val then
                            c:geometry(gtable.crush(c:geometry(), val))
                            -- Make a copy as the original will be changed
                            results[c] = gtable.clone(c:geometry())
                        end
                        return c:geometry()
                    end
                else
                    return function(_,...) return ret2(c,...) end
                end
            end
            return ret2
        end})
    return ret
end

local function get_all_tag_clients(t)
    local s = t.screen

    local all_clients = gtable.clone(t:clients(), false)

    local clients = {}

    for _, c in ipairs(all_clients) do
        if not c.minimized then
            table.insert(clients, c)
        end
    end

    for _, c in ipairs(s.clients) do
        if c.sticky and not c.minimized then
            if not gtable.hasitem(clients, c) then
                table.insert(clients, c)
            end
        end
    end

    return clients
end

local function fake_arrange(tag)
    local cls,results,flt = {},setmetatable({},{__mode="k"}),{}
    local l = tag.layout
    local focus, focus_wrap = capi.client.focus, nil

    for _ ,c in ipairs (get_all_tag_clients(tag)) do
        -- Handle floating client separately
        if not c.minimized then
            local floating = c.floating
            if (not floating) and (l ~=  floating_l) then
                cls[#cls+1] = gen_cls(c,results)
                if c == focus then
                    focus_wrap = cls[#cls]
                end
            else
                flt[#flt+1] = gtable.clone(c:geometry())
                flt[#flt].c = c
            end
        end
    end

    -- The magnifier layout require a focussed client
    -- there wont be any as that layout is not selected
    -- take one at random or (TODO) use stack data
    if not focus_wrap then
        focus_wrap = cls[1]
    end

    local param = {
        tag              = tag,
        screen           = 1,
        clients          = cls,
        focus            = focus_wrap,
        geometries       = setmetatable({}, {__mode = "k"}),
        workarea         = tag.screen.workarea,
        useless_gap      = tag.gaps or beautiful.useless_gap or 4,
        gap_single_client= tag.gap_single_client or beautiful.gap_single_client or nil,
        apply_size_hints = false,
    }

    l.arrange(param)

    local ret = {}

    for _, geo_src in ipairs {param.geometries, flt } do
        for c, geo in pairs(geo_src) do
            geo.c = geo.c or c

            if type(c) == "table" and c.geometry then
                c:geometry(geo)
            end

            geo.color = geo.c.color
            table.insert(ret, geo)
        end
    end

    return ret
end

local function gen_fake_clients(tag, args)
    local pager = wibox.widget.base.make_widget()

    function pager:fit()
        return 60, 48
    end

    if not tag then return end

    local sgeo = tag.screen.geometry

    local show_name = args.display_client_name or args.display_label

    function pager:draw(_, cr, w, h)
        if not tag.client_geo then return end

        for _, geom in ipairs(tag.client_geo) do
            local x      = ( (geom.x - sgeo.x) * w)/sgeo.width
            local y      = ( (geom.y - sgeo.y) * h)/sgeo.height
            local width  = (geom.width*w)/sgeo.width
            local height = (geom.height*h)/sgeo.height

            cr:set_source(color(geom.color or beautiful.bg_normal))
            cr:rectangle(x,y,width,height)
            cr:fill_preserve()
            cr:set_source(color(geom.c.border_color or beautiful.border_color))
            cr:stroke()

            if show_name and type(geom.c) == "table" and geom.c.name then
                cr:set_source(beautiful.fg_normal)
                cr:move_to(x + 2, y + height - 2)
                cr:show_text(geom.c.name)
            end
        end

        -- Draw the screen outline.
        cr:set_source(color.change_opacity(beautiful.fg_normal, 0.1725))
        cr:set_line_width(1.5)
        cr:set_dash({10,4},1)
        cr:rectangle(0, 0, w, h)
        cr:stroke()
    end

    return pager
end

local function gen_fake_pager_widget(tags, w2, args)
    local layout = gen_taglist_layout_proxy(tags, w2, "pager")
    layout.spacing = 10

    for _, t in ipairs(tags) do
        layout:add(wibox.widget {
            gen_fake_clients(t, args),
            widget        = wibox.container.background
        })
    end

    return layout
end

local function wrap_timeline(w, dot)
    return wibox.widget {
            gen_vertical_line { dot = dot or false},
            {
                w,
                top     = 0,
                bottom  = dot and 5 or 0,
                left    = 0,
                widget  = wibox.container.margin
            },
            fit    = fixed_fit2,
            layout = wibox.layout.fixed.horizontal
        }
end

local function gen_vertical_space(spacing)
    return wibox.widget {
        draw   = function() end,
        fit    = function() return 1, spacing end,
        widget = wibox.widget.base.make_widget()
    }
end

local function gen_label(text)
    return wibox.widget {
            gen_vertical_line {
                dot    = true,
                begin  = text == "Begin",
                finish = text == "End",
                center = true,
            },
        {
            {
                {
                    {
                        markup       = "<span size='smaller'><b>"..text.."</b> </span>",
                        forced_width = 50,
                        widget       = wibox.widget.textbox
                    },
                    top    = 2,
                    bottom = 2,
                    right  = 5,
                    left   = 10,
                    widget = wibox.container.margin
                },
                shape        = shape.rectangular_tag,
                border_width = 2,
                border_color = beautiful.border_color,
                bg           = beautiful.bg_normal,
                widget       = wibox.container.background
            },
            top    = 10,
            bottom = 10,
            widget = wibox.container.margin
        },
        fit    = fixed_fit2,
        layout = wibox.layout.fixed.horizontal
    }
end

local function draw_info(s, cr, factor)
    cr:set_source(color.change_opacity(beautiful.fg_normal, 0.4))

    local pctx    = PangoCairo.font_map_get_default():create_context()
    local playout = Pango.Layout.new(pctx)
    local pdesc   = Pango.FontDescription()
    pdesc:set_absolute_size(11 * Pango.SCALE)
    playout:set_font_description(pdesc)

    local rows = {
        "primary", "index", "geometry", "dpi", "dpi range", "outputs:"
    }

    local dpi_range = s.minimum_dpi and s.preferred_dpi and s.maximum_dpi
        and (s.minimum_dpi.."-"..s.preferred_dpi.."-"..s.maximum_dpi)
        or s.dpi.."-"..s.dpi

    local values = {
        s.primary and "true" or "false",
        s.index,
        s.x..":"..s.y.." "..s.width.."x"..s.height,
        s.dpi,
        dpi_range,
        "",
    }

    for n, o in pairs(s.outputs) do
        table.insert(rows, "  "..n)
        table.insert(values,
            math.ceil(o.mm_width).."mm x "..math.ceil(o.mm_height).."mm"
        )
    end

    local col1_width, col2_width, height = 0, 0, 0

    -- Get the extents of the longest label.
    for k, label in ipairs(rows) do
        local attr, parsed = Pango.parse_markup(label..":", -1, 0)
        playout.attributes, playout.text = attr, parsed
        local _, logical = playout:get_pixel_extents()
        col1_width = math.max(col1_width, logical.width+10)

        attr, parsed = Pango.parse_markup(values[k], -1, 0)
        playout.attributes, playout.text = attr, parsed
        _, logical = playout:get_pixel_extents()
        col2_width = math.max(col2_width, logical.width+10)

        height = math.max(height, logical.height)
    end

    local dx = (s.width*factor - col1_width - col2_width - 5)/2
    local dy = (s.height*factor - #values*height)/2 - height

    -- Draw everything.
    for k, label in ipairs(rows) do
        local attr, parsed = Pango.parse_markup(label..":", -1, 0)
        playout.attributes, playout.text = attr, parsed
        playout:get_pixel_extents()
        cr:move_to(dx, dy)
        cr:show_layout(playout)

        attr, parsed = Pango.parse_markup(values[k], -1, 0)
        playout.attributes, playout.text = attr, parsed
        local _, logical = playout:get_pixel_extents()
        cr:move_to( dx+col1_width+5, dy)
        cr:show_layout(playout)

        dy = dy + 5 + logical.height
    end
end

local function gen_ruler(h_or_v, factor, margins)
    local ret = wibox.widget.base.make_widget()

    function ret:fit()
        local w, h
        local rw, rh = root.size()
        rw, rh = rw*factor, rh*factor

        if h_or_v == "vertical" then
            w = 1
            h = rh + margins.top/2 + margins.bottom/2
        else
            w = rw + margins.left/2 + margins.right/2
            h = 1
        end

        return w, h
    end

    function ret:draw(_, cr, w, h)
        cr:set_source(color("#77000033"))
        cr:set_line_width(2)
        cr:set_dash({1,1},1)
        cr:move_to(0, 0)
        cr:line_to(w == 1 and 0 or w, h == 1 and 0 or h)
        cr:stroke()
    end

    return ret
end

-- When multiple tags are present, only show the selected tag(s) for each screen.
local function gen_screens(l, screens, args)
    local margins = {left=50, right=50, top=30, bottom=30}

    local ret = wibox.layout.manual()

    local screen_copies = {}

    -- Keep a copy because it can change.
    local rw, rh = root.size()

    -- Find the current origin.
    local x0, y0 = math.huge, math.huge

    for s in screen do
        x0, y0 = math.min(x0, s.geometry.x), math.min(y0, s.geometry.y)
        local scr_cpy = gtable.clone(s.geometry, false)
        scr_cpy.outputs = gtable.clone(s.outputs, false)
        scr_cpy.primary = screen.primary == s

        for _, prop in ipairs {
          "dpi", "index", "maximum_dpi", "minimum_dpi", "preferred_dpi" } do
            scr_cpy[prop] = s[prop]
        end

        table.insert(screen_copies, scr_cpy)
    end

    function ret:fit()
        local w = margins.left+(x0+rw)/screen_scale_factor + 5 + margins.right
        local h = margins.top +(y0+rh)/screen_scale_factor + 5 + margins.bottom
        return w, h
    end

    -- Add the rulers.
    for _, s in ipairs(screen_copies) do
        ret:add_at(
            gen_ruler("vertical"  , 1/screen_scale_factor, margins),
            {x=margins.left+s.x/screen_scale_factor, y =margins.top/2}
        )
        ret:add_at(
            gen_ruler("vertical"  , 1/screen_scale_factor, margins),
            {x=margins.left+s.x/screen_scale_factor+s.width/screen_scale_factor, y =margins.top/2}
        )
        ret:add_at(
            gen_ruler("horizontal", 1/screen_scale_factor, margins),
            {y=margins.top+s.y/screen_scale_factor, x =margins.left/2}
        )
        ret:add_at(
            gen_ruler("horizontal", 1/screen_scale_factor, margins),
            {y=margins.top+s.y/screen_scale_factor+s.height/screen_scale_factor, x =margins.left/2}
        )
    end

    -- Print an outline for the screens
    for k, s in ipairs(screen_copies) do
        s.widget = wibox.widget.base.make_widget()

        local wb = gen_fake_taglist_wibar(screens[k].tags)
        wb.forced_width = s.width/screen_scale_factor

        local sel = nil

        -- AwesomeWM always use the layout from the first selected tag.
        for _, t in ipairs(screens[k].tags) do
            if t.selected then
                sel = t
                break
            end
        end

        -- Fallback because some older examples don't select tags.
        if not sel then
            sel = screens[k].tags[1]
        end

        local clients_w = gen_fake_clients(sel, args)

        local content = wibox.widget {
            wb,
            clients_w,
            nil,
            layout       = wibox.layout.align.vertical,
            forced_width = s.width/screen_scale_factor,
        }

        function s.widget:fit()
            return s.width/screen_scale_factor, s.height/screen_scale_factor
        end

        function s.widget:draw(_, cr, w, h)
            cr:set_source(color.change_opacity(beautiful.fg_normal, 0.1725))
            cr:set_line_width(1.5)
            cr:set_dash({10,4},1)
            cr:rectangle(1,1,w-2,h-2)
            cr:stroke()

            if args.display_label ~= false then
                draw_info(s, cr, 1/screen_scale_factor)
            end
        end

        function s.widget:after_draw_children(_, cr)
            if args.display_mouse and screens[k].cursor_screen == s.index then
                local rel_x = screens[k].cursor_position.x - s.x
                local rel_y = screens[k].cursor_position.y - s.y
                draw_mouse(cr, rel_x/screen_scale_factor+5, rel_y/screen_scale_factor+5)
            end
        end

        function s.widget:layout(_, width, height)
            return { wibox.widget.base.place_widget_at(
                content, 0, 0, width, height
            ) }
        end

        ret:add_at(s.widget, {x=margins.left+s.x/screen_scale_factor, y=margins.top+s.y/screen_scale_factor})
    end

    l:add(wrap_timeline(wibox.widget {
        markup  = "<i>Current tags:</i>",
        opacity = 0.5,
        widget  = wibox.widget.textbox
    }, true))
    l:add(wrap_timeline(ret,false))
end

-- When a single screen is present, show all tags.
local function gen_noscreen(l, tags, args)
    local w2 = draw_lines()
    l:add(wrap_timeline(wibox.widget {
        markup  = "<i>Current screens:</i>",
        opacity = 0.5,
        widget  = wibox.widget.textbox
    }, true))

    local wrapped_wibar = wibox.widget {
        gen_fake_taglist_wibar(tags, w2),
        fill_space = false,
        fit = fixed_fit2,
        layout = wibox.layout.fixed.horizontal
    }

    l:add(wrap_timeline(wrapped_wibar, false))

    if #capi.client.get() > 0 or args.show_empty then
        local w3, h3 = w2:fit({dpi=96}, 9999, 9999)
        assert(w3 < 5000 and h3 < 5000)

        l:add(wrap_timeline(w2, false))
        l:add(wrap_timeline(gen_fake_pager_widget(tags, w2, args), false))
    end
end

local function gen_timeline(args)
    local l = wibox.layout.fixed.vertical()
    l.fit = fixed_fit2

    l:add(gen_label("Begin"))

    -- Run the some steps of the Initialization.
    client.emit_signal("scanning")

    -- Finish the initialization.
    awesome.startup = false --luacheck: ignore

    for _, event in ipairs(history) do
        require("gears.timer").run_delayed_calls_now()
        local ret = event.callback()
        require("gears.timer").run_delayed_calls_now()
        if event.event == "event" then
            l:add(wrap_timeline(gen_vertical_space(5)))
            l:add(wrap_timeline(wibox.widget {
                markup = "<u><b>"..event.description.."</b></u>",
                widget = wibox.widget.textbox
            }, true))
        elseif event.event == "tags" and #ret == 1 and not args.display_screen then
            gen_noscreen(l, ret[1].tags, args)
        elseif event.event == "tags" and (#ret > 1 or args.display_screen) then
            gen_screens(l, ret, args)
        elseif event.event == "widget" then
            l:add(wrap_timeline(ret))
        end
    end

    -- Spacing.
    l:add(wrap_timeline(gen_vertical_space(10)))

    l:add(gen_label("End"))

    return l
end

local function wrap_with_arrows(widget)
    local w = widget:fit({dpi=96}, 9999,9999)

    local arrows = wibox.widget.base.make_widget()

    function arrows:fit()
        return w, 10
    end

    function arrows:draw(_, cr, w2, h)

        cr:set_line_width(1)
        cr:set_source_rgba(1, 0, 0, 0.3)

        w2 = math.min(640, w2)

        local x = (w2 % 24) / 2

        while x + 15 < w2 do
            cr:move_to(x+2  , 0  )
            cr:line_to(x+10 , h-1)
            cr:line_to(x+20 , 0  )
            cr:stroke()
            x = x + 24
        end
    end

    assert(w < 5000)

    return wibox.widget {
        widget,
        {
            draw   = function() end,
            fit    = function() return 1, 10 end,
            widget = wibox.widget.base.make_widget()
        },
        {
            markup      = "<span color='#ff000055'>Code for this sequence</span>",
            align       = "center",
            foced_width = w,
            widget      = wibox.widget.textbox,
        },
        arrows,
        forced_width = w,
        fit          = fixed_fit2,
        layout       = wibox.layout.fixed.vertical
    }
end

-- Use a recording surface to store the widget content.
function module.display_widget(wdg, width, height, context)
    local function do_it()
        context = context or {dpi=96}

        local w, h = wdg:fit(context, width or 9999, height or 9999)

        w, h = math.min(w,  width or 9999), math.min(h, height or 9999)

        local memento = cairo.RecordingSurface(
            cairo.Content.CAIRO_FORMAT_ARGB32,
            cairo.Rectangle { x = 0, y = 0, width = w, height = h }
        )

        local cr = cairo.Context(memento)

        wibox.widget.draw_to_cairo_context(wdg, cr, w, h, context)

        return wibox.widget {
            fit = function()
                return w, h+5
            end,
            draw = function(_, _, cr2)
                cr2:translate(0, 5)
                cr2:set_source_surface(memento)
                cr2:paint()
            end
        }
    end

    table.insert(history, {event="widget", callback = do_it})
end

function module.display_tags()
    local function do_it()
        local ret = {}
        for s in screen do
            local st = {}
            for _, t in ipairs(s.tags) do
                -- Copy just enough for the taglist to work.
                table.insert(st, {
                    name                = t.name,
                    selected            = t.selected,
                    icon                = t.icon,
                    screen              = t.screen,
                    _private            = t._private,
                    clients             = t.clients,
                    layout              = t.layout,
                    master_width_factor = t.master_width_factor,
                    client_geo          = fake_arrange(t),
                })
                assert(#st[#st].client_geo == #get_all_tag_clients(t))
            end
            table.insert(ret, {
                tags            = st,
                cursor_screen   = mouse.screen.index,
                cursor_position = gtable.clone(mouse.coords())
            })
        end
        return ret
    end

    table.insert(history, {event="tags", callback = do_it})
end

function module.add_event(description, callback)
    assert(description and callback)
    table.insert(history, {
        event       = "event",
        description = description,
        callback    = callback
    })
end

function module.execute(args)
    local widget = gen_timeline(args or {})
    require("gears.timer").run_delayed_calls_now()
    require("gears.timer").run_delayed_calls_now()
    require("gears.timer").run_delayed_calls_now()

    if (not args) or args.show_code_pointer ~= false then
        widget = wrap_with_arrows(widget)
    end

    local w, h = widget:fit({dpi=96}, 9999,9999)
    wibox.widget.draw_to_svg_file(widget, image_path..".svg", w, h)
end

loadfile(file_path)(module)
