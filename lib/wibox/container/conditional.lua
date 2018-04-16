---------------------------------------------------------------------------
--
-- A container displaying a child widget when a condition is met.
--
-- This widget accept multiple widget templates (or instances) and will display
-- the first one matching the condition. If templates are provided (instead
-- of widget instances), the widget will be created only when the condition is
-- met.
--
-- If multiple candidates meet, the condition, the first one is used.
--
-- Please note that the `widget` and `children` properties are read-only due
-- to the nature of this widget.
--
-- Each template (or widget instance) encapsulated by this container has the
-- following properties:
--
-- * **when** (*function*, *mandatory*): The function called to determine if
--  the condition is met. It has to return true or false. It will be called
--  when `eval` is called or when one of the attached signal is emitted.
-- * **selected_callback** (*function*): When the template is selected.
-- * **deselected_callback** (*function*): When the template is no longer
--  selected.
-- * **updated_callback** (*function*): When `eval` returns the same condition
--  has before.
--
-- This widget can be tailored to specific use case. For example, to track the
-- focused client:
--
--@DOC_wibox_container_conditional_extend_EXAMPLE@
--
--@DOC_wibox_container_defaults_conditional_EXAMPLE@
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016-2018 Emmanuel Lepage Vallee
-- @classmod wibox.container.conditional
---------------------------------------------------------------------------

local gdebug = require("gears.debug")
local gtable = require("gears.table")
local gtimer = require("gears.timer")
local base   = require("wibox.widget.base")

local conditional = { mt = {} }

-- Layout this layout
function conditional:layout(_, width, height)
    if not self._private.widget then
        return
    end

    return { base.place_widget_at(self._private.widget, 0, 0, width, height) }
end

-- Fit this layout into the given area
function conditional:fit(context, width, height)
    if not self._private.widget then
        return 0, 0
    end
    return base.fit_widget(self, context, self._private.widget, width, height)
end

-- As all other containers support a `widget` argument, make sure no automated
-- codepaths call this by accident.
function conditional:set_widget(_)
    assert(false, "The conditional container does not accept widget, see `templates`")
end

--- The current child widget instance.
--
-- The widget that has been produced be the `current_template`.
--
-- This property is read only.
--
-- @property widget
-- @param widget

function conditional:get_widget()
    return self._private.widget
end

function conditional:set_children(children)
    gdebug.print_warning("The conditional container does not accept widget, see `templates`")
    self:set_widget(children[1])
end

--- Get the number of children element
-- @treturn table The children
function conditional:get_children()
    return {self._private.widget}
end

--- The list of conditional templates.
-- @property templates
-- @tparam table templates

function conditional:get_templates()
    return self._private.templates
end

--- Set the fallback conditional handler.
--
-- When no `when` function is set on the template, this one is called. This is
-- useful if an extended version of this widget adds additional properties to
-- templates.
--
-- @property default_when
-- @param function

function conditional:set_default_when(f)
    self._private.default_when = f
end

function conditional:set_templates(templates)
    for k, v in pairs(templates) do
        -- The order is important, so make sure it's stable
        assert(type(k) == "number", "The conditional template table only takes numeric keys")

        -- Without a way to determine the selected template, thigs will explode.
        -- It might come later if the fallback handler is set. Give it an event
        -- loop iteration before asserting.
        if (not self._private.delay_error) and type(v.when) ~= "function" then
            self._private.delay_error = true
            gtimer.delayed_call(function()
                self._private.delay_error = false
                for _, v2 in pairs(self._private.templates) do
                    if type(v2.when) ~= "function" and not self._private.default_when then
                        assert(false, "The conditional templates require a `when` function")
                        return
                    end
                end
            end)
        end
    end

    self._private.templates = templates
    self:eval()
end

--- The list of conditional templates.
-- @property selected_template
-- @tparam table selected_template The selected template or a widget instance

function conditional:get_selected_template()
    return self._private.selected
end

--- Return all attached signals.
--
-- The format is a list of tables with the object as the first value and the
-- signal name has the second.
--
-- @property attached_signals
-- @param table

function conditional:get_attached_signals()
    return self._private.attached_signal
end

--- The list of global signals used to automatically call `eval()`.
--
-- The format is a table of table with objects as keys and signal name as values.
--
-- @DOC_wibox_container_conditional_global_signals_EXAMPLE@
--
-- @property connected_global_signals
-- @param table

function conditional:get_connected_global_signals()
    return self._private.connected_global_signals
end

function conditional:set_connected_global_signals(signals)
    if self._private.connected_global_signals then
        for _, v in pairs(self._private.connected_global_signals) do
            v[1].disconnect_signal(v[2], self._private.eval_handler)
        end
    end

    self._private.connected_global_signals = signals

    for _, v in pairs(self._private.connected_global_signals) do
        assert(v[1].connect_signal, "The object passed to `global_signals` is invalid")
        assert(not v[1].is_widget,
            "Regular objects cannot be used in `global_signals`, use `attached_signals`"
        )
        v[1].connect_signal(v[2], self._private.eval_handler)
    end
end

--- Controls the arguments passed to the `when` callback.
--
-- This is useful when extending this container to fit specific purposes.
--
-- All values returned by this function will be passed to the `when` callback.
-- Note that it will be called only once for all templates.
--
-- The default value returns the `conditional` widget instance.
--
-- The callback has a single parameter (the widget instance).
--
-- @property argument_callback
-- @param function

function conditional:set_argument_callback(callback)
    self._private.argument_callback = callback
end

function conditional:get_argument_callback()
    return self._private.argument_callback
end

--- Reset this layout. The widget will be removed and the rotation reset.
function conditional:reset()
    self:set_widget(nil)
end

--- Evaluate all the `when` clauses and update the current widget(s).
--
-- @DOC_wibox_container_conditional_eval_EXAMPLE@
--
-- The signal is `property::selected_template` and has the following argument
--    * The `wibox.container.conditional` widget instance
--    * The current template
--    * The old template
--    * The current template widget instance
--    * The old template widget instance
--
function conditional:eval()

    -- Avoid called eval multiple timer per event loop cycle due to the
    -- attached signals. This creates and destroy widgets for nothing
    self._private.exec_once = false

    local old_selected, selected = self._private.selected ,nil

    for _, v in ipairs(self._private.templates or {}) do
        if v.when then
            if v.when(self._private.argument_callback(self)) then
                selected = v
                break
            end
        elseif self._private.default_when then
            local ret = self._private.default_when(v, self._private.argument_callback(self))

            -- The function must return something or is considered broken
            assert(type(ret) == "boolean", "The conditional layout `default_when`"..
                " must return a boolean"
            )

            if ret then
                selected = v
                break
            end
        else
            assert(false, "A conditional template has neither a `when` property"..
                " nor a default_when"
            )
        end
    end

    -- Nothing changed, move on
    if selected == self._private.selected then
        if selected and selected.updated_callback then
            selected.updated_callback(self._private.argument_callback())
        end

        return
    end

    if self._private.selected and self._private.selected.deselected_callback then
        self._private.selected:deselected_callback()
    end

    self._private.selected = selected

    if self._private.selected and self._private.selected.selected_callback then
        self._private.selected:selected_callback()
    end

    local old_widget = self._private.widget

    -- Create the widget unless it's already one.
    self._private.widget = selected and
        base.make_widget_from_value(selected) or nil

    self:emit_signal("property::selected_template",
        selected,
        old_selected,
        self._private.widget,
        old_widget
    )

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
end

--- Add a signal source to automatically call `eval()`.
--
-- @DOC_wibox_container_conditional_object_signals_EXAMPLE@
--
-- @tparam gears.object object The source object.
-- @tparam string signal The signal name.
function conditional:attach_signal(object, signal)
    table.insert(self._private.attached_signal, {object, signal})
    object:connect_signal(signal, self._private.eval_handler)
end

--- Remove a signal source to automatically call `eval()`.
function conditional:detach_signal(object, signal)
    object:disconnect_signal(signal, self._private.eval_handler)
    for k, v in ipairs(self._private.attached_signal) do
        if v[1] == object and v[2] == signal then
            table.remove(self._private.attached_signal, k)
        end
    end
end

-- Magical variable to avoid the template instantiation ahead of time.
conditional._accept_templates = true

--- Returns a new conditional container.
-- @param[opt] widget The widget to display.
-- @tparam[opt={}] table args Options for this widget.
-- @function wibox.container.conditional
local function new(args)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, conditional, true)

    ret._private.exec_once = false
    ret._private.attached_signal = {}

    ret._private.eval_handler = function()
        if not ret._private.exec_once then
            gtimer.delayed_call(function()
                ret:eval()
            end)
        end
        ret._private.exec_once = true
    end

    gtable.crush(ret, args or {})

    if not ret._private.argument_callback then
        ret._private.argument_callback = function()
            return ret
        end
    end

    return ret
end

function conditional.mt:__call(_, ...)
    return new(_, ...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(conditional, conditional.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
