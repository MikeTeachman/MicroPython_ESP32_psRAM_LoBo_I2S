/*
 * This file is part of the MicroPython ESP32 project
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Mike Teachman
 *  *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <string.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "driver/i2s.h"
enum {
    ARG_id,
    ARG_mode,
    ARG_samplerate,
    ARG_bits,
    ARG_channelformat,
    ARG_commformat,
    ARG_dmacount,
    ARG_dmalen,
    ARG_useapll,
    ARG_fixedmclk,
    ARG_sck,
    ARG_ws,
    ARG_sdout,
    ARG_sdin };
static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,                                MP_ARG_REQUIRED | MP_ARG_INT,   {.u_int = -1} },
        { MP_QSTR_mode,             MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT,   {.u_int = -1} },
        { MP_QSTR_samplerate,       MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT,   {.u_int = -1} },
        { MP_QSTR_bits,             MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT,   {.u_int = -1} },
        { MP_QSTR_channelformat,    MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT,   {.u_int = -1} },
        { MP_QSTR_commformat,       MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT,   {.u_int = -1} },
        { MP_QSTR_dmacount,         MP_ARG_KW_ONLY                   | MP_ARG_INT,   {.u_int = 16} },
        { MP_QSTR_dmalen,           MP_ARG_KW_ONLY                   | MP_ARG_INT,   {.u_int = 64} },
        { MP_QSTR_useapll,          MP_ARG_KW_ONLY                   | MP_ARG_BOOL,  {.u_bool = false} },
        { MP_QSTR_fixedmclk,        MP_ARG_KW_ONLY                   | MP_ARG_INT,   {.u_int = 0} },
        { MP_QSTR_sck,              MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ,   {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_ws,               MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ,   {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sdout,            MP_ARG_KW_ONLY                   | MP_ARG_OBJ,   {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sdin,             MP_ARG_KW_ONLY                   | MP_ARG_OBJ,   {.u_obj = MP_OBJ_NULL} },
};
typedef struct _machine_i2s_obj_t {
    mp_obj_base_t           base;
    i2s_port_t              id;
    uint8_t                 mode;
    int32_t                 samplerate;
    i2s_bits_per_sample_t   bits;
    i2s_channel_fmt_t       channelformat;
    i2s_comm_format_t       commformat;
    int16_t                 dmacount;
    int16_t                 dmalen;
    bool                    useapll;
    int32_t                 fixedmclk;
    int8_t                  sck;
    int8_t                  ws;
    int8_t                  sdout;
    int8_t                  sdin;
} machine_i2s_obj_t;
STATIC esp_err_t i2s_init_helper(machine_i2s_obj_t *i2s_obj) {
    esp_err_t err;
    i2s_config_t i2s_config;
    i2s_config.mode = i2s_obj->mode;
    i2s_config.sample_rate = i2s_obj->samplerate;
    i2s_config.bits_per_sample = i2s_obj->bits;
    i2s_config.channel_format = i2s_obj->channelformat;
    i2s_config.communication_format = i2s_obj->commformat;
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;  // TODO should this be configurable?
    i2s_config.dma_buf_count = i2s_obj->dmacount;
    i2s_config.dma_buf_len = i2s_obj->dmalen;
    i2s_config.use_apll = i2s_obj->useapll;
    i2s_config.fixed_mclk = i2s_obj->fixedmclk;
    i2s_pin_config_t pin_config;
    pin_config.bck_io_num = i2s_obj->sck;
    pin_config.ws_io_num = i2s_obj->ws;
    pin_config.data_out_num = I2S_PIN_NO_CHANGE; // TODO change pin assignment when Master write is implemented
    pin_config.data_in_num = i2s_obj->sdin;
    err = i2s_driver_install(i2s_obj->id, &i2s_config, 0, NULL);
    if (err == ESP_OK) {
        err = i2s_set_pin(i2s_obj->id, &pin_config);
    }
    return err;
}
/******************************************************************************/
// MicroPython bindings for I2S
STATIC void machine_i2s_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_i2s_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "I2S(id=%u, mode=%u, samplerate=%d, bits=%u,\n"
            "channelformat=%u, commformat=%u,\n"
            "dmacount=%d, dmalen=%d,\n"
            "useapll=%d, fixedmclk=%d,\n"
            "sck=%d, ws=%d, sdout=%d, sdin=%d)",
            self->id, self->mode, self->samplerate, self->bits,
            self->channelformat, self->commformat,
            self->dmacount, self->dmalen,
            self->useapll, self->fixedmclk,
            self->sck, self->ws, self->sdout, self->sdin);
}
mp_obj_t machine_i2s_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    // TODO add argument checking.  e.g.  datain needs to be defined for Rx Master mode
    // TODO: check pin args before calling gpio functions
    int8_t sck = machine_pin_get_gpio(args[ARG_sck].u_obj);
    int8_t ws = machine_pin_get_gpio(args[ARG_ws].u_obj);
    //sdout = machine_pin_get_gpio(args[ARG_sdout].u_obj);
    // ^^^^  TODO
    int8_t sdin = machine_pin_get_gpio(args[ARG_sdin].u_obj);
    // Create I2S object
    machine_i2s_obj_t *self = m_new_obj(machine_i2s_obj_t);
    self->base.type = &machine_i2s_type;
    self->id = args[ARG_id].u_int;
    self->mode = args[ARG_mode].u_int;
    self->samplerate = args[ARG_samplerate].u_int;
    self->bits = args[ARG_bits].u_int;
    self->channelformat = args[ARG_channelformat].u_int;
    self->commformat = args[ARG_commformat].u_int;
    self->dmacount = args[ARG_dmacount].u_int;
    self->dmalen = args[ARG_dmalen].u_int;
    self->useapll = args[ARG_useapll].u_bool;
    self->fixedmclk = args[ARG_fixedmclk].u_int;
    self->sck = sck;
    self->ws = ws;
    //self->sdout = sdout;  /// TODO
    self->sdin = sdin;
    if (i2s_init_helper(self) != ESP_OK) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Error installing I2S driver"));
    }
    return MP_OBJ_FROM_PTR(self);
}
STATIC mp_obj_t machine_i2s_init(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // TODO call the init helper function
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2s_init_obj, 1, machine_i2s_init);
STATIC mp_obj_t machine_i2s_readinto(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_i2s_obj_t *self = pos_args[0];
    STATIC const mp_arg_t allowed_args[] = {
            { MP_QSTR_buf,      MP_ARG_REQUIRED | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
            // TODO consider adding timeout argument, rather than hard-coded portMAX_DELAY
            // - look for ESP-IDF api call to convert ms or us to ticks ... hopefully it exists
    };
    // TODO handle error case of no buffer or not sized properly (multiple of 4 bytes needed for 32 bit memory?)
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), allowed_args, args);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_WRITE);
    uint32_t num_bytes_read = 0;
    if (i2s_read(self->id, bufinfo.buf, bufinfo.len, &num_bytes_read, portMAX_DELAY) != ESP_OK) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Error reading I2S bus"));
    }
    return mp_obj_new_int(num_bytes_read);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2s_readinto_obj, 2, machine_i2s_readinto);
STATIC mp_obj_t machine_i2s_deinit(mp_obj_t self_in) {
    machine_i2s_obj_t *self = self_in;
    // TODO only call uninstall if I2S is installed on this port
    i2s_driver_uninstall(self->id);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_i2s_deinit_obj, machine_i2s_deinit);
STATIC const mp_rom_map_elem_t machine_i2s_locals_dict_table[] = {
        // Methods
        { MP_ROM_QSTR(MP_QSTR_init),        (mp_obj_t)&machine_i2s_init_obj },
        { MP_ROM_QSTR(MP_QSTR_readinto),    (mp_obj_t)&machine_i2s_readinto_obj },
        { MP_ROM_QSTR(MP_QSTR_deinit),      (mp_obj_t)&machine_i2s_deinit_obj },
        // Constants
        { MP_ROM_QSTR(MP_QSTR_BPS8),            MP_ROM_INT(I2S_BITS_PER_SAMPLE_8BIT) },
        { MP_ROM_QSTR(MP_QSTR_BPS16),           MP_ROM_INT(I2S_BITS_PER_SAMPLE_16BIT) },
        { MP_ROM_QSTR(MP_QSTR_BPS24),           MP_ROM_INT(I2S_BITS_PER_SAMPLE_24BIT) },
        { MP_ROM_QSTR(MP_QSTR_BPS32),           MP_ROM_INT(I2S_BITS_PER_SAMPLE_32BIT) },
        { MP_ROM_QSTR(MP_QSTR_I2S),             MP_ROM_INT(I2S_COMM_FORMAT_I2S) },
        { MP_ROM_QSTR(MP_QSTR_I2S_MSB),         MP_ROM_INT(I2S_COMM_FORMAT_I2S_MSB) },
        { MP_ROM_QSTR(MP_QSTR_I2S_LSB),         MP_ROM_INT(I2S_COMM_FORMAT_I2S_LSB) },
        { MP_ROM_QSTR(MP_QSTR_RIGHT_LEFT),      MP_ROM_INT(I2S_CHANNEL_FMT_RIGHT_LEFT) },
        { MP_ROM_QSTR(MP_QSTR_ALL_RIGHT),       MP_ROM_INT(I2S_CHANNEL_FMT_ALL_RIGHT) },
        { MP_ROM_QSTR(MP_QSTR_ALL_LEFT),        MP_ROM_INT(I2S_CHANNEL_FMT_ALL_LEFT) },
        { MP_ROM_QSTR(MP_QSTR_ONLY_RIGHT),      MP_ROM_INT(I2S_CHANNEL_FMT_ONLY_RIGHT) },
        { MP_ROM_QSTR(MP_QSTR_ONLY_LEFT),       MP_ROM_INT(I2S_CHANNEL_FMT_ONLY_LEFT) },
        { MP_ROM_QSTR(MP_QSTR_NUM0),            MP_ROM_INT(I2S_NUM_0) },
        { MP_ROM_QSTR(MP_QSTR_NUM1),            MP_ROM_INT(I2S_NUM_1) },
        { MP_ROM_QSTR(MP_QSTR_MASTER),          MP_ROM_INT(I2S_MODE_MASTER) },
        { MP_ROM_QSTR(MP_QSTR_RX),              MP_ROM_INT(I2S_MODE_RX) },
};
MP_DEFINE_CONST_DICT(machine_i2s_locals_dict, machine_i2s_locals_dict_table);
const mp_obj_type_t machine_i2s_type = {
        { &mp_type_type },
        .name = MP_QSTR_I2S,
        .print = machine_i2s_print,
        .make_new = machine_i2s_make_new,
        .locals_dict = (mp_obj_dict_t *) &machine_i2s_locals_dict,
};
