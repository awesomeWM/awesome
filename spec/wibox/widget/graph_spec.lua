---------------------------------------------------------------------------
-- @author Alex Belykh
-- @copyright 2021 Alex Belykh
---------------------------------------------------------------------------

-- Require test_utils for the assert.widget_fit() helper.
require("wibox.test_utils")
-- Set emit_signal so as to not die on deprecation warnings.
_G.awesome.emit_signal = function() end
-- Silence debug warnings.
require("gears.debug").print_warning = function() end

local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local color = require("gears.color")
local graph = require("wibox.widget.graph")

local deprecated_properties = {
    height = true,
    width = true,
    stack_colors = true,
}

local property_defaults = {
    baseline_value = 0,
    clamp_bars = true,
    nan_indication = true,
    step_width = 1,
    step_spacing = 0,
}

local redrawless_properties = {
    capacity = true,
    height = true,
    width = true,
    stack_colors = true,
}

local data = {45.5, -44.5, -7.5, 1.5, 47.5, -38.5, 0.5, 38.0, 47.0, 23.5}
local data2 = {-26.5, 25.0, -19.0, 38.0, 12.0 -19.0, -35.0, 16.5}

local function push_data(widget, d, group_idx)
    -- Add in reverse, so that d could be compared with
    -- the backing value array directly.
    for i = #d,1,-1 do
        widget:add_value(d[i], group_idx)
    end
end

describe("wibox.widget.graph", function()
    local widget
    local redraw_needed, layout_changed

    before_each(function()
        widget = graph()

        widget:connect_signal("widget::redraw_needed", function()
            redraw_needed = redraw_needed + 1
        end)
        widget:connect_signal("widget::layout_changed", function()
            layout_changed = layout_changed + 1
        end)
        redraw_needed, layout_changed = 0, 0
    end)

    -- Check the trivial properties of all getters and setters.
    --
    -- There shouldn't be a field with a "get_/set_*" name, that isn't one.
    -- Iteration is over module fields, because fields of the instance can be
    -- polluted by inheritance and whatnot with something, that doesn't
    -- hold itself to the standard I'm setting here.
    for field, _ in pairs(graph) do

        if string.sub(field, 1, 4) == "get_" then
            -- A field with a getter-like name. Let's ensure that it is one.
            local prop_name = string.sub(field, 5)

            describe("field " .. field, function()
                it("has a corresponding set_" .. prop_name .. " counterpart", function()
                    assert.is_function(widget["set_" .. prop_name])
                end)

                it("is a property getter", function()
                    assert.is_function(widget[field])
                    stub(widget, field, 3456)
                    -- Access the property through metatable magic
                    local result_value = widget[prop_name]

                    assert.is.equal(3456, result_value)
                    assert.stub(widget[field]).was_called_with(widget)
                end)
            end)

        elseif string.sub(field, 1, 4) == "set_" then
            -- A field with a setter-like name. Let's ensure that it is one.
            local prop_name = string.sub(field, 5)

            describe("field " .. field, function()
                local property_signal_emitted, property_signal_emitted_with

                before_each(function()
                    widget:connect_signal("property::" .. prop_name, function(...)
                        property_signal_emitted = property_signal_emitted + 1
                        property_signal_emitted_with = {...}
                    end)
                    property_signal_emitted, property_signal_emitted_with = 0, nil
                end)

                it("has a corresponding get_" .. prop_name .. " counterpart", function()
                    assert.is_function(widget["get_" .. prop_name])
                end)

                it("is a property setter", function()
                    assert.is_function(widget[field])
                    assert.is.equal(0, redraw_needed)
                    assert.is.equal(0, layout_changed)
                    assert.is.equal(0, property_signal_emitted)

                    local s = spy.on(widget, field)

                    -- An access through metatable magic
                    widget[prop_name] = 3456

                    -- should have caused the call of the method
                    assert.spy(s).was_called_with(match.is_ref(widget), 3456)

                    -- and maybe a property::<prop_name> signal
                    assert.is.equal(deprecated_properties[prop_name] and 0 or 1, property_signal_emitted)
                    -- and maybe a redraw
                    assert.is.equal(redrawless_properties[prop_name] and 0 or 1, redraw_needed)
                    -- but never a layout change.
                    assert.is.equal(0, layout_changed)

                    if not deprecated_properties[prop_name] then
                        -- The property signal contains the new value.
                        assert.is.equal(widget, property_signal_emitted_with[1])
                        assert.is.equal(3456, property_signal_emitted_with[2])
                        -- What is set, can be gotten back through the prop_name field.
                        assert.is.equal(3456, widget[prop_name])
                        -- The setter returns the widget itself for call chaining.
                        assert.spy(s).returned_with(match.is_ref(widget))
                    end

                    s:clear()
                    assert.spy(s).was_not.called()

                    -- A repeated setting of the same value
                    widget[prop_name] = 3456
                    -- should have caused the call of the method again
                    assert.spy(s).was_called_with(match.is_ref(widget), 3456)
                    -- but none of the signals.
                    assert.is.equal(deprecated_properties[prop_name] and 0 or 1, property_signal_emitted)
                    assert.is.equal(redrawless_properties[prop_name] and 0 or 1, redraw_needed)
                    assert.is.equal(0, layout_changed)
                end)

                it("has a specific default value", function()
                    assert.is_equal(property_defaults[prop_name], widget[prop_name])
                end)

                it("is not magical on nil-s", function()
                    -- When set to nil
                    widget[prop_name] = nil
                    -- it should stay nil and not fall back to some "default".
                    assert.is.equal(nil, widget[prop_name])
                end)
            end) -- end describe(field)

        end -- end if
    end -- end field loop

    -- Now let's check some nontrivial behavior

    describe("values", function()
        it("are empty in a fresh instance", function()
            assert.is.same({}, widget._private.values)
        end)

        describe("method add_value()", function()
            it("adds values", function()
                push_data(widget, data)
                -- Adds into the first datagroup by default.
                assert.is.same({data}, widget._private.values)
            end)

            it("defaults to NaN when no/falsy value is supplied", function()
                local amount = 15
                for _ = 1, amount do
                    widget:add_value(false)
                    widget:add_value()
                    widget:add_value(false, 3)
                    widget:add_value(nil, 3)
                end

                assert.array(widget._private.values).has.no.holes()
                assert.is.equal(3, #widget._private.values)
                assert.is.equal(2*amount, #widget._private.values[1])
                assert.is.equal(0, #widget._private.values[2])
                assert.is.equal(2*amount, #widget._private.values[3])

                for i = 1, 2*amount do
                    local tmp = widget._private.values[1][i]
                    assert.is_not.equal(tmp, tmp)
                    tmp = widget._private.values[3][i]
                    assert.is_not.equal(tmp, tmp)
                end
            end)

            it("adds values into specific data group", function()
                push_data(widget, data, 15)
                assert.is.same(data, widget._private.values[15])

                -- Smaller datagroups are present too, but empty.
                assert.array(widget._private.values).has.no.holes()
                assert.is.equal(15, #widget._private.values)
                for i, data_group in ipairs(widget._private.values) do
                    assert.array(data_group).has.no.holes()
                    assert.is.equal(i ~= 15 and 0 or #data, #data_group)
                end

                -- Adding again to a different group
                push_data(widget, data2, 30)
                -- works
                assert.is.same(data2, widget._private.values[30])
                -- and doesn't affect the other group.
                assert.is.same(data, widget._private.values[15])

                -- Smaller-index datagroups are present but empty.
                assert.array(widget._private.values).has.no.holes()
                assert.is.equal(30, #widget._private.values)
                for i, data_group in ipairs(widget._private.values) do
                    assert.array(data_group).has.no.holes()
                    if i ~= 15 and i ~= 30 then
                        assert.is.same({}, data_group)
                    end
                end
            end)

            it("doesn't care about group order", function()
                for i = math.max(#data,#data2),1,-1 do
                    if i % 2 == 0 and data[i] then
                        widget:add_value(data[i], 1)
                    end
                    if data2[i] then
                     widget:add_value(data2[i], 2)
                    end
                    if i % 2 == 1 and data[i] then
                        widget:add_value(data[i], 1)
                    end
                end

                assert.is.same({data, data2}, widget._private.values)
            end)

            it("doesn't work with non-natural datagroups", function()
                for i = #data,1,-1 do
                    assert.has.errors(function()
                        widget:add_value(data[i], 14.5)
                    end)
                end
                for i = #data,1,-1 do
                    assert.has.errors(function()
                        widget:add_value(data[i], 0)
                    end)
                end
                for i = #data,1,-1 do
                    assert.has.errors(function()
                        widget:add_value(data[i], -4)
                    end)
                end
                for i = #data,1,-1 do
                    assert.has.errors(function()
                        widget:add_value(data[i], "index")
                    end)
                end
                for i = #data,1,-1 do
                    assert.has.errors(function()
                        widget:add_value(data[i], {1, 2, 3})
                    end)
                end
                for i = #data,1,-1 do
                    assert.has.errors(function()
                        widget:add_value(data[i], function() end)
                    end)
                end

                -- No values were added after all this,
                -- but some empty datagroups were. This is a bit suboptimal,
                -- but is not worth fixing. Adding an assert here
                -- so that one would be reminded to change it to
                -- a #values == 0 check, if one fixes it.
                assert.is.equal(14, #widget._private.values)
                assert.array(widget._private.values).has.no.holes()
                for _, data_group in ipairs(widget._private.values) do
                    assert.is.same({}, data_group)
                end
            end)

            it("honors the capacity property", function()
                local function check_cap(cap, expected_len)
                    expected_len = expected_len or math.max(0, math.ceil(cap))

                    widget.capacity = cap

                    push_data(widget, data, 1)
                    push_data(widget, data2, 2)

                    -- Only the last inserted elements are kept.
                    assert.is.same(
                        {
                           {unpack(data, 1, expected_len)},
                           {unpack(data2, 1, expected_len)}
                        },
                        widget._private.values
                    )
                end

                for i = 0, math.min(#data, #data2) do
                    check_cap(i)
                end
                -- It handles negatives and NaNs like zeros
                check_cap(-100, 0)
                check_cap(-100.5, 0)
                check_cap(0/0)
                -- And fractional numbers are rounded up
                check_cap(2.5, 3)

                -- But setting the capacity property by itself doesn't do anything,
                -- if add_value() wasn't called.
                widget.capacity = 1
                assert.is.equal(3, #widget._private.values[1])
                assert.is.equal(3, #widget._private.values[2])
            end)

            it("stores up to 8192 values when no usage stats are available", function()
                assert.is_nil(widget.capacity)
                assert.is_nil(widget._private.last_drawn_values_num)
                for i = 1, 9000 do
                    widget:add_value(i)
                    widget:add_value(i, 3)
                end
                assert.is.equal(8192, #widget._private.values[1])
                assert.is.equal(8192, #widget._private.values[3])
            end)

            it("relies on usage stats, when capacity is unset", function()
                assert.is_nil(widget.capacity)
                widget._private.last_drawn_values_num = 100

                for i = 1, 400 do
                    widget:add_value(i)
                    widget:add_value(i, 3)
                end

                -- The smallest multiple of 64 that is >= last_drawn_values_num + 64
                assert.is.equal(192, #widget._private.values[1])
                assert.is.equal(192, #widget._private.values[3])

                -- so 192 elements will be kept with this too.
                widget._private.last_drawn_values_num = 128

                widget:add_value(0)
                widget:add_value(0, 3)

                assert.is.equal(192, #widget._private.values[1])
                assert.is.equal(192, #widget._private.values[3])

                -- But this is one is already one too many.
                widget._private.last_drawn_values_num = 129

                for i = 1, 400 do
                    widget:add_value(i)
                    widget:add_value(i, 3)
                end

                assert.is.equal(256, #widget._private.values[1])
                assert.is.equal(256, #widget._private.values[3])

                -- Setting it back and calling add_value() once is enough
                -- to purge overflowing elements,
                widget._private.last_drawn_values_num = 128
                widget:add_value(0)
                assert.is.equal(192, #widget._private.values[1])
                -- but only in the group, for which add_value() happened to be
                -- called during the time last_drawn_values_num was small enough,
                -- even though it's probably not a very fair behavior and could
                -- lead to weird visual artefacts.
                assert.is.equal(256, #widget._private.values[3])

                -- Calling add_value for the other group puts it in line too.
                widget:add_value(0, 3)
                assert.is.equal(192, #widget._private.values[3])
            end)


        end) -- end describe(add_value)

        describe("method clear()", function()
            it("clears values", function()
                local function check_clear(i)
                    assert.is.same({}, widget._private.values)
                    push_data(widget, data)
                    assert.is.same({data}, widget._private.values)
                    widget:clear()
                    assert.is.same({}, widget._private.values)
                    push_data(widget, data2, 3*i)
                    assert.is.same(data2, widget._private.values[3*i])
                    widget:clear()
                    assert.is.same({}, widget._private.values)
                end

                for i = 1, 3 do
                    check_clear(i)
                end
            end)
        end) -- end describe(clear)

    end) -- end describe(values)

    describe("method fit()", function()
        it("requests all available space", function()
            assert.widget_fit(widget, {120, 80}, {120, 80})
            assert.widget_fit(widget, {1300, 700}, {1300, 700})
        end)
    end)

    describe("drawing-related", function()
        local dimensions
        local context
        local cr
        before_each(function()
            dimensions = {120, 80}
            context = { fake_context = "fake context" }
            cr = setmetatable({"fake cairo"}, {__index = function() return function() end end})
            -- assert.widget_fit(widget, dimensions, dimensions)
        end)

        describe("property step_shape()", function()
            it("is not called when there are no values", function()
                widget.step_shape = spy.new()

                widget:draw(context, cr, unpack(dimensions))
                widget:draw(context, cr, unpack(dimensions))

                -- No values to draw, so no shapes drawn
                assert.spy(widget.step_shape).was_not_called()
            end)

            it("is called to draw values", function()
                widget.step_shape = spy.new()

                -- It is called once for each value.
                push_data(widget, data)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_shape).was_called(#data)

                -- All data is drawn despite the gap between data groups
                push_data(widget, data2, 3)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_shape).was_called(2*#data + #data2)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_shape).was_called(3*#data + 2*#data2)
            end)

            it("receives proper arguments from draw()", function()
                widget.step_shape = spy.new()
                push_data(widget, data)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_shape).was_called_with(
                    cr, -- cairo
                    match.is_number(), -- width
                    match.is_number() -- height
                )
            end)

            it("doesn't receive NaNs from draw()", function()
                widget.step_shape = spy.new(function(_, width, height)
                    -- These are never NaNs.
                    assert.is_equal(width, width)
                    assert.is_equal(height, height)
                end)

                -- When there are no NaNs in data, there are no NaNs.
                push_data(widget, data)
                push_data(widget, data2, 3)
                widget:draw(context, cr, unpack(dimensions))
                -- Write out arguments in full once so that one doesn't
                -- forget to update this test if arguments get changed.
                assert.spy(widget.step_shape).was_called_with(
                    cr, match.is_number(), match.is_number()
                )
                widget.step_shape:clear()

                -- No matter where the NaNs are,
                widget:add_value(0/0, 1)
                widget:add_value(0/0, 2)
                widget:add_value(0/0, 3)
                widget:add_value(0/0, 4)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_shape).was_called()
                widget.step_shape:clear()

                -- even if there's nothing but NaNs, we get them not,
                widget:clear()
                for _ = 1, 100 do
                    widget:add_value(0/0, 1)
                    widget:add_value(0/0, 2)
                    widget:add_value(0/0, 3)
                end
                widget:draw(context, cr, unpack(dimensions))
                -- in fact we don't even get called.
                assert.spy(widget.step_shape).was_not_called()
                widget.step_shape:clear()
            end)

            it("'s errors are not silenced by draw()", function()
                widget.step_shape = spy.new(function() assert(false) end)
                push_data(widget, data)

                assert.has.errors(function()
                   widget:draw(context, cr, unpack(dimensions))
                end)
                assert.spy(widget.step_shape).was_called(1)
            end)

            it("is called only between group_start() and group_end()", function()
                local inside_group = nil
                widget.group_start = spy.new(function(_, group_idx)
                    assert.is_nil(inside_group)
                    inside_group = group_idx
                end)
                widget.group_finish = spy.new(function(_, group_idx)
                    assert.is_equal(group_idx, inside_group)
                    inside_group = nil
                end)
                widget.step_shape = spy.new(function()
                    assert.is_number(inside_group)
                end)

                push_data(widget, data)
                push_data(widget, data2, 3)

                widget:draw(context, cr, unpack(dimensions))

                -- Callbacks should have been called for each data group,
                -- including the empty second group,
                assert.spy(widget.group_start).was_called(3)
                assert.spy(widget.step_shape).was_called(#data + #data2)
                assert.spy(widget.group_finish).was_called(3)
                -- and group_finish() was the last of them.
                assert.is_nil(inside_group)
            end)

            it("is disabled by falsy group_colors", function()
                -- TODO: if should_draw_data_group() gets decided on,
                -- this should be rewritten in terms of it and
                -- should_draw_data_group() should be tested by colors in turn.
                widget.step_shape = spy.new()
                widget.group_colors = { "#feedf00d", "#deadbeef", "#0badcafe" }
                push_data(widget, data, 1)
                push_data(widget, data, 2)
                push_data(widget, data2, 3)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_shape).was_called(2*#data + #data2)
                widget.step_shape:clear()
                widget.group_colors = { nil, false, "#feedf00d" }
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_shape).was_called(#data2)
            end)
        end) -- end describe(step_shape)

        local function validate_draw_callback_options(options, group_idx, step_width)
            if step_width then
                -- Compare with the value passed as a parameter.
                assert.is.equal(step_width, options._step_width)
            end
            assert.is.number(options._step_width)
            assert.is.number(options._step_spacing)
            assert.is.number(options._width)
            assert.is.number(options._height)
            assert.is.number(options._group_idx)
            if group_idx then
                -- Compare with the value passed as a parameter.
                assert.is.equal(group_idx, options._group_idx)
            end
            -- Ensure it's integer.
            assert.is.equal(math.floor(options._group_idx), options._group_idx)
            assert.is.equal(widget, options._graph)
            -- It also contains the draw context
            assert.is.equal("fake context", options.fake_context)
        end

        describe("property step_hook()", function()
            it("is not called when there are no values", function()
                widget.step_hook = spy.new()

                widget:draw(context, cr, unpack(dimensions))
                widget:draw(context, cr, unpack(dimensions))

                -- No values to draw, so no shapes drawn
                assert.spy(widget.step_hook).was_not_called()
            end)

            it("is called to draw values", function()
                widget.step_hook = spy.new()

                -- It is called once for each value.
                push_data(widget, data)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_hook).was_called(#data)

                -- All data is drawn despite the gap between data groups
                push_data(widget, data2, 3)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_hook).was_called(2*#data + #data2)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_hook).was_called(3*#data + 2*#data2)
            end)

            it("receives proper arguments from draw()", function()
                widget.step_hook = spy.new(function(_, _, _, _, step_width, options)
                    validate_draw_callback_options(options, nil, step_width)
                end)
                push_data(widget, data)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_hook).was_called_with(
                    cr, -- cairo
                    match.is_number(), -- x
                    match.is_number(), -- value_y
                    match.is_number(), -- baseline_y
                    match.is_number(), -- step_width
                    match.is_table() -- options
                )
            end)

            it("receives NaNs from draw()", function()
                local nans_encountered = 0
                widget.step_hook = spy.new(function(_, x, y, b, step_width)
                    if not(y == y) then
                        nans_encountered = nans_encountered + 1
                    end
                    -- These are never NaNs though.
                    assert.is_equal(x, x)
                    assert.is_equal(b, b)
                    assert.is_equal(step_width, step_width)
                end)

                -- When there are no NaNs in data, there are no NaNs.
                push_data(widget, data)
                push_data(widget, data2, 3)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(0, nans_encountered)
                -- Write out arguments in full once so that one doesn't
                -- forget to update this test if arguments get changed.
                assert.spy(widget.step_hook).was_called_with(
                    cr, -- cairo
                    match.is_number(), -- x
                    match.is_number(), -- value_y
                    match.is_number(), -- baseline_y
                    match.is_number(), -- step_width
                    match.is_table() -- options
                )

                -- When there is as much as a single NaN, we get it,
                widget:add_value(0/0, 3)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(1, nans_encountered)
                -- no matter in which group,
                widget:add_value(0/0, 1)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(3, nans_encountered)
                -- even in a previously empty one.
                widget:add_value(0/0, 2)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(6, nans_encountered)
            end)

            it("overrides step_shape()", function()
                -- Even if step_shape is set after step_hook
                widget.step_hook = spy.new()
                widget.step_shape = spy.new()

                push_data(widget, data)
                push_data(widget, data2, 3)

                widget:draw(context, cr, unpack(dimensions))

                -- step_hook should have been called
                assert.spy(widget.step_hook).was_called(#data + #data2)
                -- and step_shape should not.
                assert.spy(widget.step_shape).was_not.called()
            end)

            it("'s errors are not silenced by draw()", function()
                widget.step_hook = spy.new(function() assert(false) end)
                push_data(widget, data)

                assert.has.errors(function()
                   widget:draw(context, cr, unpack(dimensions))
                end)
                assert.spy(widget.step_hook).was_called(1)
            end)

            it("is called only between group_start() and group_end()", function()
                local inside_group = nil
                widget.group_start = spy.new(function(_, group_idx)
                    assert.is_nil(inside_group)
                    inside_group = group_idx
                end)
                widget.group_finish = spy.new(function(_, group_idx)
                    assert.is_equal(group_idx, inside_group)
                    inside_group = nil
                end)
                widget.step_hook = spy.new(function(_, _, _, _, _, options)
                    assert.is_equal(options._group_idx, inside_group)
                end)

                push_data(widget, data)
                push_data(widget, data2, 3)

                widget:draw(context, cr, unpack(dimensions))

                -- Callbacks should have been called for each data group,
                -- including the empty second group,
                assert.spy(widget.group_start).was_called(3)
                assert.spy(widget.step_hook).was_called(#data + #data2)
                assert.spy(widget.group_finish).was_called(3)
                -- and group_finish() was the last of them.
                assert.is_nil(inside_group)
            end)

            it("is disabled by falsy group_colors", function()
                -- TODO: if should_draw_data_group() gets decided on,
                -- this should be rewritten in terms of it and
                -- should_draw_data_group() should be tested by colors in turn.
                widget.step_hook = spy.new()
                widget.group_colors = { "#feedf00d", "#deadbeef", "#0badcafe" }
                push_data(widget, data, 1)
                push_data(widget, data, 2)
                push_data(widget, data2, 3)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_hook).was_called(2*#data + #data2)
                widget.step_hook:clear()
                widget.group_colors = { nil, false, "#feedf00d" }
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.step_hook).was_called(#data2)
            end)

            it("always starts drawing values starting with x=0", function()
                local first_value = nil
                widget.step_hook = function(_, x)
                    if first_value == nil then
                        first_value = x
                    end
                end
                widget.group_finish = spy.new(function(_, group_idx)
                    assert.is.equal(group_idx ~= 2 and 0 or nil, first_value)
                    first_value = nil
                end)
                push_data(widget, data, 1)
                push_data(widget, data2, 3)

                -- With default settings.
                widget:draw(context, cr, unpack(dimensions))
                -- And with step_width.
                widget.step_width = 3
                widget:draw(context, cr, unpack(dimensions))
                -- And with step_spacing.
                widget.step_spacing = 3
                widget:draw(context, cr, unpack(dimensions))

                assert.spy(widget.group_finish).was_called(9)
            end)

        end) -- end describe(step_hook)

        describe("property group_start()", function()
            it("is called by draw() for each data group", function()
                widget.group_start = spy.new()
                -- Not used, when there are no data groups.
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.group_start).was_not.called()
                -- Used once for each data group (even empty ones, as it
                -- happens, though that is not really necessary).
                -- But I'm testing for it here too, so that possible
                -- future behavior change can be detected, and it can
                -- be decided, if it's ok to break.
                push_data(widget, data, 1)
                push_data(widget, data2, 3)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.group_start).was_called(3)
                assert.spy(widget.group_start).was_called_with(cr, 1, match.is_table())
                assert.spy(widget.group_start).was_called_with(cr, 2, match.is_table())
                assert.spy(widget.group_start).was_called_with(cr, 3, match.is_table())
            end)

            it("receives proper arguments from draw()", function()
                widget.group_start = spy.new(function(_, group_idx, options)
                    validate_draw_callback_options(options, group_idx)
                end)
                push_data(widget, data)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.group_start).was_called_with(cr, 1, match.is_table())
            end)

            it("'s errors are not silenced by draw()", function()
                widget.group_start = spy.new(function() assert(false) end)
                push_data(widget, data)

                assert.has.errors(function()
                   widget:draw(context, cr, unpack(dimensions))
                end)
                assert.spy(widget.group_start).was_called(1)
            end)

            it("shifts coordinates when set to a number", function()
                local x_coords = {}
                widget.step_hook = function(_, x)
                    table.insert(x_coords, x)
                end
                -- First a run without setting it.
                assert.is_nil(widget.group_start)
                push_data(widget, data)
                widget:draw(context, cr, unpack(dimensions))
                -- Now a run with setting it to a number.
                local x_coords_unshifted = {unpack(x_coords)}
                x_coords = {}
                widget.group_start = 3
                widget:draw(context, cr, unpack(dimensions))

                assert.is.equal(#x_coords_unshifted, #x_coords)
                for i, x in ipairs(x_coords_unshifted) do
                    assert.is.equal(x+3, x_coords[i])
                end
            end)
        end) -- end describe(group_start)

        describe("property group_finish()", function()
            it("is called by draw() for each data group", function()
                widget.group_finish = spy.new()
                -- Not used, when there are no data groups.
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.group_finish).was_not.called()
                -- Used once for each data group (even empty ones, as it
                -- happens, though that is not really necessary).
                -- But I'm testing for it here too, so that possible
                -- future behavior change can be detected, and it can
                -- be decided, if it's ok to break.
                push_data(widget, data, 1)
                push_data(widget, data2, 3)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.group_finish).was_called(3)
                assert.spy(widget.group_finish).was_called_with(cr, 1, match.is_table())
                assert.spy(widget.group_finish).was_called_with(cr, 2, match.is_table())
                assert.spy(widget.group_finish).was_called_with(cr, 3, match.is_table())
            end)

            it("receives proper arguments from draw()", function()
                widget.group_finish = spy.new(function(_, group_idx, options)
                    validate_draw_callback_options(options, group_idx)
                end)
                push_data(widget, data)
                widget:draw(context, cr, unpack(dimensions))
                assert.spy(widget.group_finish).was_called_with(cr, 1, match.is_table())
            end)

            it("'s errors are not silenced by draw()", function()
                widget.group_finish = spy.new(function() assert(false) end)
                push_data(widget, data)

                assert.has.errors(function()
                   widget:draw(context, cr, unpack(dimensions))
                end)
                assert.spy(widget.group_finish).was_called(1)
            end)
        end) -- end describe(group_finish)

        describe("method compute_drawn_values_num()", function()
            it("'s default implementation computes things correctly", function()
                local function cdvn(param)
                    return widget:compute_drawn_values_num(param)
                end

                -- Negative and zero values don't make it return negatives.
                -- Default settings, border_width=step_spacing=0, step_width=1.
                assert.is.equal(0, cdvn(-5))
                assert.is.equal(0, cdvn(0))
                assert.is.equal(10, cdvn(10))
                assert.is.equal(11, cdvn(10.5))
                -- border_width doesn't affect anything, because
                -- the function assumes that it was subtracted already.
                widget.border_width = 2
                assert.is.equal(0, cdvn(-50.5))
                assert.is.equal(0, cdvn(0))
                assert.is.equal(9, cdvn(9))
                assert.is.equal(10, cdvn(10))
                assert.is.equal(11, cdvn(10.5))
                -- All of this is just how you'd expect it to be for
                -- math.ceil(width/(step_width + step_spacing), which it is.
                widget.step_width = 2
                assert.is.equal(0, cdvn(-100))
                assert.is.equal(0, cdvn(0))
                assert.is.equal(4, cdvn(8))
                assert.is.equal(5, cdvn(9))
                assert.is.equal(5, cdvn(10))
                assert.is.equal(6, cdvn(11))
                assert.is.equal(6, cdvn(12))
                widget.step_spacing = 2
                assert.is.equal(0, cdvn(-100))
                assert.is.equal(0, cdvn(0))
                assert.is.equal(2, cdvn(8))
                assert.is.equal(3, cdvn(9))
                assert.is.equal(3, cdvn(10))
                assert.is.equal(3, cdvn(11))
                assert.is.equal(3, cdvn(12))
                assert.is.equal(4, cdvn(13))
            end)

            it("is called by draw() once", function()
                local s = spy.on(widget, "compute_drawn_values_num")

                widget:draw(context, cr, unpack(dimensions))

                assert.spy(s).was_called(1)
                assert.spy(s).was_called_with(match.is_ref(widget), dimensions[1])
            end)

            it("'s result is stored by draw() in last_drawn_values_num", function()
                local our_value = 10
                local function cdvn()
                    return our_value
                end
                widget.compute_drawn_values_num = cdvn

                -- The usage stats variable is uninitialized before first call.
                assert.is_nil(widget._private.last_drawn_values_num)

                widget:draw(context, cr, unpack(dimensions))
                -- Now it should be initialized.
                assert.is.equal(our_value, widget._private.last_drawn_values_num)

                -- Repeated calls with increasing values should set it instantly.
                our_value = 20
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(our_value, widget._private.last_drawn_values_num)
                our_value = 35
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(our_value, widget._private.last_drawn_values_num)
                our_value = 50
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(our_value, widget._private.last_drawn_values_num)
                -- Same values keep it on the same level too.
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(our_value, widget._private.last_drawn_values_num)

                -- Calls with smaller values should only decrement it by one.
                our_value = 30
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(49, widget._private.last_drawn_values_num)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(48, widget._private.last_drawn_values_num)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(47, widget._private.last_drawn_values_num)

                -- No matter how small the values are.
                our_value = 0
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(46, widget._private.last_drawn_values_num)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(45, widget._private.last_drawn_values_num)

                -- Negatives and NaNs are outright ignored.
                our_value = -100
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(45, widget._private.last_drawn_values_num)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(45, widget._private.last_drawn_values_num)
                our_value = 0/0
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(45, widget._private.last_drawn_values_num)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(45, widget._private.last_drawn_values_num)

                -- Positive infinity will fry the widget forever though.
                our_value = math.huge
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(math.huge, widget._private.last_drawn_values_num)
                our_value = 15
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(math.huge, widget._private.last_drawn_values_num)
            end)

        end) -- end describe(computed_drawn_values_num)

        describe("colors", function()
            local magic_color
            local magic_color_used
            local current_cr_source
            before_each(function()
                cr.set_source = function(_, src)
                    if src == magic_color then
                        magic_color_used = magic_color_used + 1
                    end
                    current_cr_source = src
                end
                magic_color, magic_color_used = color("#deadc0de"), 0
                current_cr_source = nil
            end)

            it("aren't used, when unset", function()
                -- Let's check that our magic color isn't some default color
                -- in a fresh widget
                assert.is.equal(0, magic_color_used)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(0, magic_color_used)
                -- even after pushing data.
                push_data(widget, data, 4)
                widget:draw(context, cr, unpack(dimensions))
                assert.is.equal(0, magic_color_used)
            end)

            describe("property color", function()
                -- These tests overlap with pick_data_group_color() tests,
                -- but are added for completeness.

                it("is used by draw() when set", function()
                    assert.is.equal(0, magic_color_used)
                    widget.color = magic_color
                    -- Not used, if there are no values to draw.
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)
                    -- Used, if there are, once for each data group.
                    push_data(widget, data, 4)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(4, magic_color_used)
                end)

                it("is overridden by group_colors property", function()
                    assert.is.equal(0, magic_color_used)
                    widget.color = magic_color
                    widget.group_colors = { "#feedf00d" }
                    push_data(widget, data, 1)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)
                end)
            end) -- end describe(color)

            describe("property group_colors", function()
                -- These tests overlap with pick_data_group_color() tests,
                -- but are added for completeness.

                it("is used by draw() when set", function()
                    assert.is.equal(0, magic_color_used)
                    widget.group_colors = { "#feedfood", false, magic_color, magic_color }
                    -- Not used, if there are no values to draw.
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)
                    -- Not used, if there are no values in its group.
                    push_data(widget, data, 1)
                    push_data(widget, data2, 2)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)
                     -- Used, if there are, once for each data group.
                    push_data(widget, data, 3)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(1, magic_color_used)
                    push_data(widget, data2, 4)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(3, magic_color_used)
                end)
            end) -- end describe(group_colors)

            describe("property background_color", function()
                it("is used by draw() when set", function()
                    assert.is.equal(0, magic_color_used)
                    widget.background_color = magic_color
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(1, magic_color_used)
                end)
            end) -- end describe(background_color)

            describe("property border_color", function()
                it("is used by draw() when set", function()
                    assert.is.equal(0, magic_color_used)
                    widget.border_color = magic_color

                    -- Not used, when border_width <= 0, so not by default either.
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)
                    widget.border_width = 0
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)
                    widget.border_width = -1
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)

                    -- Anything positive enables the border.
                    widget.border_width = 0.000001
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(1, magic_color_used)
                end)
            end) -- end describe(border_color)

            describe("property nan_color", function()
                it("is good to go by default", function()
                    assert.is.truthy(widget.nan_indication)
                    -- TODO: maybe expose default_nan_color and check that it's used.
                end)

                it("is used by draw() when set", function()
                    assert.is.equal(0, magic_color_used)
                    widget.nan_color = magic_color
                    push_data(widget, data, 4)

                    -- Not used, when there are no NaNs.
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)

                    -- Is used once when a NaN is found, no matter how many
                    -- NaNs and groups there are.
                    widget:add_value(0/0, 4)
                    widget:add_value(0/0, 4)
                    widget:add_value(0/0, 5)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(1, magic_color_used)

                    -- But not when nan_indication is disabled.
                    widget.nan_indication = false
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(1, magic_color_used)
                end)
            end) -- end describe(nan_color)

            describe("method pick_data_group_color()", function()
                it("is called by draw() for each data group", function()
                    widget.pick_data_group_color = spy.new()
                    -- Not used, when there are no data groups.
                    widget:draw(context, cr, unpack(dimensions))
                    assert.spy(widget.pick_data_group_color).was_not.called()
                    -- Used once for each data group (even empty ones, as it
                    -- happens, though that is not really necessary).
                    -- But I'm testing for it here too, so that possible
                    -- future behavior change can be detected, and it can
                    -- be decided, if it's ok to break.
                    push_data(widget, data, 1)
                    push_data(widget, data2, 3)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.spy(widget.pick_data_group_color).was_called(3)
                    assert.spy(widget.pick_data_group_color).was_called_with(match.is_ref(widget), 1)
                    assert.spy(widget.pick_data_group_color).was_called_with(match.is_ref(widget), 2)
                    assert.spy(widget.pick_data_group_color).was_called_with(match.is_ref(widget), 3)
                end)

                it("is used by draw() to pick colors", function()
                    local s = spy.new(function(_, data_group)
                        return data_group > 2 and magic_color or "#feedf00d"
                    end)
                    widget.pick_data_group_color = s

                    -- Not used, if there are no values to draw.
                    assert.is.equal(0, magic_color_used)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)
                    assert.spy(s).was_not_called()
                    -- Colors it returns are used for respective data groups,
                    -- once for each data group.
                    push_data(widget, data, 1)
                    push_data(widget, data2, 2)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(0, magic_color_used)
                    assert.spy(s).was_called(2)
                    push_data(widget, data, 3)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(1, magic_color_used)
                    assert.spy(s).was_called(5)
                    push_data(widget, data2, 4)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(3, magic_color_used)
                    assert.spy(s).was_called(9)
                    s:clear()
                    -- Also used, when group_start is set to number,
                    assert.is_nil(widget.group_start)
                    widget.group_start = 3
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(5, magic_color_used)
                    assert.spy(s).was_called(4)
                    s:clear()
                    -- but not when group_start is set to function.
                    widget.group_start = function() end
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(5, magic_color_used)
                    assert.spy(s).was_not_called()
                end)

                it("'s returned color is set early, so as to be available in the corresponding step_shape()", function()
                    local current_data_group
                    widget.pick_data_group_color = function(_, data_group)
                        current_data_group = data_group
                        return (data_group % 2 == 1) and magic_color or "#feedf00d"
                    end
                    widget.step_shape = function()
                        if current_data_group % 2 == 1 then
                            assert.is.equal(magic_color, current_cr_source)
                        else
                            assert.is_not.equal(magic_color, current_cr_source)
                        end
                    end
                    assert.is.equal(0, magic_color_used)
                    assert.is_nil(current_cr_source)
                    push_data(widget, data, 1)
                    push_data(widget, data2, 2)
                    push_data(widget, data2, 3)
                    push_data(widget, data, 4)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(2, magic_color_used)
                end)

                it("'s returned color is set early, so as to be available in the corresponding step_hook()", function()
                    widget.pick_data_group_color = function(_, data_group)
                        return (data_group % 2 == 1) and magic_color or "#feedf00d"
                    end
                    widget.step_hook = function(_, _, _, _, _, options)
                        if options._group_idx % 2 == 1 then
                            assert.is.equal(magic_color, current_cr_source)
                        else
                            assert.is_not.equal(magic_color, current_cr_source)
                        end
                    end
                    assert.is.equal(0, magic_color_used)
                    assert.is_nil(current_cr_source)
                    push_data(widget, data, 1)
                    push_data(widget, data2, 2)
                    push_data(widget, data2, 3)
                    push_data(widget, data, 4)
                    widget:draw(context, cr, unpack(dimensions))
                    assert.is.equal(2, magic_color_used)
                end)

                it("returns some valid color even in a fresh widget", function()
                    local groups = {1, 2, 50, 0, -1}
                    local colors = {}
                    for i, group in ipairs(groups) do
                        colors[i] = widget:pick_data_group_color(group)
                        assert.is.truthy(colors[i])
                    end
                    -- Regardless of data presence.
                    push_data(widget, data, 2)
                    local colors2 = {}
                    for i, group in ipairs(groups) do
                        colors2[i] = widget:pick_data_group_color(group)
                        assert.is.truthy(colors2[i])
                    end
                    assert.is.same(colors, colors2)
                    -- And it's actually the same default color in all cases.
                    for i = 2, #colors do
                        assert.is.equal(colors[i-1], colors[i])
                    end
                    -- And gears.color() doesn't choke on it.
                    assert.is.truthy(color(colors[1]))
                    -- Because it's red, tbh.
                    assert.is.equal("#ff0000", colors[1])
                end)

                it("uses color and group_colors properties", function()
                    local groups = {1, 2, 5, 3, 4, 0, 6, -1}
                    for _, group in ipairs(groups) do
                        assert.is_not.equal(magic_color, widget:pick_data_group_color(group))
                    end
                    widget.color = magic_color
                    for _, group in ipairs(groups) do
                        assert.is.equal(magic_color, widget:pick_data_group_color(group))
                    end
                    -- But group_colors override color.
                    local group_colors = { "#feedf00d", "#deadbeef", false, nil, "#0badcafe" }
                    widget.group_colors = {unpack(group_colors, 1, 5)}
                    for _, group in ipairs(groups) do
                        if group_colors[group] then
                            assert.is.equal(group_colors[group], widget:pick_data_group_color(group))
                        else
                            -- But not when the color is falsy in the table group_colors.
                            assert.is.equal(magic_color, widget:pick_data_group_color(group))
                        end
                    end
                end)
             end) -- end describe(pick_data_group_color)

        end) -- end describe(colors)

    end) -- end describe(drawing-related)

end) -- end describe(graph)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
