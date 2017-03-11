---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
---------------------------------------------------------------------------

local hierarchy = require("wibox.hierarchy")

local Region = require("lgi").cairo.Region
local matrix = require("gears.matrix")
local utils = require("wibox.test_utils")

local function make_widget(children)
    local result = utils.widget_stub()
    result.layout = function()
        return children
    end
    return result
end

local function make_child(widget, width, height, mat)
    return { _widget = widget, _width = width, _height = height, _matrix = mat }
end

describe("wibox.hierarchy", function()
    describe("Accessor functions", function()
        local widget, instance
        before_each(function()
            local function nop() end
            local context = {}
            widget = make_widget(nil)
            instance = hierarchy.new(context, widget, 10, 20, nop, nop)
        end)

        it("get_widget", function()
            assert.is.equal(instance:get_widget(), widget)
        end)

        it("get_matrix_to_parent", function()
            assert.is.equal(matrix.identity, instance:get_matrix_to_parent())
        end)

        it("get_matrix_to_device", function()
            assert.is.equal(matrix.identity, instance:get_matrix_to_device())
        end)

        it("get_matrix_from_parent", function()
            assert.is.equal(matrix.identity, instance:get_matrix_from_parent())
        end)

        it("get_matrix_from_device", function()
            assert.is.equal(matrix.identity, instance:get_matrix_from_device())
        end)

        it("get_draw_extents", function()
            assert.is.same({ instance:get_draw_extents() }, { 0, 0, 10, 20 })
        end)

        it("get_size", function()
            assert.is.same({ instance:get_size() }, { 10, 20 })
        end)

        it("get_children", function()
            assert.is.same(instance:get_children(), {})
        end)
    end)

    it("disconnect works", function()
        local child = make_widget(nil)
        local parent = make_widget({
            make_child(child, 2, 5, matrix.create_translate(10, 0))
        })

        local extra_arg = {}
        local child_redraws, child_layouts = 0, 0
        local parent_redraws, parent_layouts = 0, 0
        local function redraw(arg, extra)
            assert.is.equal(extra_arg, extra)
            if arg:get_widget() == child then
                child_redraws = child_redraws + 1
            elseif arg:get_widget() == parent then
                parent_redraws = parent_redraws + 1
            else
                error("Unknown widget")
            end
        end
        local function layout(arg, extra)
            assert.is.equal(extra_arg, extra)
            if arg:get_widget() == child then
                child_layouts = child_layouts + 1
            elseif arg:get_widget() == parent then
                parent_layouts = parent_layouts + 1
            else
                error("Unknown widget")
            end
        end
        local context = {}
        local instance = hierarchy.new(context, parent, 15, 20, redraw, layout, extra_arg) -- luacheck: no unused

        -- There should be a connection
        parent:emit_signal("widget::redraw_needed")
        assert.is.same({ 0, 0, 1, 0 }, { child_redraws, child_layouts, parent_redraws, parent_layouts })
        child:emit_signal("widget::redraw_needed")
        assert.is.same({ 1, 0, 1, 0 }, { child_redraws, child_layouts, parent_redraws, parent_layouts })
        child:emit_signal("widget::layout_changed")
        assert.is.same({ 1, 1, 1, 0 }, { child_redraws, child_layouts, parent_redraws, parent_layouts })
        parent:emit_signal("widget::layout_changed")
        assert.is.same({ 1, 1, 1, 1 }, { child_redraws, child_layouts, parent_redraws, parent_layouts })

        -- Garbage-collect the hierarchy
        instance = nil
        collectgarbage("collect")

        -- No connections should be left
        parent:emit_signal("widget::redraw_needed")
        child:emit_signal("widget::redraw_needed")
        child:emit_signal("widget::layout_changed")
        parent:emit_signal("widget::layout_changed")
        assert.is.same({ 1, 1, 1, 1 }, { child_redraws, child_layouts, parent_redraws, parent_layouts })
    end)

    describe("children", function()
        local child, intermediate, parent
        local hierarchy_child, hierarchy_intermediate, hierarchy_parent
        before_each(function()
            child = make_widget(nil)
            intermediate = make_widget({
                make_child(child, 10, 20, matrix.create_translate(0, 5):scale(2, 2))
            })
            parent = make_widget({
                make_child(intermediate, 5, 2, matrix.create_translate(4, 0))
            })

            local function nop() end
            local context = {}
            hierarchy_parent = hierarchy.new(context, parent, 15, 16, nop, nop)

            -- This also tests get_children
            local children = hierarchy_parent:get_children()
            assert.is.equal(#children, 1)
            hierarchy_intermediate = children[1]

            children = hierarchy_intermediate:get_children()
            assert.is.equal(#children, 1)
            hierarchy_child = children[1]
        end)

        it("get_widget", function()
            assert.is.equal(hierarchy_child:get_widget(), child)
            assert.is.equal(hierarchy_intermediate:get_widget(), intermediate)
            assert.is.equal(hierarchy_parent:get_widget(), parent)
        end)

        it("get_matrix_to_parent", function()
            assert.is.equal(hierarchy_child:get_matrix_to_parent(), matrix.create(2, 0, 0, 2, 0, 5))
            assert.is.equal(hierarchy_intermediate:get_matrix_to_parent(), matrix.create_translate(4, 0))
            assert.is.equal(hierarchy_parent:get_matrix_to_parent(), matrix.identity)
        end)

        it("get_matrix_to_device", function()
            assert.is.equal(hierarchy_child:get_matrix_to_device(), matrix.create(2, 0, 0, 2, 4, 5))
            assert.is.equal(hierarchy_intermediate:get_matrix_to_device(), matrix.create_translate(4, 0))
            assert.is.equal(hierarchy_parent:get_matrix_to_device(), matrix.identity)
        end)

        it("get_matrix_from_parent", function()
            assert.is.equal(hierarchy_child:get_matrix_from_parent(), matrix.create(0.5, 0, 0, 0.5, 0, -2.5))
            assert.is.equal(hierarchy_intermediate:get_matrix_from_parent(), matrix.create_translate(-4, 0))
            assert.is.equal(hierarchy_parent:get_matrix_from_parent(), matrix.identity)
        end)

        it("get_matrix_from_device", function()
            assert.is.equal(hierarchy_child:get_matrix_from_device(), matrix.create(0.5, 0, 0, 0.5, -2, -2.5))
            assert.is.equal(hierarchy_intermediate:get_matrix_from_device(), matrix.create_translate(-4, 0))
            assert.is.equal(hierarchy_parent:get_matrix_from_device(), matrix.identity)
        end)

        it("get_draw_extents", function()
            assert.is.same({ hierarchy_child:get_draw_extents() }, { 0, 0, 10, 20 })
            assert.is.same({ hierarchy_intermediate:get_draw_extents() }, { 0, 0, 20, 45 })
            assert.is.same({ hierarchy_parent:get_draw_extents() }, { 0, 0, 24, 45 })
        end)

        it("get_size", function()
            assert.is.same({ hierarchy_child:get_size() }, { 10, 20 })
            assert.is.same({ hierarchy_intermediate:get_size() }, { 5, 2 })
            assert.is.same({ hierarchy_parent:get_size() }, { 15, 16 })
        end)
    end)

    describe("update", function()
        local child, intermediate, parent
        local instance
        local function nop() end
        before_each(function()
            child = make_widget(nil)
            intermediate = make_widget({
                make_child(child, 10, 20, matrix.create_translate(0, 5))
            })
            parent = make_widget({
                make_child(intermediate, 5, 2, matrix.create_translate(4, 0))
            })

            local context = {}
            instance = hierarchy.new(context, parent, 15, 16, nop, nop)
        end)

        it("No difference 1", function()
            local region = instance:update(context, parent, 15, 16)
            assert.is.equal(region:num_rectangles(), 0)
        end)

        it("No difference 2", function()
            local region1 = Region.create()
            local region2 = instance:update(context, parent, 15, 16, region1)
            assert.is.equal(region1, region2)
            assert.is.equal(region2:num_rectangles(), 0)
        end)

        it("child moved", function()
            -- Clear caches and change result of intermediate
            intermediate.layout = function()
                return { make_child(child, 10, 20, matrix.create_translate(0, 4)) }
            end
            intermediate:emit_signal("widget::layout_changed")

            local region = instance:update(context, parent, 15, 16)
            assert.is.equal(region:num_rectangles(), 1)
            local rect = region:get_rectangle(0)
            -- The widget drew to 4, 5, 10, 20 before and 4, 4, 10, 20 after
            assert.is.same({ rect.x, rect.y, rect.width, rect.height }, { 4, 4, 10, 21 })
        end)

        it("child disappears", function()
            -- Clear caches and change result of intermediate
            intermediate.layout = function() end
            intermediate:emit_signal("widget::layout_changed")

            local region = instance:update(context, parent, 15, 16)
            assert.is.equal(region:num_rectangles(), 1)
            local rect = region:get_rectangle(0)
            -- The child was drawn to 4, 5, 10, 20
            assert.is.same({ rect.x, rect.y, rect.width, rect.height }, { 4, 5, 10, 20 })
        end)

        it("widget changed", function()
            -- Clear caches and change result of parent
            local new_intermediate = make_widget({
                make_child(child, 10, 20, matrix.create_translate(0, 5))
            })
            parent.layout = function()
                return { make_child(new_intermediate, 5, 2, matrix.create_translate(4, 0)) }
            end
            parent:emit_signal("widget::layout_changed")

            local region = instance:update(context, parent, 15, 16)
            assert.is.equal(region:num_rectangles(), 1)
            local rect = region:get_rectangle(0)
            -- Intermediate drew to 4, 0, 5, 2 (and so does new_intermediate)
            assert.is.same({ rect.x, rect.y, rect.width, rect.height }, { 4, 0, 5, 2 })
        end)
    end)

    describe("widget counts", function()
        local child, intermediate, parent
        local unrelated
        local context, instance
        before_each(function()
            local function nop() end
            context = {}
            child = make_widget(nil)
            intermediate = make_widget({
                make_child(child, 10, 20, matrix.identity)
            })
            parent = make_widget({
                make_child(intermediate, 10, 20, matrix.identity)
            })
            unrelated = make_widget(nil)

            hierarchy.count_widget(child)
            hierarchy.count_widget(parent)
            hierarchy.count_widget(unrelated)
            instance = hierarchy.new(context, parent, 10, 20, nop, nop)
        end)

        it("basic counts", function()
            local unrelated_other = make_widget(nil)
            assert.is.equal(1, instance:get_count(child))
            -- intermediate was not passed to hierarchy.count_widget()!
            assert.is.equal(0, instance:get_count(intermediate))
            assert.is.equal(1, instance:get_count(parent))
            assert.is.equal(0, instance:get_count(unrelated))
            assert.is.equal(0, instance:get_count(unrelated_other))
        end)

        it("after update", function()
            -- Replace child and intermediate by just a new_child
            local new_child = make_widget(nil)
            parent.layout = function()
                return { make_child(new_child, 10, 20, matrix.identity) }
            end
            parent:emit_signal("widget::layout_changed")
            instance:update(context, parent, 10, 20)

            assert.is.equal(0, instance:get_count(child))
            -- new_child was not passed to hierarchy.count_widget()!
            assert.is.equal(0, instance:get_count(new_child))
            assert.is.equal(1, instance:get_count(parent))
            assert.is.equal(0, instance:get_count(unrelated))
        end)

        it("collectible", function()
            -- This test that hierarchy.count_widget() does not prevent garbage collection of the widget.
            local weak = setmetatable({}, { __mode = "v"})
            weak[1], weak[2], weak[3], weak[4] = child, intermediate, parent, instance
            child, intermediate, parent, instance = nil, nil, nil, nil

            assert.is.equal(4, #weak)
            collectgarbage("collect")
            assert.is.equal(0, #weak)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
