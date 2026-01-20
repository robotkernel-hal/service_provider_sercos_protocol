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

#ifndef SERVICE_PROVIDER_SERCOS_PROTOCOL__SERVICE_ID_H
#define SERVICE_PROVIDER_SERCOS_PROTOCOL__SERVICE_ID_H

#include "robotkernel/helpers.h"
#include "service_provider_sercos_protocol/base.h"

#include <list>
#include <math.h>
#include <string>

namespace service_provider_sercos_protocol {

class service_id {
    public:
        service_data_t data;

        template <typename T>
            std::string _val_to_buf(const char *fmt_flt, const char *fmt_dec, 
                    int decimal, const T *val);

        template<typename signed_T, typename unsigned_T, typename float_T>
            std::string _data_fix_to_string(const unsigned_T *val, const char *fmt_signed,
                    const char *fmt_unsigned, const char *fmt_float);

        template<typename signed_T, typename unsigned_T, typename float_T>
            std::string _data_var_to_string(size_t size, unsigned_T *val, 
                    const char *fmt_signed, const char *fmt_unsigned, const char *fmt_float);

        template <typename val_T>
            val_T _buf_to_val(int decimal, val_T val);

        template<typename signed_T, typename unsigned_T, typename float_T>
            std::vector<uint16_t> _string_to_data_fix(const std::string& val);

        template<typename signed_T, typename unsigned_T, typename float_T>
            std::vector<uint16_t> _string_to_data_var(const std::string& val);

    public:
        service_id() {};
        ~service_id() {};

        //! converts data value to python string
        std::string data_to_string(const std::vector<uint16_t>& val);
        std::string min_val_to_string();
        std::string max_val_to_string();
        std::string val_to_string();

        //! converts python string to data value
        void string_to_data(const std::string& buf, std::vector<uint16_t>& value);
        void string_to_min_val(const std::string buf);
        void string_to_max_val(const std::string buf);
        void string_to_val(const std::string buf);
};

template <typename T>
std::string service_id::_val_to_buf(const char *fmt_flt, const char *fmt_dec, 
        int decimal, const T *val) {
    if (decimal) {
        double ddata = val[0];
        ddata /= pow(10., (double)decimal);
        return robotkernel::helpers::string_printf(fmt_flt, decimal, ddata); 
    } 

    return robotkernel::helpers::string_printf(fmt_dec, val[0]); 
}

template<typename signed_T, typename unsigned_T, typename float_T>
std::string service_id::_data_fix_to_string(const unsigned_T *val, const char *fmt_signed,
        const char *fmt_unsigned, const char *fmt_float) {
    switch (data.attr.datatype) { 
        case SSA_DATATYPE_INT:
            return _val_to_buf<signed_T>("%.*lf", fmt_signed, 
                    data.attr.decimalpoint, (signed_T *)val);
            break; 
        case SSA_DATATYPE_BINARY: 
        case SSA_DATATYPE_UINT: 
        case SSA_DATATYPE_UINT2: 
        case SSA_DATATYPE_UINT3: 
            return _val_to_buf<unsigned_T>("%.*lf", fmt_unsigned, 
                    data.attr.decimalpoint, (unsigned_T* )val);
            break; 
        case SSA_DATATYPE_FLOAT: 
            return _val_to_buf<float_T>("%.*lf", fmt_float, 
                    data.attr.decimalpoint, (float_T *)val);
            break;
        default: 
            break;
    }

    return std::string("");
}

template<typename signed_T, typename unsigned_T, typename float_T>
std::string service_id::_data_var_to_string(size_t size, unsigned_T *val, 
        const char *fmt_signed, const char *fmt_unsigned, const char *fmt_float) {
    std::string out = ""; 

    for (unsigned int i = 0; i < size; ++i) {
        out += _data_fix_to_string<signed_T, unsigned_T, float_T>(&val[i], 
                fmt_signed, fmt_unsigned, fmt_float);

        if ((i + 1) < size)
            out += std::string(", ");
    }

    return out;
}

template <typename val_T>
val_T service_id::_buf_to_val(int decimal, val_T val) {
    if (decimal) {
        double ddata = (double)(val);
        ddata *= pow(10., (double)decimal);
        return (val_T)ddata;
    } else 
        return (val_T)(val);
}

template<typename signed_T, typename unsigned_T, typename float_T>
std::vector<uint16_t> service_id::_string_to_data_fix(const std::string& val) {
    std::vector<uint16_t> ans;

    switch (data.attr.datatype) { 
        case SSA_DATATYPE_INT: {
            int tmp_value;
            try { tmp_value = std::stoi(val, nullptr, 0); } catch (...) { break; }

            signed_T tmp = _buf_to_val<int>(data.attr.decimalpoint, tmp_value);
            ans.resize((sizeof(tmp) + 1) / 2);
            memcpy(&ans[0], &tmp, sizeof(tmp));
            break; 
        }
        case SSA_DATATYPE_BINARY: 
        case SSA_DATATYPE_UINT: 
        case SSA_DATATYPE_UINT2: 
        case SSA_DATATYPE_UINT3: {
            unsigned int tmp_value;
            try { tmp_value = std::stoul(val, nullptr, 0); } catch (...) { break; }

            unsigned_T tmp = _buf_to_val<unsigned int>(data.attr.decimalpoint, tmp_value);
            ans.resize((sizeof(tmp) + 1) / 2);
            memcpy(&ans[0], &tmp, sizeof(tmp));
            break; 
        }
        case SSA_DATATYPE_FLOAT: {
            double tmp_value;
            try { tmp_value = std::stod(val); } catch (...) { break; }

            float_T tmp = _buf_to_val<double>(data.attr.decimalpoint, tmp_value);
            ans.resize((sizeof(tmp) + 1) / 2);
            memcpy(&ans[0], &tmp, sizeof(tmp));
            break;
        }
        default: 
            break;
    }

    return ans;
}

template<typename signed_T, typename unsigned_T, typename float_T>
std::vector<uint16_t> service_id::_string_to_data_var(const std::string& val) {
    std::list<std::string> list_vals;
    std::string tmp_value = val;
    std::string::size_type a = tmp_value.find_first_not_of("[ ,]");

    while (a != std::string::npos) {
        tmp_value = tmp_value.substr(a);
        a = tmp_value.find_first_not_of("[ ,]");
        list_vals.push_back(tmp_value.substr(0, a));
    }

    size_t ans_len = 2 + (sizeof(unsigned_T)/2 * list_vals.size());
    std::vector<uint16_t> ans(ans_len);
    ans[0] = sizeof(unsigned_T) * list_vals.size();

    signed_T *svalue    = (signed_T *)  &ans[2];
    unsigned_T *uvalue  = (unsigned_T *)&ans[2];
    float_T *fvalue     = (float_T *)   &ans[2];
    int idx = 0;

    for (typename std::list<std::string>::const_iterator it = list_vals.begin(); it != list_vals.end(); ++it, ++idx) {
        const std::string& elem = *it;

        switch (data.attr.datatype) { 
            case SSA_DATATYPE_INT: {
                int tmp_value;
                try { tmp_value = std::stoi(elem, nullptr, 0); } catch (...) { break; }
                svalue[idx] = _buf_to_val<int>(data.attr.decimalpoint, tmp_value);
                break; 
            }
            case SSA_DATATYPE_BINARY: 
            case SSA_DATATYPE_UINT: 
            case SSA_DATATYPE_UINT2: 
            case SSA_DATATYPE_UINT3: {
                unsigned int tmp_value;
                try { tmp_value = std::stoul(elem, nullptr, 0); } catch (...) { break; }
                uvalue[idx] = _buf_to_val<unsigned int>(data.attr.decimalpoint, tmp_value);
                break; 
            }
            case SSA_DATATYPE_FLOAT: {
                double tmp_value;
                try { tmp_value = std::stod(elem); } catch (...) { break; }
                fvalue[idx] = (float_T)_buf_to_val<double>(data.attr.decimalpoint, tmp_value);
                break;
            }
            default: 
                break;
        }
    }

    return ans;
}

inline std::string service_id::min_val_to_string() {
    if (!data.min_value.size())
        return std::string("");

    return data_to_string(data.min_value);
}

inline std::string service_id::max_val_to_string() {
    if (!data.max_value.size())
        return std::string("");

    return data_to_string(data.max_value);
}

inline std::string service_id::val_to_string() {
    return data_to_string(data.value);
}

inline void service_id::string_to_min_val(const std::string buf) {
    return string_to_data(buf, data.min_value);
}

inline void service_id::string_to_max_val(const std::string buf) {
    return string_to_data(buf, data.max_value);
}

inline void service_id::string_to_val(const std::string buf) {
    return string_to_data(buf, data.value);
}

}; // namespace service_provider_sercos_protocol

#endif // SERVICE_PROVIDER_SERCOS_PROTOCOL__SERVICE_ID_H

