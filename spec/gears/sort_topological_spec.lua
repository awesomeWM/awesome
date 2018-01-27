local tsort = require("gears.sort.topological")

describe("gears.sort.topological", function()
    describe("prepend_1", function()
        local ts = tsort.topological()
        ts:prepend(1, { 2, 3, 4 })
        ts:prepend(2, { 3, 4 })
        ts:prepend(3, { 4 })
        local res = ts:sort()

        assert.is.equal(type(res), "table")

        assert(#res == 4)
        for k, v in pairs(res) do
            assert.is.equal(k,  v)
        end
    end)

    describe("append_1", function()

        local ts = tsort.topological()
        ts:append(4, { 1, 2, 3 })
        ts:append(3, { 1, 2 })
        ts:append(2, { 1 })
        local res = ts:sort()

        assert.is.equal(type(res), "table")

        assert(#res == 4)
        for k, v in pairs(res) do
            assert.is.equal(k,  v)
        end
    end)

    describe("mixed_1", function()
        local ts = tsort.topological()

        ts:prepend(1, { 2, 3 })
        ts:append(3, { 1, 2 })
        ts:append(2, { 1 })
        ts:prepend(2, { 3 })

        local res, err = ts:sort()
        assert.is.equal(res[1], 1 )
        assert.is.equal(res[2], 2 )
        assert.is.equal(res[3], 3 )
        assert.is.equal(type(err), "nil")
    end)

    describe("mixed_2", function()
        local ts = tsort.topological()
        ts:append(11, { 2, 9, 3, 5, 7, 10 })
        ts:append(8, { 7, 3 })
        ts:prepend(8, { 9 })
        ts:prepend(3, { 10 })
        ts:prepend(3, { 7, 5 })
        ts:prepend(2, { 3, 5 })
        ts:append(5, { 3 })
        ts:append(10, { 9 })
        ts:prepend(5, { 7 })

        local res, err = ts:sort()
        assert.is.equal(type(res), "table")
        assert.is.equal(type(err), "nil")

        assert.is.equal(#res, 8 )

        assert.is.equal(res[1], 2)
        assert.is.equal(res[2], 3)
        assert.is.equal(res[3], 5)
        assert.is.equal(res[4], 7)
        assert.is.equal(res[5], 8)
        assert.is.equal(res[6], 9)
        assert.is.equal(res[7], 10)
        assert.is.equal(res[8], 11)
    end)

    describe("mixed_2_and_clone", function()
        local ts = tsort.topological()
        ts:append(11, { 2, 9, 3, 5, 7, 10 })
        ts:append(8, { 7, 3 })
        ts:prepend(8, { 9 })
        ts:prepend(3, { 10 })
        ts:prepend(3, { 7, 5 })
        ts:prepend(2, { 3, 5 })
        ts:append(5, { 3 })
        ts:append(10, { 9 })
        ts:prepend(5, { 7 })

        local res, err = ts:sort()
        assert.is.equal(type(res), "table")
        assert.is.equal(type(err), "nil")

        assert.is.equal(#res, 8 )

        assert.is.equal(res[1], 2)
        assert.is.equal(res[2], 3)
        assert.is.equal(res[3], 5)
        assert.is.equal(res[4], 7)
        assert.is.equal(res[5], 8)
        assert.is.equal(res[6], 9)
        assert.is.equal(res[7], 10)
        assert.is.equal(res[8], 11)

        local ts2 = ts:clone()
        local res2, err2 = ts2:sort()
        assert.is.equal(type(res2), "table")
        assert.is.equal(type(err2), "nil")
        assert.is.equal(#res, #res2)

        for i=1, #res do
            assert.is.equal(res[i], res2[i])
        end
    end)

    describe("simple_remove", function()
        local ts = tsort.topological()

        ts:prepend(1, { 2, 3 })
        ts:append(3, { 1, 2 })
        ts:append(2, { 1 })
        ts:prepend(2, { 3 })

        local res, err = ts:sort()

        assert.is.equal(#res, 3)
        assert.is.equal(type(err), "nil")

        ts:remove(2)

        res, err = ts:sort()
        assert.is.equal(#res, 2)
        assert.is.equal(type(err), "nil")
    end)

    describe("simple_error_1", function()
        local ts = tsort.topological()
        ts:append(2, { 2 })

        local res, err = ts:sort()
        assert.is.equal(type(err), "number")
        assert.is.equal(type(res), "nil")
    end)

    describe("simple_error_2", function()
        local ts = tsort.topological()
        ts:append(2, { 2, 3 })

        local res, err = ts:sort()
        assert.is.equal(type(err), "number")
        assert.is.equal(type(res), "nil")
    end)

    describe("simple_error_3", function()
        local ts = tsort.topological()
        ts:append(2, { 3 })
        ts:append(3, { 2 })

        local res, err = ts:sort()
        assert.is.equal(type(err), "number")
        assert.is.equal(type(res), "nil")
    end)

    describe("simple_error_4", function()
        local ts = tsort.topological()
        ts:prepend(2, { 3 })
        ts:prepend(3, { 2 })

        local res, err = ts:sort()
        assert.is.equal(type(err), "number")
        assert.is.equal(type(res), "nil")
    end)
end)
