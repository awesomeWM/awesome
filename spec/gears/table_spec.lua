
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
end)
