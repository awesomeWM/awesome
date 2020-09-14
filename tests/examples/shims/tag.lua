local gears_obj = require("gears.object")
local gtable = require("gears.table")

local tag, meta = awesome._shim_fake_class()

local function has_selected_tag(s)
    for _, t in ipairs(root._tags) do
        if t.selected and ((not s) or s == t.screen) then
            return true
        end
    end
    return false
end

local function get_clients(self)
    local list = {}

    for _, c in ipairs(client.get()) do
        if #c:tags() > 0 then
            for _, t in ipairs(c:tags()) do
                if t == self then
                    table.insert(list, c)
                end
            end
        end
    end

    return list
end

local function new_tag(_, args)
    local ret = gears_obj()
    awesome._forward_class(ret, tag)

    ret._private = {}
    ret.name = tostring(args.name) or "test"
    ret.activated = true
    ret.selected = not has_selected_tag(args.screen)

    -- Deprecated.
    ret.data = ret._private

    function ret:clients(new_clients)
        local old_clients = get_clients(ret)

        -- Remove the old clients.
        if new_clients then
            for _, c in ipairs(client.get()) do
                local had_client = gtable.hasitem(old_clients, c)
                local has_client = gtable.hasitem(new_clients, c)
                 if had_client and not has_client then
                    local ctags = gtable.clone(c:tags(), false)

                    for k, t in ipairs(ctags) do
                        if t == self then
                            table.remove(ctags, k)
                            c:tags(ctags)
                        end
                    end
                end
            end

            -- Add the new clients.
            for _, c in ipairs(new_clients) do

                if new_clients and not gtable.hasitem(old_clients, c) then
                    local ctags = gtable.clone(c:tags(), false)
                    table.insert(ctags, self)
                    c:tags(ctags)
                end
            end
        end

        -- Generate the client list.
        local list = get_clients(ret)

        return list
    end

    table.insert(root._tags, ret)

    return setmetatable(ret, {
                        __index     = function(...) return meta.__index(...) end,
                        __newindex = function(...) return meta.__newindex(...) end
                    })
end

return setmetatable(tag, {
                    __call      = new_tag,
                })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
