---------------------------------------------------------------------------
--- Extra sorting algorithms.
--
-- @submodule gears.sort
---------------------------------------------------------------------------

local tsort  = {}
local gtable = require("gears.table")

local mt = { __index = tsort }

local function add_node(self, node)
    if not self._edges[node] then
        self._edges[node] = {}
    end
end

--- Ensure that `node` appears after all `dependencies`.
-- @param node The node that edges are added to.
-- @tparam table dependencies List of nodes that have to appear before `node`.
-- @noreturn
-- @method append
function tsort:append(node, dependencies)
    add_node(self, node)
    for _, dep in ipairs(dependencies) do
        add_node(self, dep)
        self._edges[node][dep] = true
    end
end

--- Ensure that `node` appears before all `subordinates`.
-- @param node The node that edges are added to.
-- @tparam table subordinates List of nodes that have to appear after `node`.
-- @noreturn
-- @method prepend
function tsort:prepend(node, subordinates)
    for _, dep in ipairs(subordinates) do
        self:append(dep, { node })
    end
end

local HANDLING, DONE = 1, 2

local function visit(result, self, state, node)
    if state[node] == DONE then
        -- This node is already in the output
        return
    end
    if state[node] == HANDLING then
        -- We are handling this node already and managed to visit it again
        -- from itself. Thus, there must be a loop.
        result.BAD = node
        return true
    end

    state[node] = HANDLING
    -- Before this node, all nodes that it depends on must appear
    for dep in pairs(self._edges[node]) do
        if visit(result, self, state, dep) then
            return true
        end
    end
    state[node] = DONE
    table.insert(result, node)
end

--- Create a copy of this topological sort.
-- This is useful to backup it before adding elements that can potentially
-- have circular dependencies and thus render the original useless.
-- @treturn gears.sort.topological The cloned sorter object.
-- @method clone
function tsort:clone()
    local new = tsort.topological()

    -- Disable deep copy as the sorted values may be objects or tables
    new._edges = gtable.clone(self._edges, false)

    return new
end

--- Remove a node from the topological map.
--
-- @param node The node
-- @noreturn
-- @method remove
function tsort:remove(node)
    self._edges[node] = nil
    for _, deps in pairs(self._edges) do
        deps[node] = nil
    end
end

--- Try to sort the nodes.
-- @treturn[1] table A sorted list of nodes
-- @treturn[2] nil
-- @return[2] A node around which a loop exists
-- @method sort
function tsort:sort()
    local result, state = {}, {}
    for node in pairs(self._edges) do
        if visit(result, self, state, node) then
            return nil, result.BAD
        end
    end
    return result
end

--- A topological sorting class.
--
-- The object returned by this function allows to create simple dependency
-- graphs. It can be used for decision making or ordering of complex sequences.
--
--
--@DOC_text_gears_sort_topological_EXAMPLE@
--
-- @constructorfct gears.sort.topological

function tsort.topological()
    return setmetatable({
        _edges = {},
    }, mt)
end

return setmetatable(tsort, {__call = function(_, ...)
    return tsort.topological(...)
end})
