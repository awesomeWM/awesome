
local gtable = require("gears.table")

describe("gears.table", function()
    it("table.keys", function()
        local t = { 1, a = 2, 3 }
        assert.is.same(gtable.keys(t), { 1, 2, "a" })
    end)

    describe("gears.table.count_keys", function()
        it("counts keys in an empty table", function()
            local t = {}
            assert.is.same(gtable.count_keys(t), 0)
        end)

        it("counts keys in a sparse array", function()
            local t = { 1, nil, 3 }
            assert.is.same(gtable.count_keys(t), 2)
        end)

        it("counts keys in a regular array", function()
            local t = { 1, 2, 3 }
            assert.is.same(gtable.count_keys(t), 3)
        end)

        it("counts keys in a hash table", function()
            local t = { a = 1, b = "2", c = true }
            assert.is.same(gtable.count_keys(t), 3)
        end)

        it("counts keys in a mixed table", function()
            local t = { 1, a = 2, nil, 4 }
            assert.is.same(gtable.count_keys(t), 3)
        end)

    end)

    it("table.keys_filter", function()
        local t = { "a", 1, function() end, false}
        assert.is.same(gtable.keys_filter(t, "number", "function"), { 2, 3 })
    end)

    it("table.reverse", function()
        local t = { "a", "b", c = "c", "d" }
        assert.is.same(gtable.reverse(t), { "d", "b", "a", c = "c" })
    end)

    describe("table.iterate", function()
        it("no filter", function()
            local t = { "a", "b", c = "c", "d" }
            local f = gtable.iterate(t, function() return true end)
            assert.is.equal(f(), "a")
            assert.is.equal(f(), "b")
            assert.is.equal(f(), "d")
            assert.is.equal(f(), nil)
            assert.is.equal(f(), nil)
        end)

        it("b filter", function()
            local t = { "a", "b", c = "c", "d" }
            local f = gtable.iterate(t, function(i) return i == "b" end)
            assert.is.equal(f(), "b")
            assert.is.equal(f(), nil)
            assert.is.equal(f(), nil)
        end)

        it("with offset", function()
            local t = { "a", "b", c = "c", "d" }
            local f = gtable.iterate(t, function() return true end, 2)
            assert.is.equal(f(), "b")
            assert.is.equal(f(), "d")
            assert.is.equal(f(), "a")
            assert.is.equal(f(), nil)
            assert.is.equal(f(), nil)
        end)
    end)


    describe("table.cycle_value", function()
        it("nil argument", function()
            local t = { "a", "b", "c", "d" }
            local f = gtable.cycle_value(t, "a")
            assert.is.same(f, "b")
        end)
        it("with step size", function()
            local t = { "a", "b", "c", "d" }
            local f = gtable.cycle_value(t, "a", 2)
            assert.is.same(f, "c")
        end)
        it("b filter", function()
            local t = { "a", "b", "c", "d" }
            local f = gtable.cycle_value(t, "a", 1, function(i) return i == "b" end)
            assert.is.equal(f, "b")
        end)
        it("e filter", function()
            local t = { "a", "b", "c", "d" }
            local f = gtable.cycle_value(t, "a", 1, function(i) return i == "e" end)
            assert.is.equal(f, nil)
        end)
        it("b filter and step size", function()
            local t = { "a", "b", "c", "d" }
            local f = gtable.cycle_value(t, "b", 2, function(i) return i == "b" end)
            assert.is.equal(f, nil)
        end)
    end)

    describe("table.find_keys", function()
        it("nil argument", function()
            local t = { "a", "b", c = "c", "d" }
            local f = gtable.find_keys(t, function(k) return k == "c" end)
            assert.is.same(f, {"c"})
        end)
    end)

    describe("table.hasitem", function()
        it("exist", function()
            local t = {"a", "b", c = "c"}
            local f = gtable.hasitem(t, "c")
            assert.is.equal(f, "c")
        end)
        it("nil", function()
            local t = {"a", "b"}
            local f = gtable.hasitem(t, "c")
            assert.is.equal(f, nil)
        end)
    end)

    describe("table.join", function()
        it("nil argument", function()
            local t = gtable.join({"a"}, nil, {"b"})
            assert.is.same(t, {"a", "b"})
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
