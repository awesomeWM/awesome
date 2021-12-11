#!/usr/bin/env lua

-- This uses the "depgraph" package to parse all source code and look for calls
-- to require(). For all these uses, we make sure that it is allowed by the
-- white-list below. This is done to mitigate the risk of cyclic dependencies
-- between modules and to (hopefully) enforce a bit of sane code structure.

local have_depgraph, depgraph = pcall(require, "depgraph")

if not have_depgraph then
    print("depgraph not found")
    print(depgraph)
    print("(this error is non-fatal; we just skip its use)")
    os.exit(0)
end

local allowed_deps = {
    gears = {
        lgi = true,
    },
    beautiful = {
        gears = true,
        lgi = true,
    },
    wibox = {
        beautiful = true,
        gears = true,
        lgi = true,
    },
    awful = {
        beautiful = true,
        gears = true,
        lgi = true,
        wibox = true,

        -- Necessary to lazy-load the deprecated modules.
        ["awful.*"] = true,

        -- For legacy reasons.
        ruled = true
    },
    naughty = {
        awful = true,
        beautiful = true,
        gears = true,
        lgi = true,
        wibox = true,
    },
    menubar = {
        awful = true,
        beautiful = true,
        gears = true,
        lgi = true,
        wibox = true,
    },
    ruled = {
        awful = true,
        beautiful = true,
        gears = true,
        lgi = true,
        wibox = true,
        naughty = true,
    },
    -- TODO: Get rid of these
    ["gears.surface"]        = {
        ["wibox.hierarchy"] = true,
        beautiful = true,
    },
}

-- Turn "foo.bar.baz" into "foo.bar". Returns nil if there is nothing more to
-- remove.
local function get_supermodule(module)
    return string.match(module, "(.+)%.[a-z_]+")
end

-- Check if "module" (or one of its parents) is allowed to depend on
-- "dependency" (or one of its parents).
local function is_allowed(module, dependency)
    assert(module ~= nil)
    -- If both have a common parent, the dependency is allowed
    do
        local last_mod, mod = module, module
        while mod do
            last_mod, mod = mod, get_supermodule(mod)
        end
        local last_dep, dep = dependency, dependency
        while dep do
            last_dep, dep = dep, get_supermodule(dep)
        end
        if last_mod == last_dep then
            return true
        end
    end
    -- Check the allowed_deps table
    while module ~= nil do
        local allowed = allowed_deps[module]
        if allowed then
            local dep = dependency
            while dep ~= nil do
                if allowed[dep] then
                    return true
                end
                dep = get_supermodule(dep)
            end
        end
        module = get_supermodule(module)
    end
    return false
end

-- Generate the dependency graph
local graph = assert(depgraph.make_graph({"lib"}, {}, "lib", nil, nil))

-- Check if all require's are allowed by the above table
local had_violation = false
for _, module in ipairs(graph.modules) do
    for _, dep in ipairs(module.deps) do
        if not is_allowed(module.name, dep.name) then
            had_violation = true
            print(string.format("Dependency from %s onto %s is not allowed",
                    module.name, dep.name))
        end
    end
end

if had_violation then
    os.exit(1)
end

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
