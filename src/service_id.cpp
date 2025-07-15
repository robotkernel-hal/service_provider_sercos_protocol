//! robotkernel interface sercos protocol
/*!
 * author: Robert Burger <robert.burger@dlr.de>
 */

// vim: tabstop=4 softtabstop=4 shiftwidth=4 expandtab:

/*
 * This file is part of service_provider_sercos_protocol.
 *
 * service_provider_sercos_protocol is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * service_provider_sercos_protocol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with service_provider_sercos_protocol; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "service_id.h"

using namespace std;
using namespace robotkernel;
using namespace service_provider_sercos_protocol;

string service_id::data_to_string(const std::vector<uint16_t>& val) {
    if (val.size() == 0)
        return string("");

    switch (data.attr.datalength) {
        case SSA_DATALENGTH_2BYTEFIX:
            return _data_fix_to_string<int16_t, uint16_t, uint16_t>(&val[0], "%hd", "%hu", "%hu");
        case SSA_DATALENGTH_4BYTEFIX:
            return _data_fix_to_string<int32_t, uint32_t, uint32_t>((uint32_t *)&val[0], "%d", "%u", "%*lf");
        case SSA_DATALENGTH_8BYTEFIX:
            return _data_fix_to_string<int64_t, uint64_t, uint64_t>((uint64_t *)&val[0], "%lld", "%llu", "%*lf");
        case SSA_DATALENGTH_1BYTEVAR:
            if (data.attr.datatype == SSA_DATATYPE_CHARSET)
                return string("\"") + string((char *)&(val[2]), 
                        min((int)val[0], (int)strlen((char *)&(val[2])))) + string("\"");
                
            return string("[") + _data_var_to_string<int8_t, uint8_t, uint8_t>(val[0],
                        (uint8_t *)&val[2], "%hhd", "%hhu", "%hhu") + string("]");
        case SSA_DATALENGTH_2BYTEVAR:
            return string("[") + _data_var_to_string<int16_t, uint16_t, uint16_t>(val[0] / 2,
                    (uint16_t *)&val[2], "%hd", "%hu", "%hu") + string("]");
        case SSA_DATALENGTH_4BYTEVAR:
            return string("[") + _data_var_to_string<int32_t, uint32_t, float>(val[0] / 4,
                    (uint32_t *)&val[2], "%d", "%u", "%*lf") + string("]");
        case SSA_DATALENGTH_8BYTEVAR:
            return string("[") + _data_var_to_string<int64_t, uint64_t, double>(val[0] / 8,
                    (uint64_t *)&val[2], "%lld", "%llu", "%*lf") + string("]");
        default:
            break;
    }

    return string("");
}
        
//! converts python string to data value
void service_id::string_to_data(const string& buf, std::vector<uint16_t>& value) {
    switch (data.attr.datalength) {
        case SSA_DATALENGTH_2BYTEFIX: 
            value = _string_to_data_fix<int16_t, uint16_t, uint16_t>(buf);
            break;
        case SSA_DATALENGTH_4BYTEFIX:
            value = _string_to_data_fix<int32_t, uint32_t, float>(buf);
            break;
        case SSA_DATALENGTH_8BYTEFIX:
            value = _string_to_data_fix<int64_t, uint64_t, double>(buf);
            break;
        case SSA_DATALENGTH_1BYTEVAR:
            if (data.attr.datatype == SSA_DATATYPE_CHARSET) {
                size_t idlen = (buf.length()+4+1)/2;
                value.resize(idlen);
                uint16_t *val = &value[0];
                val[0] = buf.length();
                strncpy((char *)&val[2], buf.c_str(), buf.length());
                if (buf.length()%2) {
                    ((char *)val)[4+buf.length()] = '\0';
                    val[0]++;
                }
            }  else {
                value = _string_to_data_var<int8_t, uint8_t, uint8_t>(buf);
            }
            break;
        case SSA_DATALENGTH_2BYTEVAR:
            value = _string_to_data_var<int16_t, uint16_t, uint16_t>(buf);
            break;
        case SSA_DATALENGTH_4BYTEVAR:
            value = _string_to_data_var<int32_t, uint32_t, float>(buf);
            break;
        case SSA_DATALENGTH_8BYTEVAR:
            value = _string_to_data_var<int64_t, uint64_t, double>(buf);
            break;
        default:
            break;
    }
}

