local gears = {sort={topological = require("gears.sort.topological")}} --DOC_HIDE

local tsort = gears.sort.topological()
tsort:prepend('a', { 'b' })
tsort:prepend('b', { 'c' })
tsort:prepend('c', { 'd' })
tsort:append('e', { 'd' })
tsort:append('f', { 'e', 'd' })

local res = assert(tsort:sort())

for k, v in ipairs(res) do
    print("The position #"..k.." is: "..v)
end

assert(#res == 6) --DOC_HIDE
assert(res[1] == 'a') --DOC_HIDE
assert(res[2] == 'b') --DOC_HIDE
assert(res[3] == 'c') --DOC_HIDE
assert(res[4] == 'd') --DOC_HIDE
assert(res[5] == 'e') --DOC_HIDE
assert(res[6] == 'f') --DOC_HIDE
