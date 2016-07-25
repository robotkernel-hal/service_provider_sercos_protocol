//! robotkernel interface sercos protocol
/*!
 * author: Robert Burger
 *
 * $Id$
 */

// vim: tabstop=4 softtabstop=4 shiftwidth=4 expandtab:

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with robotkernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "service_id.h"
#include "robotkernel/module.h"

#include <string_util/string_util.h>

using namespace std;
using namespace robotkernel;
using namespace interface_sercos_protocol;

int service_id::_read(const sercos_service_element element, uint16_t *buf, const size_t buflen) {
    sercos_service_transfer sst;
    sst.slave_id = slave_id;
    sst.idn = idn;
    sst.element = element;
    sst.buflen = buflen;
    sst.buf = buf;
    sst.direction = SSD_DRIVE_TO_MASTER;

    return kernel::request_cb(mod_name.c_str(), MOD_REQUEST_SERCOS_SERVICE_TRANSFER, &(sst));
}

int service_id::_write(sercos_service_element element, uint16_t *buf, size_t buflen) {
    sercos_service_transfer sst;
    sst.slave_id = slave_id;
    sst.idn = idn;
    sst.element = element;
    sst.buflen = buflen;
    sst.buf = buf;
    sst.direction = SSD_MASTER_TO_DRIVE;

    return kernel::request_cb(mod_name.c_str(), MOD_REQUEST_SERCOS_SERVICE_TRANSFER, &(sst));
}

// get id name
int service_id::_update_name() {
    int ret;
    uint16_t size[2];
    ret = _read(SSE_NAME, &size[0], 2);
    if (ret) {
        status = format_string("reading id name size failed. "
                "service transfer error code %d", ret);
        return ret;
    }

    ret = _read(SSE_NAME, (uint16_t *)&data.name[0], (size[0] + 4) / 2);
    if (ret) {
        status = format_string("reading id name failed. "
                "service transfer error code %d", ret);
    }

    data.name[size[0] + 4] = '\0';

    return ret;
}

// get id structure
int service_id::_update_structure() {
    int ret;
    ret = _read(SSE_STRC, (uint16_t *)&data.structure, 1);
    if (ret)
        status = format_string("reading id structure failed. "
                "service transfer error code %d", ret);

    return ret;
}

// get unit
int service_id::_update_unit() {
    int ret;
    uint16_t size[2];
    ret = _read(SSE_UNIT, &size[0], 2);
    if (ret) {
        status = format_string("reading id unit size failed. "
                "service transfer error code %d", ret);
        return ret;
    }

    if (size[0] <= 12) {
        ret = _read(SSE_UNIT, (uint16_t *)&data.unit[0], (size[0] + 4) / 2);
        if (ret)
            status = format_string("reading id unit failed. "
                    "service transfer error code %d", ret);
    }

    return ret;
}

// get idn attribute
int service_id::_update_attr() {
    int ret;
    ret = _read(SSE_ATTR, (uint16_t *)&data.attr, sizeof(data.attr) / 2);
    if (ret)
        status = format_string("reading id attr failed. "
                "service transfer error code %d", ret);

    return ret;
}

int service_id::_update_val(sercos_service_element element, uint16_t ** ans, size_t *len) {
    int ret = 0;
    if (!data.attr_valid)
        return ret;

    switch (data.attr.datalength) {
        case SSA_DATALENGTH_2BYTEFIX:
            if (*ans) {
                delete[] *ans;
                *ans = NULL;
            }

            *ans = new uint16_t[1]();
            *len = 2;
            ret = _read(element, *ans, 1);
            if (ret)
                status = format_string("reading id value failed. "
                        "service transfer error code %d", ret);
            break;
        case SSA_DATALENGTH_4BYTEFIX:
            if (*ans) {
                delete[] *ans;
                *ans = NULL;
            }

            *ans = new uint16_t[2]();
            *len = 4;
            ret = _read(element, *ans, 2);
            if (ret)
                status = format_string("reading id value failed. "
                        "service transfer error code %d", ret);
            break;
        case SSA_DATALENGTH_8BYTEFIX:
            if (*ans) {
                delete[] *ans;
                *ans = NULL;
            }

            *ans = new uint16_t[4]();
            *len = 8;
            ret = _read(element, *ans, 4);
            if (ret)
                status = format_string("reading id value failed. "
                        "service transfer error code %d", ret);
            break;
        case SSA_DATALENGTH_1BYTEVAR:
        case SSA_DATALENGTH_2BYTEVAR:
        case SSA_DATALENGTH_4BYTEVAR:
        case SSA_DATALENGTH_8BYTEVAR: {
            uint16_t size[2];
            ret = _read(element, &size[0], 2);
            if (ret) {
                status = format_string("reading id value size failed. "
                        "service transfer error code %d", ret);
                break;
            }

            if (*ans) {
                delete[] *ans;
                *ans = NULL;
            }

            *ans = new uint16_t[(size[0] + 4 + 1) / 2]();
            *len = size[0] + 4;
            memset(*ans, 0, *len);
            ret = _read(element, *ans, (size[0] + 4) / 2);
            if (ret)
                status = format_string("reading id value failed. "
                        "service transfer error code %d", ret);
            break;
        }
        default:
            status = format_string("switch does not match any case %d\n", 
                    data.attr.datalength);
            break;
    }

    return ret;
}

service_id::service_id(string mod_name, int slave_id, int idn, int elements) 
    : mod_name(mod_name), slave_id(slave_id), idn(idn), elements(elements) {
    memset(&data, 0, sizeof(data));

    data.min_val = NULL;
    data.max_val = NULL;
    data.val = NULL;

    data.name_valid = data.unit_valid = data.attr_valid =
        data.min_val_valid = data.max_val_valid = false;

    update_elements(elements);
}

service_id::~service_id() {
    if (data.val)
        delete[] data.val;
    if (data.min_val)
        delete[] data.min_val;
    if (data.max_val)
        delete[] data.max_val;
}


string service_id::data_to_string(uint16_t *val) {
    char buf_fixed[32], buf_var[2048]; 
    buf_fixed[0] = '\0';
    buf_var[0] = '\0';
    string ans = "";

    switch (data.attr.datalength) {
        case SSA_DATALENGTH_2BYTEFIX:
            _data_fix_to_string<int16_t, uint16_t, uint16_t>(
                    buf_fixed, val, "%hd", "%hu", "%hu");
            ans = string(buf_fixed);
            break;
        case SSA_DATALENGTH_4BYTEFIX:
            _data_fix_to_string<int32_t, uint32_t, uint32_t>(
                    buf_fixed, (uint32_t *)val, "%d", "%u", "%*lf");
            ans = string(buf_fixed);
            break;
        case SSA_DATALENGTH_8BYTEFIX:
            _data_fix_to_string<int64_t, uint64_t, uint64_t>(
                    buf_fixed, (uint64_t *)val, "%lld", "%llu", "%*lf");
            ans = string(buf_fixed);
            break;
        case SSA_DATALENGTH_1BYTEVAR:
            if (data.attr.datatype == SSA_DATATYPE_CHARSET)
                ans = string("\"") + string((char *)&(val[2]), 
                        min((int)val[0], (int)strlen((char *)&(val[2])))) + string("\"");
            else {
                _data_var_to_string<int8_t, uint8_t, uint8_t>(buf_var, val[0],
                        (uint8_t *)&(val[2]), "%hhd", "%hhu", "%hhu");
                ans = string("[") + string(buf_var) + string("]");
            }
            break;
        case SSA_DATALENGTH_2BYTEVAR:
            _data_var_to_string<int16_t, uint16_t, uint16_t>(buf_var, val[0] / 2,
                    (uint16_t *)&(val[2]), "%hd", "%hu", "%hu");
            ans = string("[") + string(buf_var) + string("]");
            break;
        case SSA_DATALENGTH_4BYTEVAR:
            _data_var_to_string<int32_t, uint32_t, float>(buf_var, val[0] / 4,
                    (uint32_t *)&(val[2]), "%d", "%u", "%*lf");
            ans = string("[") + string(buf_var) + string("]");
            break;
        case SSA_DATALENGTH_8BYTEVAR:
            _data_var_to_string<int64_t, uint64_t, double>(buf_var, val[0] / 8,
                    (uint64_t *)&(val[2]), "%lld", "%llu", "%*lf");
            ans = string("[") + string(buf_var) + string("]");
            break;
        default:
            break;
    }

    return ans;
}
        
//! converts python string to data value
void service_id::string_to_data(string buf, uint16_t **val, size_t *len) {
    py_value *pval = eval_full(buf);
    py_list *pval_list = dynamic_cast<py_list *>(pval);
    
    switch (data.attr.datalength) {
        case SSA_DATALENGTH_2BYTEFIX:
            *val = _string_to_data_fix<int16_t, uint16_t, uint16_t, py_value>(pval);
            *len = 2;
            break;
        case SSA_DATALENGTH_4BYTEFIX:
            *(uint32_t **)val = _string_to_data_fix<int32_t, uint32_t, float, py_value>(pval);
            *len = 4;
            break;
        case SSA_DATALENGTH_8BYTEFIX:
            *(uint64_t **)val = _string_to_data_fix<int64_t, uint64_t, double, py_value>(pval);
            *len = 8;
            break;
        case SSA_DATALENGTH_1BYTEVAR:
            if (data.attr.datatype == SSA_DATATYPE_CHARSET) {
                *val = new uint16_t[(buf.length()+4+1)/2]();
                *len = buf.length() + 4;
                (*val)[0] = buf.length();
                strncpy((char *)&(*val)[2], buf.c_str(), buf.length());
                if (buf.length()%2) {
                    ((char *)(*val))[4+buf.length()] = '\0';
                    (*val)[0]++;
                    (*len)++;
                }
            }  else {
                if (pval_list) {
                    *(uint8_t **)val = _string_to_data_var<int8_t, uint8_t, uint8_t, py_value>(pval_list->value);
                    *len = (*(uint16_t **)val)[0] + 4;
                    break;
                }
            }
            break;
        case SSA_DATALENGTH_2BYTEVAR:
            if (pval_list) {
                *val = _string_to_data_var<int16_t, uint16_t, uint16_t, py_value>(pval_list->value);
                *len = (*(uint16_t **)val)[0] + 4;
                break;
            }
        case SSA_DATALENGTH_4BYTEVAR:
            if (pval_list) {
                *(uint32_t **)val = _string_to_data_var<int32_t, uint32_t, float, py_value>(pval_list->value);
                *len = (*(uint16_t **)val)[0] + 4;
                break;
            }
            break;
        case SSA_DATALENGTH_8BYTEVAR:
            if (pval_list) {
                *(uint64_t **)val = _string_to_data_var<int64_t, uint64_t, double, py_value>(pval_list->value);
                *len = (*(uint16_t **)val)[0] + 4;
                break;
            }
            break;
        default:
            break;
    }

    delete pval;
}

//! write elements to sercos joints
void service_id::write_elements(int elements) {
    if (elements & SSE_STRC)
        _write(SSE_STRC, (uint16_t *)&data.structure, (sizeof(data.structure)+1)/2);
    if (elements & SSE_NAME)
        _write(SSE_NAME, (uint16_t *)data.name, (data.name_len+1)/2);
    if (elements & SSE_UNIT) 
        _write(SSE_UNIT, (uint16_t *)data.unit, (data.unit_len+1)/2);
    if (elements & SSE_ATTR)
        _write(SSE_ATTR, (uint16_t *)&data.attr, (sizeof(data.attr)+1)/2);
    if (elements & SSE_MINVAL)
        _write(SSE_MINVAL, data.min_val, (data.min_val_len+1)/2);
    if (elements & SSE_MAXVAL)
        _write(SSE_MAXVAL, data.max_val, (data.max_val_len+1)/2);
    if (elements & SSE_DATA)
        _write(SSE_DATA, data.val, (data.val_len+1)/2);
}

