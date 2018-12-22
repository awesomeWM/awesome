
local gtable = require("gears.table")

describe("gears.table", function()
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

    describe("table.join", function()
        it("nil argument", function()
            local t = gtable.join({"a"}, nil, {"b"})
            assert.is.same(t, {"a", "b"})
        end)
    end)

    it("table.slice", function()
        local t = { "a", "b", c = "c", "d" }
        assert.is.same(gtable.slice(t), { "a", "b", "d" })
        assert.is.same(gtable.slice(t, 1, 1), {})
        assert.is.same(gtable.slice(t, 1, 2), { "a" })
        assert.is.same(gtable.slice(t, 2, 3), { "b" })
        assert.is.same(gtable.slice(t, 0, 10), { "a", "b", "d" })

        assert.is.same(gtable.slice(t, 0, -1), { "a", "b" })
        assert.is.same(gtable.slice(t, -1, -1), { })
        assert.is.same(gtable.slice(t, -2), { "b", "d" })
        assert.is.same(gtable.slice(t, -2, -2), { })

        assert.is.same(gtable.slice(t, 0, nil, 1), { "a", "b", "d" })
        assert.is.same(gtable.slice(t, 0, -1, 2), { "b" })
    end)
end)
