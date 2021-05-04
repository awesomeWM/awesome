---------------------------------------------------------------------------
--- A helper module for computations involving Bézier curves
--
-- @author Alex Belykh &lt;albel727@ngs.ru&gt;
-- @copyright 2021 Alex Belykh
-- @submodule gears.math
---------------------------------------------------------------------------

local table_insert = table.insert

local bezier = {}

--- Compute the value of a Bézier curve at a given value of the t parameter.
--
-- This function evaluates the given curve `B` of an arbitrary degree
-- at a given point t.
--
-- @tparam {number,...} c The table of control points of the curve.
-- @tparam number t The value of the t parameter to evaluate the curve at.
-- @treturn[1] number The value of `B(t)`.
-- @treturn[2] nil `nil`, if c is empty.
-- @staticfct gears.math.bezier.curve_evaluate_at
function bezier.curve_evaluate_at(c, t)
    local from = c
    local tmp = {nil, nil, nil, nil}
    while #from > 1 do
        for i = 1, #from-1 do
            tmp[i] = from[i]*(1-t) + from[i+1]*t
        end
        tmp[#from] = nil
        from = tmp
    end

    return from[1]
end

--- Split a Bézier curve into two curves at a given value of the t parameter.
--
-- This function splits the given curve `B` of an arbitrary degree at a point t
-- into two curves of the same degree `B_left` and `B_right`, such that
-- `B_left(0)=B(0)`, `B_left(1)=B(t)=B_right(0)`, `B_right(1)=B(1)`.
--
-- @tparam {number,...} c The table of control points of the curve.
-- @tparam number t The value of the t parameter to split the curve at.
-- @treturn {number,...} The table of control points for `B_left`.
-- @treturn {number,...} The table of control points for `B_right`.
-- @staticfct gears.math.bezier.curve_split_at
function bezier.curve_split_at(c, t)
    local coefs_left, coefs_right = {}, {}
    local from = c
    local tmp = {nil, nil, nil, nil}
    while #from > 0 do
        table_insert(coefs_left, from[1])
        table_insert(coefs_right, 1, from[#from])
        for i = 1, #from-1 do
            tmp[i] = from[i]*(1-t) + from[i+1]*t
        end
        tmp[#from] = nil
        from = tmp
    end

    return coefs_left, coefs_right
end

--- Get the n-th derivative Bézier curve of a Bézier curve.
--
-- This function computes control points for the curve that is
-- the derivative of order `n` in `t`, i.e. `B^(n)(t)`,
-- of the given curve `B(t)` of an arbitrary degree.
--
-- @tparam {number,...} c The table of control points of the curve.
-- @tparam[opt=1] integer n The order of the derivative to take.
-- @treturn[1] {number,...} The table of control points of `B^(n)(t)`.
-- @treturn[2] nil If n is less than 0.
-- @staticfct gears.math.bezier.curve_derivative
function bezier.curve_derivative(c, n)
    n = n or 1
    if n < 0 then
        return
    end
    if n < 1 then
        return c
    end
    local c_len = #c
    if c_len < n+1 then
        return {}
    end

    local from = c
    local tmp = {}

    for l = c_len-1, c_len-n, -1 do
        for i = 1, l do
            tmp[i] = (from[i+1] - from[i])*l
        end
        tmp[l+1] = nil
        from = tmp
    end

    return from
end

-- This is used instead of plain 0 to try and be compatible
-- with objects that implement their own arithmetic via metatables.
local function get_zero(c, zero)
    return c and c*0 or zero
end

--- Compute the value of the n-th derivative of a Bézier curve
--- at a given value of the t parameter.
--
-- This is roughly the same as
-- `curve_evaluate_at(curve_derivative(c, n), t)`, but the latter
-- would throw errors or return nil instead of 0 in some cases.
--
-- @tparam {number,...} c The table of control points of the curve.
-- @tparam number t The value of the t parameter to compute the derivative at.
-- @tparam[opt=1] integer n The order of the derivative to take.
-- @tparam[opt=nil] number|nil zero The value to return if c is empty.
-- @treturn[1] number The value of `B^(n)(t)`.
-- @treturn[2] nil nil, if n is less than 0.
-- @treturn[3] number|nil The value of the zero parameter, if c is empty.
-- @staticfct gears.math.bezier.curve_derivative_at
function bezier.curve_derivative_at(c, t, n, zero)
    local d = bezier.curve_derivative(c, n)
    if not d then
        return
    end

    return bezier.curve_evaluate_at(d, t) or get_zero(c[1], zero)
end

--- Compute the value of the 1-st derivative of a Bézier curve at t=0.
--
-- This is the same as `curve_derivative_at(c, 0)`, but since it's particularly
-- computationally simple and useful in practice, it has its own function.
--
-- @tparam {number,...} c The table of control points of the curve.
-- @tparam[opt=nil] number|nil zero The value to return if c is empty.
-- @treturn[1] number The value of `B'(0)`.
-- @treturn[2] number|nil The value of the zero parameter, if c is empty.
-- @staticfct gears.math.bezier.curve_derivative_at_zero
function bezier.curve_derivative_at_zero(c, zero)
    local l = #c
    if l < 2 then
        return get_zero(c[1], zero)
    end
    return (c[2]-c[1])*(l-1)
end

--- Compute the value of the 1-st derivative of a Bézier curve at t=1.
--
-- This is the same as `curve_derivative_at(c, 1)`, but since it's particularly
-- computationally simple and useful in practice, it has its own function.
--
-- @tparam {number,...} c The table of control points of the curve.
-- @tparam[opt=nil] number|nil zero The value to return if c is empty.
-- @treturn[1] number The value of `B'(1)`.
-- @treturn[2] number|nil The value of the zero parameter, if c is empty.
-- @staticfct gears.math.bezier.curve_derivative_at_one
function bezier.curve_derivative_at_one(c, zero)
    local l = #c
    if l < 2 then
        return get_zero(c[1], zero)
    end
    return (c[l]-c[l-1])*(l-1)
end

--- Get the (n+1)-th degree Bézier curve, that has the same shape as
-- a given n-th degree Bézier curve.
--
-- Given the control points of a curve B of degree n, this function computes
-- the control points for the curve Q, such that `Q(t) = B(t)`, and
-- Q has the degree n+1, i.e. it has one control point more.
--
-- @tparam {number,...} c The table of control points of the curve B.
-- @treturn {number,...} The table of control points of the curve Q.
-- @staticfct gears.math.bezier.curve_elevate_degree
function bezier.curve_elevate_degree(c)
    local ret = {c[1]}
    local len = #c

    for i = 1, len-1 do
        ret[i+1] = (i*c[i] + (len-i)*c[i+1])/len
    end

    ret[len+1] = c[len]
    return ret
end

--- Get a cubic Bézier curve that passes through given points (up to 4).
--
-- This function takes up to 4 values and returns the 4 control points
-- for a cubic curve
--
-- `B(t) = c0\*(1-t)^3 + 3\*c1\*t\*(1-t)^2 + 3\*c2\*t^2\*(1-t) + c3\*t^3`,
-- that takes on these values at equidistant values of the t parameter.
--
-- If only p0 is given, `B(0)=B(1)=B(for all t)=p0`.
--
-- If p0 and p1 are given, `B(0)=p0` and `B(1)=p1`.
--
-- If p0, p1 and p2 are given, `B(0)=p0`, `B(1/2)=p1` and `B(1)=p2`.
--
-- For 4 points given, `B(0)=p0`, `B(1/3)=p1`, `B(2/3)=p2`, `B(1)=p3`.
--
-- @tparam number p0
-- @tparam[opt] number p1
-- @tparam[opt] number p2
-- @tparam[opt] number p3
-- @treturn number c0
-- @treturn number c1
-- @treturn number c2
-- @treturn number c3
-- @staticfct gears.math.bezier.cubic_through_points
function bezier.cubic_through_points(p0, p1, p2, p3)
    if not p1 then
        return p0, p0, p0, p0
    end
    if not p2 then
        local c1 = (2*p0+p1)/3
        local c2 = (2*p1+p0)/3
        return p0, c1, c2, p1
    end
    if not p3 then
        local c1 = (4*p1 - p2)/3
        local c2 = (4*p1 - p0)/3
        return p0, c1, c2, p2
    end
    local c1 = (-5*p0 + 18*p1 - 9*p2 + 2*p3)/6
    local c2 = (-5*p3 + 18*p2 - 9*p1 + 2*p0)/6
    return p0, c1, c2, p3
end

--- Get a cubic Bézier curve with given values and derivatives at endpoints.
--
-- This function computes the 4 control points for the cubic curve B, such that
-- `B(0)=p0`, `B'(0)=d0`, `B(1)=p3`, `B'(1)=d3`.
--
-- @tparam number d0 The value of the derivative at t=0.
-- @tparam number p0 The value of the curve at t=0.
-- @tparam number p3 The value of the curve at t=1.
-- @tparam number d3 The value of the derivative at t=1.
-- @treturn number c0
-- @treturn number c1
-- @treturn number c2
-- @treturn number c3
-- @staticfct gears.math.bezier.cubic_from_points_and_derivatives
function bezier.cubic_from_points_and_derivatives(d0, p0, p3, d3)
    local c1 = p0 + d0/3
    local c2 = p3 - d3/3
    return p0, c1, c2, p3
end

--- Get a cubic Bézier curve with given values at endpoints and starting
--- derivative, while minimizing (an approximation of) the stretch energy.
--
-- This function computes the 4 control points for the cubic curve B, such that
-- `B(0)=p0`, `B'(0)=d0`, `B(1)=p3`, and
-- the integral of `(B'(t))^2` on `t=[0,1]` is minimal.
-- (The actual stretch energy is the integral of `|B'(t)|`)
--
-- In practical terms this is almost the same as "the curve of shortest length
-- connecting given points and having the given starting speed".
--
-- @tparam number d0 The value of the derivative at t=0.
-- @tparam number p0 The value of the curve at t=0.
-- @tparam number p3 The value of the curve at t=1.
-- @treturn number c0
-- @treturn number c1
-- @treturn number c2
-- @treturn number c3
-- @staticfct gears.math.bezier.cubic_from_derivative_and_points_min_stretch
function bezier.cubic_from_derivative_and_points_min_stretch(d0, p0, p3)
    local c1 = p0 + d0/3
    local c2 = (2*p0 - c1 + 3*p3) / 4
    return p0, c1, c2, p3
end

--- Get a cubic Bézier curve with given values at endpoints and starting
--- derivative, while minimizing (an approximation of) the strain energy.
--
-- This function computes the 4 control points for the cubic curve B, such that
-- `B(0)=p0`, `B'(0)=d0`, `B(1)=p3`, and
-- the integral of `(B''(t))^2` on `t=[0,1]` is minimal.
--
-- In practical terms this is almost the same as "the curve of smallest
-- speed change connecting given points and having the given starting speed".
--
-- @tparam number d0 The value of the derivative at t=0.
-- @tparam number p0 The value of the curve at t=0.
-- @tparam number p3 The value of the curve at t=1.
-- @treturn number c0
-- @treturn number c1
-- @treturn number c2
-- @treturn number c3
-- @staticfct gears.math.bezier.cubic_from_derivative_and_points_min_strain
function bezier.cubic_from_derivative_and_points_min_strain(d, p0, p3)
    local c1 = p0 + d/3
    local c2 = (c1 + p3) / 2
    return p0, c1, c2, p3
end

--- Get a cubic Bézier curve with given values at endpoints and starting
--- derivative, while minimizing the jerk energy.
--
-- This function computes the 4 control points for the cubic curve B, such that
-- `B(0)=p0`, `B'(0)=d0`, `B(1)=p3`, and
-- the integral of `(B'''(t))^2` on `t=[0,1]` is minimal.
--
-- In practical terms this is almost the same as "the curve of smallest
-- acceleration change connecting given points and having the given
-- starting speed".
--
-- @tparam number d0 The value of the derivative at t=0.
-- @tparam number p0 The value of the curve at t=0.
-- @tparam number p3 The value of the curve at t=1.
-- @treturn number c0
-- @treturn number c1
-- @treturn number c2
-- @treturn number c3
-- @staticfct gears.math.bezier.cubic_from_derivative_and_points_min_jerk
function bezier.cubic_from_derivative_and_points_min_jerk(d, p0, p3)
    local c1 = p0 + d/3
    local c2 = c1 + (p3 - p0) / 3
    return p0, c1, c2, p3
end

return bezier

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
