/* As of Feb 26, 2020, xkb_utf32_to_keysym.c exists only in development code
 * for xkbcommon/libxkbcommon. This source code is to adapt
 * https://github.com/xkbcommon/libxkbcommon/commit/0345aba082c83e9950f9dd8b7ea3bf91fe566a02
 * which Jaroslaw Kubik authored and @bluetech committed.
 * */

static xkb_keysym_t xkb_utf32_to_keysym(uint32_t ucs);
