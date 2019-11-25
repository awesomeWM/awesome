
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
