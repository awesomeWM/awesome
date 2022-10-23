--DOC_GEN_IMAGE --DOC_NO_USAGE

--DOC_HIDE_START
local parent = ...
local wibox = require("wibox")
local gears = {
    shape   = require("gears.shape"),
    surface = require("gears.surface"),
    color   = require("gears.color"),
    table   = require("gears.table"),
}
local beautiful = require("beautiful")

client.focus = client.gen_fake{
    class = "client",
    name  = "A client!",
    icon  = beautiful.awesome_icon,
}

--DOC_HIDE_END
    -- Put this in a Lua file
    local module = {}
    --DOC_NEWLINE

    local default_template = wibox.widget.template {
        {
            {
                {
                    forced_height = 16,
                    forced_width  = 16,
                    shape         = gears.shape.circle,
                    widget        = wibox.widget.separator
                },
                margins = 3,
                widget  = wibox.container.margin
            },
            {
                set_color = function(self, color)
                    self.text = color
                end,
                text   = "N/A",
                widget = wibox.widget.textbox
            },
            spacing = 5,
            widget  = wibox.layout.fixed.horizontal,
        },
        set_color = function(self, color)
            self.border_color = gears.color.to_rgba_string(color):sub(1,7).."44"
            self.fg           = color
        end,
        border_width = 1,
        shape        = gears.shape.octogon,
        widget       = wibox.container.background,
        forced_width = 100, --DOC_HIDE
    }
    --DOC_NEWLINE

    --- Set the widget template.
    -- @property widget_template
    -- @tparam[opt=nil] wibox.template|nil
    function module:set_widget_template(t)
        self._private.widget_template = wibox.widget.template.make_from_value(t)
    end

    --DOC_NEWLINE
    function module:get_widget_template()
        return self._private.widget_template or default_template
    end
    --DOC_NEWLINE

    --- Add a color to the list.
    function module:add_color(color_name)
        local instance = self.widget_template:clone()
        instance:set_property("color", color_name, nil, true)

        self._private.base_layout:add(instance)
    end
    --DOC_NEWLINE

    --- my_module_name constructor.
    --
    -- @constructorfct my_module_name
    -- @tparam[opt={}] table args
    -- @tparam wibox.template args.widget_template A widget template.
    -- @tparam wibox.layout args.base_layout A layout to use instead of
    --  `wibox.layout.fixed.vertical`.
    local function new(_, args)
        args = args or {}

        --DOC_NEWLINE
        -- Create the base_layout instance.
        local l = wibox.widget.base.make_widget_from_value(
            args.base_layout or wibox.widget {
                spacing = 5,
                widget  = wibox.layout.fixed.vertical
            }
        )

        --DOC_NEWLINE
        -- Create the base widget as a proxy widget.
        local ret = wibox.widget.base.make_widget(l, "my_module_name", {
            enable_properties = true,
        })

        --DOC_NEWLINE
        -- Set the default (fallback) template
        ret._private.base_layout = l

        --DOC_NEWLINE
        -- Add the methods and apply set the initial properties.
        gears.table.crush(ret, module, true)
        gears.table.crush(ret, args)

        --DOC_NEWLINE
        return ret
    end

    --[[ --DOC_HIDE
    return setmetatable(module, {__call = new})
    ]]--DOC_HIDE


--[[ --DOC_HIDE
--DOC_NEWLINE
Now, lets use this module to list some colors:
--DOC_NEWLINE
    local my_module_name = require("my_module_name")
--DOC_NEWLINE
]] --DOC_HIDE
local my_module_name = new --DOC_HIDE
--DOC_NEWLINE

    local my_color_list = my_module_name {}
--DOC_NEWLINE

    my_color_list:add_color("#ff0000")
    my_color_list:add_color("#00ff00")
    my_color_list:add_color("#0000ff")
    my_color_list:add_color("#ff00ff")
    my_color_list:add_color("#ffff00")
    my_color_list:add_color("#00ffff")

--DOC_HIDE_START

parent:add(my_color_list)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
