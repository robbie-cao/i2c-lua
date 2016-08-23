/*
 * Copyright (c) 2016 Frank Edelhaeuser <mrpace2@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include <lua.h>
#include <lauxlib.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>


// definitions
#define I2CLUA_NAME             "i2c"
#define I2CLUA_VERSION          "1.0.0"
#define I2CLUA_COPYRIGHT        "Copyright (C) 2014 Frank Edelhaeuser <fedel@users.sourceforge.net>"
#define I2CLUA_LICENSE          "MIT License"
#define I2CLUA_TIMESTAMP        __DATE__" "__TIME__

#define I2CLUA_SUCCESS           0
#define I2CLUA_ERR_BUS          -1
#define I2CLUA_ERR_DEV          -2
#define I2CLUA_ERR_ALLOC        -3
#define I2CLUA_ERR_PARAM        -4

#define I2CLUA_OP_WR            1
#define I2CLUA_OP_RD            2



static int i2clua_impl(lua_State *L, int op, int bus, int address, const void *wr_data, size_t wr_length, size_t rd_length) {
    struct i2c_rdwr_ioctl_data ioctl_data;
    struct i2c_msg msgs[2] = {
        { .addr = address, .flags = 0, .buf = (void*) wr_data, .len = wr_length },
        { .addr = address, .flags = I2C_M_RD, .buf = 0, .len = rd_length },
    };
    char dev[20];
    int fd, rc;

    rc = I2CLUA_SUCCESS;

    // Allocate memory for read transfer
    if (rd_length > 0) {
        msgs[1].buf = malloc(rd_length);
        if (!msgs[1].buf)
            rc = I2CLUA_ERR_ALLOC;
    }

    // Execute I2C transfer
    if (rc == I2CLUA_SUCCESS) {
        snprintf(dev, sizeof(dev) - 1, "/dev/i2c-%d", bus);
        fd = open(dev, O_RDWR);
        if (fd < 0) {
            rc = I2CLUA_ERR_BUS;
        } else {
            if ((op & I2CLUA_OP_WR) && (op & I2CLUA_OP_RD)) {
                // Combined write-read transaction
                ioctl_data.nmsgs = 2;
                ioctl_data.msgs = msgs;
            } else {
                // Write-only or read-only transaction
                ioctl_data.nmsgs = 1;
                ioctl_data.msgs = op & I2CLUA_OP_WR ? &msgs[0] : &msgs[1];
            }
            if (ioctl(fd, I2C_RDWR, &ioctl_data) != ioctl_data.nmsgs) {
                rc = I2CLUA_ERR_DEV;
            }
            close(fd);
        }
    }

    // Return results to Lua
    lua_pushnumber(L, rc);
    if (op & I2CLUA_OP_RD) {
        if ((rc == I2CLUA_SUCCESS) && (rd_length > 0))
            lua_pushlstring(L, (char*)msgs[1].buf, rd_length);
        else if (rc == I2CLUA_SUCCESS)
            lua_pushstring(L, "");
        else
            lua_pushnil(L);
    }

    if (msgs[1].buf)
        free(msgs[1].buf);

    return op & I2CLUA_OP_RD ? 2 : 1;
}
 


/**
    \brief Combined write/read transfer to/from an I2C slave device.

        result, read_data = writeread(bus, address [, write_data [, read_length]])

    Performs a transfer on the I2C bus consisting of these parts: 

        START condition
        Send write address
        Send write data (optional, if length(write_data) > 0)
        REPEATED START condition
        Send read address
        Receive read data (optional, if read_length > 0)
        STOP condition

    \param  bus         (int) I2C bus number
    \param  address     (int) 8-bit I2C slave address
    \param  write_data  (string, optional = "") Write data
    \param  read_length (int, optional = 0) Positive number of bytes to read 

    \return result      (int) I2CLUA_SUCCESS on success, I2CLUA_ERR_* on error
    \return read_data   (string) read data
*/
static int i2clua_writeread(lua_State *L) {
    size_t write_length, read_length;
    int argc, bus, address;
    const char *write_data;

    argc = lua_gettop(L);
    if (argc < 2)
        return luaL_error(L, "Missing arguments. Usage: result, read_data = writeread(bus, address [, write_data [, read_length]])");

    bus = lua_tointeger(L, 1);
    address = lua_tointeger(L, 2);
    write_length = 0;
    write_data = argc < 3 ? 0 : lua_tolstring (L, 3, &write_length);
    read_length = argc < 4 ? 0 : (size_t) lua_tointeger(L, 4);

    return i2clua_impl(L, I2CLUA_OP_WR | I2CLUA_OP_RD, bus, address, write_data, write_length, read_length);
}



/**
    \brief Write to an I2C slave device.

        result = write(bus, address [, write_data])

    Performs a transfer on the I2C bus consisting of these parts: 

        START condition
        Send write address
        Send write data (optional, if length(write_data) > 0)
        STOP condition

    \param  bus         (int) I2C bus number
    \param  address     (int) 8-bit I2C slave address
    \param  write_data  (string) Write data

    \return result      (int) I2CLUA_SUCCESS on success, I2CLUA_ERR_* on error
*/
static int i2clua_write(lua_State *L) {
    int argc, bus, address;
    const char *write_data;
    size_t write_length;

    argc = lua_gettop(L);
    if (argc < 2)
        return luaL_error(L, "Missing arguments. Usage: result = write(bus, address [, write_data])");

    bus = lua_tointeger(L, 1);
    address = lua_tointeger(L, 2);
    write_length = 0;
    write_data = argc < 3 ? 0 : lua_tolstring (L, 3, &write_length);

    return i2clua_impl(L, I2CLUA_OP_WR, bus, address, write_data, write_length, 0);
}



/**
    \brief Read from an I2C slave device.

        result, read_data = read(bus, address [, read_length])

    Performs a transfer on the I2C bus consisting of these parts: 

        START condition
        Send read address
        Receive read data (optional, if read_length > 0)
        STOP condition

    \param  bus         (int) I2C bus number
    \param  address     (int) 8-bit I2C slave address
    \param  read_length (int, optional = 0) Positive number of bytes to read 

    \return result      (int) I2CLUA_SUCCESS on success, I2CLUA_ERR_* on error
    \return read_data   (string) read data
*/
static int i2clua_read(lua_State *L) {
    int argc, bus, address;
    size_t read_length;

    argc = lua_gettop(L);
    if (argc < 2)
        return luaL_error(L, "Missing arguments. Usage: result, read_data = read(bus, address [, read_length])");

    bus = lua_tointeger(L, 1);
    address = lua_tointeger(L, 2);
    read_length = argc < 3 ? 0 : (size_t) lua_tointeger(L, 3);

    return i2clua_impl(L, I2CLUA_OP_RD, bus, address, 0, 0, read_length);
}



/**
    \brief Return string describing I2C result

        errmsg = error(result [, raise = false])

    Translate I2C operation result into human readable string and optionally raises 
    a Lua error if result indicates an error.

    \param  result      (int) I2C result code (I2CLUA_SUCCESS or I2CLUA_ERR_*)
    \param  raise       (boolean, false) raise Lua error?

    \return errmsg      (string) human readable error message describing the result code
*/
static int i2clua_error(lua_State *L) {
    int argc, result, raise;
    const char* errmsg;

    argc = lua_gettop(L);
    if (argc < 1)
        return luaL_error(L, "Missing arguments. Usage: errmsg = error(result [, raise])");

    result = lua_tointeger(L, 1);
    raise = argc < 2 ? 0 : lua_toboolean(L, 2);

    switch (result) {
        case I2CLUA_SUCCESS: 
            lua_pushstring(L, "success"); 
            break;
        case I2CLUA_ERR_BUS: 
            lua_pushstring(L, "I2C bus error"); 
            break;
        case I2CLUA_ERR_DEV: 
            lua_pushstring(L, "I2C device error (no ACK)"); 
            break;
        case I2CLUA_ERR_ALLOC: 
            lua_pushstring(L, "I2C memory allocation failed"); 
            break;
        case I2CLUA_ERR_PARAM: 
            lua_pushstring(L, "I2C: invalid parameter"); 
            break;
        default: 
            lua_pushfstring(L, "I2C: invalid result code (%i)", result); 
            break;
    }

    if ((result != I2CLUA_SUCCESS) && (raise))
        return luaL_error(L, lua_tostring(L, -1));

    return 1;
}



LUALIB_API int luaopen_i2c(lua_State *L) {

    int i;
    const luaL_reg functions[] = {
        { "read", i2clua_read },
        { "write", i2clua_write },
        { "writeread", i2clua_writeread },
        { "error", i2clua_error },
        { NULL, NULL }
    };
    const struct {
        const char *name;
        int value;
    } const_number[] = {
        // return codes
        { "SUCCESS", I2CLUA_SUCCESS },
        { "ERR_BUS", I2CLUA_ERR_BUS },
        { "ERR_DEV", I2CLUA_ERR_DEV },
        { "ERR_ALLOC", I2CLUA_ERR_ALLOC },
        { "ERR_PARAM", I2CLUA_ERR_PARAM },
    };
    const struct {
        const char *name;
        const char *value;
    } const_string[] = {
        { "NAME", I2CLUA_NAME },
        { "COPYRIGHT", I2CLUA_COPYRIGHT },
        { "LICENSE", I2CLUA_LICENSE },
        { "VERSION", I2CLUA_VERSION },
        { "TIMESTAMP", I2CLUA_TIMESTAMP },
    };

    if (luaL_newmetatable(L, I2CLUA_NAME)) {

        // metatable.__index = metatable
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2);
        lua_settable(L, -3);    

        // Library functions
        luaL_openlib(L, I2CLUA_NAME, functions, 0);

        // Library constants (TBD: how to make them read-only?)
        for (i = 0; i < sizeof(const_number) / sizeof(const_number[0]); i++) {
            lua_pushnumber(L, const_number[i].value);
            lua_setfield(L, -2, const_number[i].name);
        }
        for (i = 0; i < sizeof(const_string) / sizeof(const_string[0]); i++) {
            lua_pushstring(L, const_string[i].value);
            lua_setfield(L, -2, const_string[i].name);
        }
    }

    return 0;
}
