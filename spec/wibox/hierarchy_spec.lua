---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
---------------------------------------------------------------------------

local hierarchy = require("wibox.hierarchy")

local cairo = require("lgi").cairo
local matrix = require("gears.matrix")
local object = require("gears.object")

local function make_widget(children)
    local result = object()
    result:add_signal("widget::redraw_needed")
    result:add_signal("widget::layout_changed")
    result._layout_cache = {
        get = function(self, width, height)
            return children
        end
    }
    return result
end

local function make_child(widget, width, height, matrix)
    return { _widget = widget, _width = width, _height = height, _matrix = matrix }
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

        it("get_parent", function()
            assert.is_nil(instance:get_parent())
        end)

        it("get_root", function()
            assert.is.equal(instance:get_root(), instance)
        end)

        it("get_widget", function()
            assert.is.equal(instance:get_widget(), widget)
        end)

        it("get_matrix_to_parent", function()
            assert.is_true(matrix.equals(cairo.Matrix.create_identity(),
                    instance:get_matrix_to_parent()))
        end)

        it("get_matrix_to_device", function()
            assert.is_true(matrix.equals(cairo.Matrix.create_identity(),
                    instance:get_matrix_to_device()))
        end)

        it("get_matrix_from_parent", function()
            assert.is_true(matrix.equals(cairo.Matrix.create_identity(),
                    instance:get_matrix_from_parent()))
        end)

        it("get_matrix_from_device", function()
            assert.is_true(matrix.equals(cairo.Matrix.create_identity(),
                    instance:get_matrix_from_device()))
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
            make_child(child, 2, 5, cairo.Matrix.create_translate(10, 0))
        })

        local child_redraws, child_layouts = 0, 0
        local parent_redraws, parent_layouts = 0, 0
        local function redraw(arg)
            if arg:get_widget() == child then
                child_redraws = child_redraws + 1
            elseif arg:get_widget() == parent then
                parent_redraws = parent_redraws + 1
            else
                error("Unknown widget")
            end
        end
        local function layout(arg)
            if arg:get_widget() == child then
                child_layouts = child_layouts + 1
            elseif arg:get_widget() == parent then
                parent_layouts = parent_layouts + 1
            else
                error("Unknown widget")
            end
        end
        local context = {}
        local instance = hierarchy.new(context, parent, 15, 20, redraw, layout)

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
                make_child(child, 10, 20, cairo.Matrix.create_translate(0, 5))
            })
            parent = make_widget({
                make_child(intermediate, 5, 2, cairo.Matrix.create_translate(4, 0))
            })

            local function nop() end
            local context = {}
            hierarchy_parent = hierarchy.new(context, parent, 15, 16, nop, nop)

            -- This also tests get_children
            local children = hierarchy_parent:get_children()
            assert.is.equal(#children, 1)
            hierarchy_intermediate = children[1]

            local children = hierarchy_intermediate:get_children()
            assert.is.equal(#children, 1)
            hierarchy_child = children[1]
        end)

        it("get_parent", function()
            assert.is.equal(hierarchy_child:get_parent(), hierarchy_intermediate)
            assert.is.equal(hierarchy_intermediate:get_parent(), hierarchy_parent)
            assert.is_nil(hierarchy_parent:get_parent())
        end)

        it("get_root", function()
            assert.is.equal(hierarchy_child:get_root(), hierarchy_parent)
            assert.is.equal(hierarchy_intermediate:get_root(), hierarchy_parent)
            assert.is.equal(hierarchy_parent:get_root(), hierarchy_parent)
        end)

        it("get_widget", function()
            assert.is.equal(hierarchy_child:get_widget(), child)
            assert.is.equal(hierarchy_intermediate:get_widget(), intermediate)
            assert.is.equal(hierarchy_parent:get_widget(), parent)
        end)

        it("get_matrix_to_parent", function()
            assert.is_true(matrix.equals(hierarchy_child:get_matrix_to_parent(), cairo.Matrix.create_translate(0, 5)))
            assert.is_true(matrix.equals(hierarchy_intermediate:get_matrix_to_parent(), cairo.Matrix.create_translate(4, 0)))
            assert.is_true(matrix.equals(hierarchy_parent:get_matrix_to_parent(), cairo.Matrix.create_identity()))
        end)

        it("get_matrix_to_device", function()
            assert.is_true(matrix.equals(hierarchy_child:get_matrix_to_device(), cairo.Matrix.create_translate(4, 5)))
            assert.is_true(matrix.equals(hierarchy_intermediate:get_matrix_to_device(), cairo.Matrix.create_translate(4, 0)))
            assert.is_true(matrix.equals(hierarchy_parent:get_matrix_to_device(), cairo.Matrix.create_identity()))
        end)

        it("get_matrix_from_parent", function()
            assert.is_true(matrix.equals(hierarchy_child:get_matrix_from_parent(), cairo.Matrix.create_translate(0, -5)))
            assert.is_true(matrix.equals(hierarchy_intermediate:get_matrix_from_parent(), cairo.Matrix.create_translate(-4, 0)))
            assert.is_true(matrix.equals(hierarchy_parent:get_matrix_from_parent(), cairo.Matrix.create_identity()))
        end)

        it("get_matrix_from_device", function()
            assert.is_true(matrix.equals(hierarchy_child:get_matrix_from_device(), cairo.Matrix.create_translate(-4, -5)))
            assert.is_true(matrix.equals(hierarchy_intermediate:get_matrix_from_device(), cairo.Matrix.create_translate(-4, 0)))
            assert.is_true(matrix.equals(hierarchy_parent:get_matrix_from_device(), cairo.Matrix.create_identity()))
        end)

        it("get_draw_extents", function()
            assert.is.same({ hierarchy_child:get_draw_extents() }, { 0, 0, 10, 20 })
            assert.is.same({ hierarchy_intermediate:get_draw_extents() }, { 0, 0, 10, 25 })
            assert.is.same({ hierarchy_parent:get_draw_extents() }, { 0, 0, 15, 25 })
        end)

        it("get_size", function()
            assert.is.same({ hierarchy_child:get_size() }, { 10, 20 })
            assert.is.same({ hierarchy_intermediate:get_size() }, { 5, 2 })
            assert.is.same({ hierarchy_parent:get_size() }, { 15, 16 })
        end)
    end)

    describe("find_differences", function()
        local child, intermediate, parent
        local instance
        local function nop() end
        before_each(function()
            child = make_widget(nil)
            intermediate = make_widget({
                make_child(child, 10, 20, cairo.Matrix.create_translate(0, 5))
            })
            parent = make_widget({
                make_child(intermediate, 5, 2, cairo.Matrix.create_translate(4, 0))
            })

            local context = {}
            instance = hierarchy.new(context, parent, 15, 16, nop, nop)
        end)

        it("No difference", function()
            local context = {}
            local instance2 = hierarchy.new(context, parent, 15, 16, nop, nop)
            local region = instance:find_differences(instance2)
            assert.is.equal(region:num_rectangles(), 0)
        end)

        it("child moved", function()
            intermediate._layout_cache.get = function()
                return { make_child(child, 10, 20, cairo.Matrix.create_translate(0, 4)) }
            end
            local context = {}
            local instance2 = hierarchy.new(context, parent, 15, 16, nop, nop)
            local region = instance:find_differences(instance2)
            assert.is.equal(region:num_rectangles(), 1)
            local rect = region:get_rectangle(0)
            -- The widget drew to 4, 5, 10, 20 before and 4, 4, 10, 20 after
            assert.is.same({ rect.x, rect.y, rect.width, rect.height }, { 4, 4, 10, 21 })
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
