---------------------------------------------------------------------------
-- @author Kevin Zander
-- @copyright 2019 Kevin Zander
---------------------------------------------------------------------------

local color = require 'gears.color'
local colorful = require "beautiful.colorful"
local Color = colorful.color

local colors = {
    -- {
    --   hex str,
    --   hsl table,
    --   lighten 25%,
    --   darken 25%,
    --   invert,
    --   mix 25%,
    --   mix 75%,
    --   saturate 25%,
    --   desaturate 25%,
    --   grayscale,
    --   complement
    -- }
    -- For HSL we use the 0-100 range and convert to the 0-1 range
    --
    -- a few random colors
    {
        "#dbf7a6", -- originally #daf7a6, bumped red channel for floating precision error
        {81, 84, 81},
        "#ffffff",
        "#acec31",
        "#240859",
        "#52446c",
        "#adbb93",
        "#ddff9e",
        "#d7ebb2",
        "#cfcfcf",
        "#c2a5f7",
    },
    {
        "#944fb0",
        {283, 38, 50},
        "#caa7d8",
        "#4a2858",
        "#6bb04f",
        "#759867",
        "#8a6798",
        "#a12fd0",
        "#876f90",
        "#808080",
        "#6bb04f",
    },
    {
        "#ff5833", -- originally #ff5733, bumped green channel for floating precision error
        {11, 100, 60},
        "#ffc0b3",
        "#b32000",
        "#00a7cc",
        "#4093a6",
        "#bf6c59",
        "#ff5833",
        "#e6684c",
        "#999999",
        "#32d9ff",
    },
    {
        "#c70038",
        {343, 100, 39},
        "#ff477b", -- scss lighten returns #ff487b, rounding error
        "#480014",
        "#38ffc7",
        "#5cbfa3",
        "#a3405c",
        "#c70038",
        "#ae1943",
        "#646464",
        "#00c78f",
    },
    {
        "#920c3f", -- originally #900c3f, bumped red channel for floating precision error
        {337, 85, 31},
        "#ee3078",
        "#1c020c",
        "#6df3c0",
        "#76b9a0",
        "#89465f",
        "#9e003c",
        "#7e2044",
        "#4f4f4f",
        "#0c925f",
    },
    {
        "#581845",
        {318, 57, 22},
        "#bc3394",
        "#000000",
        "#a7e7ba",
        "#93b39d",
        "#6c4c62",
        "#660a4b",
        "#4a263f",
        "#383838",
        "#18582b",
    },
    -- red
    {
        "#ff0000",
        {0, 100, 50},
        "#ff8080",
        "#800000",
        "#00ffff",
        "#40bfbf",
        "#bf4040",
        "#ff0000",
        "#df2020",
        "#808080",
        "#00ffff",
    },
    -- cyan
    {
        "#00ffff",
        {180, 100, 50},
        "#80ffff",
        "#008080",
        "#ff0000",
        "#bf4040",
        "#40bfbf",
        "#00ffff",
        "#20dfdf",
        "#808080",
        "#ff0000",
    },
    -- white
    {
        "#ffffff",
        {0, 0, 100}, -- if saturation == 0 white/black/grey
        "#ffffff",
        "#bfbfbf",
        "#000000",
        "#404040",
        "#bfbfbf",
        "#ffffff",
        "#ffffff",
        "#ffffff",
        "#ffffff",
    },
    -- black
    {
        "#000000",
        {0, 0, 0}, -- if saturation == 0 white/black/grey
        "#404040",
        "#000000",
        "#ffffff",
        "#bfbfbf",
        "#404040",
        "#000000",
        "#000000",
        "#000000",
        "#000000",
    },
    -- grey
    {
        "#808080",
        {0, 0, 50}, -- if saturation == 0 white/black/grey
        "#c0c0c0",
        "#404040",
        "#7f7f7f",
        "#7f7f7f",
        "#808080",
        "#a06060",
        "#808080",
        "#808080",
        "#808080",
    },
}

local color_functional = {
    -- rgb(d, d, d)
    { "#bffea2ff", "rgb(191, 254, 162)" },
    { "#568482ff", "rgb(86, 132, 130)" },
    { "#4b3575ff", "rgb(75, 53, 117)" },
    { "#d328adff", "rgb(211, 40, 173)" },
    { "#e3ddfbff", "rgb(227, 221, 251)" },
    -- rgb(d, d, d, f)
    { "#bffea200", "rgb(191, 254, 162, 0)" },
    { "#568482ff", "rgb(86, 132, 130, 1)" },
    { "#4b357580", "rgb(75, 53, 117, 0.5)" },
    { "#d328ad54", "rgb(211, 40, 173, 0.33)" },
    { "#e3ddfba8", "rgb(227, 221, 251, 0.66)" },
    -- rgb(d, d, d, %)
    { "#83d69d4d", "rgb(131, 214, 157, 30%)" },
    { "#a3867efa", "rgb(163, 134, 126, 98%)" },
    { "#4bc587c4", "rgb(75, 197, 135, 77%)" },
    { "#67e4495c", "rgb(103, 228, 73, 36%)" },
    { "#ceeb12f2", "rgb(206, 235, 18, 95%)" },
    -- rgb(%, %, %)
    { "#4a5ccfff", "rgb(29%, 36%, 81%)" },
    { "#12f287ff", "rgb(7%, 95%, 53%)" },
    { "#33abe6ff", "rgb(20%, 67%, 90%)" },
    { "#120875ff", "rgb(7%, 3%, 46%)" },
    { "#3dfae8ff", "rgb(24%, 98%, 91%)" },
    -- rgb(%, %, %, f)
    { "#8a1745e0", "rgb(54%, 9%, 27%, 0.88)" },
    { "#b01a1f5c", "rgb(69%, 10%, 12%, 0.36)" },
    { "#9499ab4a", "rgb(58%, 60%, 67%, 0.29)" },
    { "#c74a5430", "rgb(78%, 29%, 33%, 0.19)" },
    { "#fc03d454", "rgb(99%, 1%, 83%, 0.33)" },
    -- rgb(%, %, %, %)
    { "#458a61c4", "rgb(27%, 54%, 38%, 77%)" },
    { "#85ab8a0a", "rgb(52%, 67%, 54%, 4%)" },
    { "#70f0f0ba", "rgb(44%, 94%, 94%, 73%)" },
    { "#4abda35c", "rgb(29%, 74%, 64%, 36%)" },
    { "#b02b73e3", "rgb(69%, 17%, 45%, 89%)" },
    -- rgb(d d d)
    { "#d765c8ff", "rgb(215 101 200)" },
    { "#e93356ff", "rgb(233 51 86)" },
    { "#478e7aff", "rgb(71 142 122)" },
    { "#5e83f3ff", "rgb(94 131 243)" },
    { "#a3b725ff", "rgb(163 183 37)" },
    -- rgb(d d d / f)
    { "#e93356c4", "rgb(233 51 86 / 0.77)" },
    { "#478e7aa1", "rgb(71 142 122 / 0.63)" },
    { "#5e83f3eb", "rgb(94 131 243 / 0.92)" },
    { "#2867221c", "rgb(40 103 34 / 0.11)" },
    { "#9d4ca385", "rgb(157 76 163 / 0.52)" },
    -- rgb(d d d / %)
    { "#053e23cf", "rgb(5 62 35 / 81%)" },
    { "#2867221c", "rgb(40 103 34 / 11%)" },
    { "#ff3883d6", "rgb(255 56 131 / 84%)" },
    { "#9d4ca387", "rgb(157 76 163 / 53%)" },
    { "#7ef94bc7", "rgb(126 249 75 / 78%)" },
    -- rgb(% % %)
    { "#d966c9ff", "rgb(85% 40% 79%)" },
    { "#eb3357ff", "rgb(92% 20% 34%)" },
    { "#478f7aff", "rgb(28% 56% 48%)" },
    { "#5e85f5ff", "rgb(37% 52% 96%)" },
    { "#a3b826ff", "rgb(64% 72% 15%)" },
    -- rgb(% % % / f)
    { "#054024cc", "rgb(2% 25% 14% / 0.80)" },
    { "#2969211c", "rgb(16% 41% 13% / 0.11)" },
    { "#ff3885d6", "rgb(100% 22% 52% / 0.84)" },
    { "#9e4da385", "rgb(62% 30% 64% / 0.52)" },
    { "#80fa4dc4", "rgb(50% 98% 30% / 0.77)" },
    -- rgb(% % % / %)
    { "#87c469e6", "rgb(53% 77% 41% / 90%)" },
    { "#4a5ccfeb", "rgb(29% 36% 81% / 92%)" },
    { "#12f28717", "rgb(7% 95% 53% / 9%)" },
    { "#33abe659", "rgb(20% 67% 90% / 35%)" },
    { "#12087512", "rgb(7% 3% 46% / 7%)" },
    -- rgba(d, d, d, f)
    { "#b02b71e0", "rgba(176, 43, 113, 0.88)" },
    { "#d4553be3", "rgba(212, 85, 59, 0.89)" },
    { "#5ab0f496", "rgba(90, 176, 244, 0.59)" },
    { "#a8db71eb", "rgba(168, 219, 113, 0.92)" },
    { "#66d0afe8", "rgba(102, 208, 175, 0.91)" },
    -- rgba(d, d, d, %)
    { "#ff35e321", "rgba(255, 53, 227, 13%)" },
    { "#ff0ede14", "rgba(255, 14, 222, 8%)" },
    { "#02ec9830", "rgba(2, 236, 152, 19%)" },
    { "#2a64e9d1", "rgba(42, 100, 233, 82%)" },
    { "#5c8d9475", "rgba(92, 141, 148, 46%)" },
    -- rgba(%, %, %, f)
    { "#3070f5eb", "rgb(19%, 44%, 96%, 0.92)" },
    { "#c4b321b0", "rgb(77%, 70%, 13%, 0.69)" },
    { "#63c7f2eb", "rgb(39%, 78%, 95%, 0.92)" },
    { "#de36cc8c", "rgb(87%, 21%, 80%, 0.55)" },
    { "#4de8e8de", "rgb(30%, 91%, 91%, 0.87)" },
    -- rgba(%, %, %, %)
    { "#b01a8ac2", "rgba(69%, 10%, 54%, 76%)" },
    { "#4fff94e0", "rgba(31%, 100%, 58%, 88%)" },
    { "#bfa10abf", "rgba(75%, 63%, 4%, 75%)" },
    { "#d6ede0d6", "rgba(84%, 93%, 88%, 84%)" },
    { "#fabfe8fc", "rgba(98%, 75%, 91%, 99%)" },
    -- rgba(d d d / f)
    { "#26e1a46e", "rgba(38 225 164 / 0.43)" },
    { "#9e48c94f", "rgba(158 72 201 / 0.31)" },
    { "#723a3047", "rgba(114 58 48 / 0.28)" },
    { "#8e6b2ce8", "rgba(142 107 44 / 0.91)" },
    { "#1b217fc2", "rgba(27 33 127 / 0.76)" },
    -- rgba(d d d / %)
    { "#488cb81f", "rgba(72 140 184 / 12%)" },
    { "#7998f175", "rgba(121 152 241 / 46%)" },
    { "#56d96f03", "rgba(86 217 111 / 1%)" },
    { "#5899d53d", "rgba(88 153 213 / 24%)" },
    { "#ad7c7b4f", "rgba(173 124 123 / 31%)" },
    -- rgba(% % % / f)
    { "#b830a10a", "rgba(72% 19% 63% / 0.04)" },
    { "#6bb3ada3", "rgba(42% 70% 68% / 0.64)" },
    { "#59309ca1", "rgba(35% 19% 61% / 0.63)" },
    { "#bd54bf33", "rgba(74% 33% 75% / 0.20)" },
    { "#edb0a842", "rgba(93% 69% 66% / 0.26)" },
    -- rgba(% % % / %)
    { "#ab802bd6", "rgba(67% 50% 17% / 84%)" },
    { "#e314a640", "rgba(89% 8% 65% / 25%)" },
    { "#a13bb552", "rgba(63% 23% 71% / 32%)" },
    { "#543d14a3", "rgba(33% 24% 8% / 64%)" },
    { "#3ba885fa", "rgba(23% 66% 52% / 98%)" },
    -- hsl(d, %, %)
    { "#124444ff", "hsl(180, 58%, 17%)" },
    { "#b43c9cff", "hsl(312, 50%, 47%)" },
    { "#223f3fff", "hsl(179, 30%, 19%)" },
    { "#988ea9ff", "hsl(262, 14%, 61%)" },
    { "#4f0c4dff", "hsl(302, 73%, 18%)" },
    -- hsl(d, %, %, f)
    { "#1f28285c", "hsl(180, 13%, 14%, 0.36)" },
    { "#d5fed29e", "hsl(117, 94%, 91%, 0.62)" },
    { "#e817e154", "hsl(302, 82%, 50%, 0.33)" },
    { "#5add8a42", "hsl(142, 66%, 61%, 0.26)" },
    { "#1e1e1aa6", "hsl(55, 8%, 11%, 0.65)" },
    -- hsl(d, %, %, %)
    { "#3e70471a", "hsl(131, 29%, 34%, 10%)" },
    { "#34f9a445", "hsl(154, 94%, 59%, 27%)" },
    { "#1d23dd29", "hsl(238, 77%, 49%, 16%)" },
    { "#d237a336", "hsl(318, 63%, 52%, 21%)" },
    { "#c1d9e666", "hsl(201, 43%, 83%, 40%)" },
    -- hsl(d % %)
    { "#bcd1a3ff", "hsl(88 33% 73%)" },
    { "#eedddeff", "hsl(355 34% 90%)" },
    { "#c9cac9ff", "hsl(148 1% 79%)" },
    { "#dce2daff", "hsl(106 12% 87%)" },
    { "#fefcfbff", "hsl(18 45% 99%)" },
    -- hsl(d % % / f)
    { "#e5eece17", "hsl(76 48% 87% / 0.09)" },
    { "#6e5530a8", "hsl(36 39% 31% / 0.66)" },
    { "#110d110d", "hsl(292 14% 6% / 0.05)" },
    { "#76efd170", "hsl(165 79% 70% / 0.44)" },
    { "#d3af5587", "hsl(43 59% 58% / 0.53)" },
    -- hsl(d % % / %)
    { "#324a6ce3", "hsl(215 37% 31% / 89%)" },
    { "#8baca887", "hsl(172 17% 61% / 53%)" },
    { "#182d62d4", "hsl(223 60% 24% / 83%)" },
    { "#efedeb2b", "hsl(26 10% 93% / 17%)" },
    { "#cadedc4d", "hsl(174 23% 83% / 30%)" },
    -- hsla(d, %, %, f)
    { "#0ba822cf", "hsla(129, 88%, 35%, 0.81)" },
    { "#404045c7", "hsla(238, 4%, 26%, 0.78)" },
    { "#0d2c9138", "hsla(226, 84%, 31%, 0.22)" },
    { "#262d0bab", "hsla(72, 62%, 11%, 0.67)" },
    { "#450f5c66", "hsla(282, 72%, 21%, 0.40)" },
    -- hsla(d, %, %, %)
    { "#28582263", "hsla(114, 44%, 24%, 39%)" },
    { "#7d9aa105", "hsla(192, 16%, 56%, 2%)" },
    { "#306e42bd", "hsla(137, 39%, 31%, 74%)" },
    { "#85d846eb", "hsla(94, 65%, 56%, 92%)" },
    { "#8375f052", "hsla(247, 81%, 70%, 32%)" },
    -- hsla(d % % / f)
    { "#89898bd6", "hsla(233 1% 54% / 0.84)" },
    { "#3562d466", "hsla(223 65% 52% / 0.40)" },
    { "#acf1b8ad", "hsla(131 72% 81% / 0.68)" },
    { "#131211b0", "hsla(56 4% 7% / 0.69)" },
    { "#d7e38791", "hsla(68 62% 71% / 0.57)" },
    -- hsla(d % % / %)
    { "#504e4e45", "hsla(1 1% 31% / 27%)" },
    { "#070c5a59", "hsla(236 86% 19% / 35%)" },
    { "#5f5cf552", "hsla(241 88% 66% / 32%)" },
    { "#31212c82", "hsla(319 19% 16% / 51%)" },
    { "#f7d8f833", "hsla(299 68% 91% / 20%)" },
}

describe("Colorful", function()

    describe("creates object with new", function()
        it("from #rrggbb string", function()
            for _,col in ipairs(colors) do
                local c = Color.new(col[1])
                -- we're going to verify against the original parser for things
                local pc = {color.parse_color(col[1])}
                assert.is.same(c.red, pc[1])
                assert.is.same(c.green, pc[2])
                assert.is.same(c.blue, pc[3])
                assert.is.same(c.alpha, pc[4])
            end
        end)

        it("from HSL table", function()
            for _,col in ipairs(colors) do
                -- we convert to [0,1]
                local h,s,l = col[2][1], col[2][2]/100, col[2][3]/100
                local c = Color.new({h,s,l})
                assert.is.same(col[1], c.as:hex())
            end
        end)

        it("from number", function()
            for _,col in ipairs(colors) do
                local c = Color.new(col[1])
                local dec = tonumber(col[1]:sub(2), 16)
                assert.is.same(c.as:dec(), dec)
            end
        end)

        it("from functional string", function()
            for _,col in ipairs(color_functional) do
                local c= Color.new(col[2])
                assert.is.same(col[1], c.as:hexa())
            end
        end)
    end)

    describe("adjusts color by", function()
        it("lightening 25%", function()
            for _,col in ipairs(colors) do
                local c = Color.new(col[1])
                c = c:lighten(0.25)
                assert.is.same(col[3], c.as:hex())
            end
        end)

        it("darkening 25%", function()
            for _,col in ipairs(colors) do
                local c = Color.new(col[1])
                c = c:darken(0.25)
                assert.is.same(col[4], c.as:hex())
            end
        end)

        it("inverting", function()
            for _,col in ipairs(colors) do
                local c = Color.new(col[1])
                c = c:invert()
                assert.is.same(col[5], c.as:hex())
            end
        end)

        it("mixing 0%", function()
            for _,col in ipairs(colors) do
                local c1 = Color.new(col[1])
                local c2 = Color.new(col[5])
                local mix = c1:mix(c2, 0)
                assert.is.same(col[5], mix.as:hex())
            end
        end)

        it("mixing 25%", function()
            for _,col in ipairs(colors) do
                local c1 = Color.new(col[1])
                local c2 = Color.new(col[5])
                local mix = c1:mix(c2, 0.25)
                assert.is.same(col[6], mix.as:hex())
            end
        end)

        it("mixing 50%", function()
            for _,col in ipairs(colors) do
                local c1 = Color.new(col[1])
                local c2 = Color.new(col[5])
                local mix = c1:mix(c2, 0.5)
                assert.is.same("#808080", mix.as:hex())
            end
        end)

        it("mixing 75%", function()
            for _,col in ipairs(colors) do
                local c1 = Color.new(col[1])
                local c2 = Color.new(col[5])
                local mix = c1:mix(c2, 0.75)
                assert.is.same(col[7], mix.as:hex())
            end
        end)

        it("mixing 100%", function()
            for _,col in ipairs(colors) do
                local c1 = Color.new(col[1])
                local c2 = Color.new(col[5])
                local mix = c1:mix(c2, 1)
                assert.is.same(col[1], mix.as:hex())
            end
        end)

        it("saturates 25%", function()
            for _,col in ipairs(colors) do
                local c = Color.new(col[1])
                c = c:saturate(0.25)
                assert.is.same(col[8], c.as:hex())
            end
        end)

        it("desaturates 25%", function()
            for _,col in ipairs(colors) do
                local c = Color.new(col[1])
                c = c:desaturate(0.25)
                assert.is.same(col[9], c.as:hex())
            end
        end)

        it("grayscales", function()
            for _,col in ipairs(colors) do
                local c = Color.new(col[1])
                c = c:grayscale()
                assert.is.same(col[10], c.as:hex())
            end
        end)
    end)
end)
