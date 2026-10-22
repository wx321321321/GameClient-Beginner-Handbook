// Requires Lua 5.4 headers and library. See examples/course/README.md.
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <iostream>

static int AddAmmo(lua_State* L) {
    const lua_Integer ammo = luaL_checkinteger(L, 1);
    const lua_Integer pickup = luaL_checkinteger(L, 2);
    if (ammo < 0 || ammo > 30 || pickup < 0 || pickup > 1000)
        return luaL_error(L, "invalid ammo input");
    const lua_Integer total = ammo + pickup;
    lua_pushinteger(L, total > 30 ? 30 : total);
    return 1;
}

int main() {
    lua_State* L = luaL_newstate();
    if (!L) return 1;
    luaL_openlibs(L); // Trusted local lesson; not a sandbox for untrusted scripts.
    lua_register(L, "add_ammo", AddAmmo);
    const char* script =
        "assert(add_ammo(28, 5) == 30)\n"
        "local ok = pcall(add_ammo, 28, -1)\n"
        "assert(not ok)\n"
        "function damage_after_armor(damage, armor)\n"
        "  return math.max(0, damage - armor)\n"
        "end\n";
    if (luaL_dostring(L, script) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        std::cerr << (error ? error : "non-string Lua error") << '\n';
        lua_close(L);
        return 2;
    }
    const int base = lua_gettop(L);
    lua_getglobal(L, "damage_after_armor");
    lua_pushinteger(L, 30);
    lua_pushinteger(L, 8);
    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        std::cerr << (error ? error : "non-string Lua error") << '\n';
        lua_settop(L, base);
        lua_close(L);
        return 3;
    }
    int isInteger = 0;
    const lua_Integer damage = lua_tointegerx(L, -1, &isInteger);
    lua_settop(L, base);
    const bool passed = isInteger && damage == 22 && lua_gettop(L) == base;
    if (passed) std::cout << "lua damage=22 stack restored\n";
    lua_close(L);
    return passed ? 0 : 4;
}
