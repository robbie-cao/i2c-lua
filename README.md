I2C Lua Binding
===============

> *Author: Frank Edelhaeuser*

This is the I2C Lua binding. This binding provides access from Lua scripts to I2C slave devices on I2C busses supported by the Linux kernel.


#### Warning ####

This program can confuse your I2C bus, cause data loss and worse!


#### Compile and Install ####

If available, you may install a precompiled package for your platform. Otherwise, compile the Lua I2C extension library for your platform:

    $ cd /path/to/i2c-lua
    $ make

Depending on your setup, you may need additional prerequisite packages for sucessfully compiling this binding. `i2c-lua` requires GCC, I2C headers and Lua headers. If `make` fails, inspect the `make` output for any error messages. You can specify additional compiler options with the `make` command:

    $ make CFLAGS=-I/usr/include/lua5.1

When `make` succeeds, it generates a shared library ``i2c.so``. Place this file into a location where Lua searches for extension modules (see http://www.lua.org/manual/5.1/manual.html#pdf-require). You may run:

    $ lua -l i2c

to display a list of directories where Lua searches for extension modules.

*Note:* This binding will not compile or work on Windows because there is no OS support for I2C in Windows.


#### Code Example ####

    local i2c = require("i2c")

    -- read 32 bytes from I2C device at bus 0, address 0x50
    result, data = i2c.read(0, 0x50, 32)
    if (result ~= i2c.SUCCESS) then
        i2c.error(result, true)
    end

    -- write bytes 0x00, 0x01, 0x02 to I2C device at bus 0, address 0x50
    result = i2c.write(0, 0x50, '\0\1\2')
    if (result ~= i2c.SUCCESS) then
        i2c.error(result, true)
    end

    -- write byte 0x12 to I2C device at bus 0, address 0x50 followed by
    -- repeated start and read 8 bytes
    result, data = i2c.writeread(0, 0x50, '\18', 8)
    if (result ~= i2c.SUCCESS) then
        i2c.error(result, true)
    end

See the API Reference below for more details on the services provided by the I2C Lua binding.



#### API Reference ####


This reference assumes that the I2C Lua binding was instantiated using this statement:

    local i2c = require("i2c")


##### Read from I2C slave #####

    result, read-data = i2c.read(bus, address [, read-length])

Performs a read transfer on the I2C bus consisting of these parts: 

  * START condition
  * Send read address
  * Receive read data (optional, if read-length > 0)
  * STOP condition

Parameters:

  * *bus*: I2C bus number (integer, required)
  * *address*: 8-bit I2C slave address (integer, required)
  * *read-length*: Positive number of bytes to read (integer, optional, default = 0)

Return Values:

  * *result*: i2c.SUCCESS on success, i2c.ERR_* on error (integer)
  * *read-data*: data read from I2C slave (string)


##### Write to I2C slave #####

    result = i2c.write(bus, address [, write-data])

Performs a write transfer on the I2C bus consisting of these parts: 

  * START condition
  * Send write address
  * Send write data (optional, if length of write-data > 0)
  * STOP condition

Parameters:

  * *bus*: I2C bus number (integer, required)
  * *address*: 8-bit I2C slave address (integer, required)
  * *write-data*: Write data (string, optional, default = "")

Return Values:

  * *result*: i2c.SUCCESS on success, i2c.ERR_* on error (integer)


##### Write to I2C slave followed by read #####

    result, read-data = i2c.writeread(bus, address [, write-data [, read-length]])

Performs a write-read transfer on the I2C bus consisting of these parts: 

  * START condition
  * Send write address
  * Send write data (optional, if length of write-data > 0)
  * REPEATED START condition
  * Send read address
  * Receive read data (optional, if read-length > 0)
  * STOP condition

Parameters:

  * *bus*: I2C bus number (integer, required)
  * *address*: 8-bit I2C slave address (integer, required)
  * *write-data*: Write data (string, optional, default = "")
  * *read-length*: Positive number of bytes to read (integer, optional, default = 0)

Return Values:

  * *result*: i2c.SUCCESS on success, i2c.ERR_* on error (integer)
  * *read-data*: data read from I2C slave (string)


##### Translate I2C result code into string, optionally raise error #####

    errmsg = i2c.error(result [, raise])

Translates I2C operation result into human readable string and raises a Lua error if *raise* evaluates to `true` and *result* indicates an error.

Parameters:

  * *result*: I2C result code (i2c.SUCCESS or i2c.ERR_*) (integer, required)
  * *raise*: raise Lua error (boolean, optional, default = false)

Return Values:

  * *errmsg*: human readable error message describing the result code (string)


#### Support and Related Information ####

Support for the I2C Lua Binding is provided using the Sourceforge ticket system: http://sourceforge.net/p/i2c-lua/tickets. Please DO NOT approach the author with support requests by email. Thank you!

  * http://www.lua.org/manual/
  * http://www.lm-sensors.org/wiki/i2cToolsDocumentation
