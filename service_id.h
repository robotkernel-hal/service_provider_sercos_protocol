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

#ifndef __MODULE_SERCOS_ID_H__
#define __MODULE_SERCOS_ID_H__

#include "robotkernel/kernel.h"
#include "robotkernel/module.h"
#include "module_intf.h"
#include <list>

namespace interface_sercos_protocol {

class service_id {
    public:
        struct {
            uint16_t structure;
            char name[65];
            size_t name_len;
            bool name_valid;
            char unit[16];
            size_t unit_len;
            bool unit_valid;
            sercos_service_attribute attr;
            bool attr_valid;
            uint16_t *min_val;
            size_t min_val_len;
            bool min_val_valid;
            uint16_t *max_val;
            size_t max_val_len;
            bool max_val_valid;
            uint16_t *val;
            size_t val_len;
        } data;

        std::string status;
    private:
        std::string mod_name;
        int slave_id;
        int idn;
        int elements;

        // read/write service id 
        int _read(const sercos_service_element element, uint16_t *buf, const size_t buflen);
        int _write(sercos_service_element element, uint16_t *buf, size_t buflen);
        
        // get id name
        void _update_name();

        // get id structure
        void _update_structure();

        // get unit
        void _update_unit();

        // get idn attribute
        void _update_attr();

        void _update_val(sercos_service_element element, uint16_t ** ans, size_t *len);

        template <typename T>
        void _val_to_buf(char buf[32], const char *fmt_flt, const char *fmt_dec, 
                int decimal, T *val);

        template<typename signed_T, typename unsigned_T, typename float_T>
        void _data_fix_to_string(char buf[32], unsigned_T *val, const char *fmt_signed,
                const char *fmt_unsigned, const char *fmt_float);

        template<typename signed_T, typename unsigned_T, typename float_T>
        void _data_var_to_string(char buf[2048], size_t size, unsigned_T *val, 
                const char *fmt_signed, const char *fmt_unsigned, const char *fmt_float);

        template <typename val_T, typename T>
        val_T _buf_to_val(int decimal, T *val);

        template<typename signed_T, typename unsigned_T, typename float_T, typename T>
        unsigned_T *_string_to_data_fix(T *val);

        template<typename signed_T, typename unsigned_T, typename float_T, typename T>
        unsigned_T *_string_to_data_var(std::list<T *>& lst);

    public:
        service_id(std::string mod_name, int slave_id, int idn, int elements);
        ~service_id();

        //! converts data value to python string
        std::string data_to_string(uint16_t *val);
        std::string min_val_to_string();
        std::string max_val_to_string();
        std::string val_to_string();
        
        //! converts python string to data value
        void string_to_data(const std::string buf, uint16_t **val, size_t *len);
        void string_to_min_val(const std::string buf);
        void string_to_max_val(const std::string buf);
        void string_to_val(const std::string buf);

        //! write elements to sercos joints
        void write_elements(int elements);
        
        void update_elements(int elements) {
            if (!data.name_valid && (elements & SSE_NAME)) {
                klog(robotkernel::interface_verbose, "   updating name\n");
                _update_name();
                data.name_valid = true;
            }
            if (!data.unit_valid && (elements & SSE_UNIT)) {
                klog(robotkernel::interface_verbose, "   updating unit\n");
                _update_unit();
                data.unit_valid = true;
            }
            if (!data.attr_valid && (elements & (SSE_ATTR | SSE_DATA | SSE_MAXVAL | SSE_MINVAL))) {
                klog(robotkernel::interface_verbose, "   updating attr\n");
                _update_attr();
                data.attr_valid = true;
            }
            if (!data.min_val_valid && (elements & SSE_MINVAL)) {
                klog(robotkernel::interface_verbose, "   updating min_val\n");
                _update_val(SSE_MINVAL, &data.min_val, &data.min_val_len);
                data.min_val_valid = true;
            }
            if (!data.max_val_valid && (elements & SSE_MAXVAL)) {
                klog(robotkernel::interface_verbose, "   updating max_val\n");
                _update_val(SSE_MAXVAL, &data.max_val, &data.max_val_len);
                data.max_val_valid = true;
            }
            if (elements & SSE_DATA) {
                klog(robotkernel::interface_verbose, "   updating value\n");
                _update_val(SSE_DATA, &data.val, &data.val_len);
            }
        }
};
        
template <typename T>
void service_id::_val_to_buf(char buf[32], const char *fmt_flt, const char *fmt_dec, 
        int decimal, T *val) {
    if (decimal) {
        double ddata = val[0];
        ddata /= pow(10., (double)decimal);
        snprintf(buf, 32, fmt_flt, decimal, ddata); 
    } else 
        snprintf(buf, 32, fmt_dec, val[0]); 
}

template<typename signed_T, typename unsigned_T, typename float_T>
void service_id::_data_fix_to_string(char buf[32], unsigned_T *val, const char *fmt_signed,
        const char *fmt_unsigned, const char *fmt_float) {
    switch (data.attr.datatype) { 
        case SSA_DATATYPE_INT:
            _val_to_buf<signed_T>(buf, "%.*lf", fmt_signed, 
                    data.attr.decimalpoint, (signed_T *)val);
            break; 
        case SSA_DATATYPE_BINARY: 
        case SSA_DATATYPE_UINT: 
        case SSA_DATATYPE_UINT2: 
        case SSA_DATATYPE_UINT3: 
            _val_to_buf<unsigned_T>(buf, "%.*lf", fmt_unsigned, 
                    data.attr.decimalpoint, (unsigned_T* )val);
            break; 
        case SSA_DATATYPE_FLOAT: 
            _val_to_buf<float_T>(buf, "%.*lf", fmt_float, 
                    data.attr.decimalpoint, (float_T *)val);
            break;
        default: 
            break;
    }
}

template<typename signed_T, typename unsigned_T, typename float_T>
void service_id::_data_var_to_string(char buf[2048], size_t size, unsigned_T *val, 
        const char *fmt_signed, const char *fmt_unsigned, const char *fmt_float) {
    std::string out = ""; 

    for (unsigned int i = 0; i < size; ++i) {
        char tmp[32];
        _data_fix_to_string<signed_T, unsigned_T, float_T>(tmp, &val[i], 
                fmt_signed, fmt_unsigned, fmt_float);

        out += std::string(tmp);
        if ((i + 1) < size)
            out += std::string(", ");
    }

    snprintf(buf, 2048, "%s", out.c_str());
}
        
template <typename val_T, typename T>
val_T service_id::_buf_to_val(int decimal, T *val) {
    if (decimal) {
        double ddata = (double)(*val);
        ddata *= pow(10., (double)decimal);
        return (val_T)ddata;
    } else 
        return (val_T)(*val);
}

template<typename signed_T, typename unsigned_T, typename float_T, typename T>
unsigned_T *service_id::_string_to_data_fix(T *val) {
    signed_T *svalue = new signed_T[1]();
    unsigned_T *uvalue = (unsigned_T *)svalue;
    float_T *fvalue = (float_T *)svalue;

    switch (data.attr.datatype) { 
        case SSA_DATATYPE_INT:
            *svalue = _buf_to_val<int, T>(data.attr.decimalpoint, val);
            break; 
        case SSA_DATATYPE_BINARY: 
        case SSA_DATATYPE_UINT: 
        case SSA_DATATYPE_UINT2: 
        case SSA_DATATYPE_UINT3: 
            *uvalue = _buf_to_val<unsigned int, T>(data.attr.decimalpoint, val);
            break; 
        case SSA_DATATYPE_FLOAT: 
            *fvalue = (float_T)_buf_to_val<double, T>(data.attr.decimalpoint, val);
            break;
        default: 
            break;
    }

    return uvalue;
}

template<typename signed_T, typename unsigned_T, typename float_T, typename T>
unsigned_T *service_id::_string_to_data_var(std::list<T *>& lst) {
    uint16_t *result = new uint16_t[2 + (sizeof(unsigned_T)/2 * lst.size())]();
    result[0] = sizeof(unsigned_T) * lst.size();
    signed_T *svalue = (signed_T *)&result[2];
    unsigned_T *uvalue = (unsigned_T *)&result[2];
    float_T *fvalue = (float_T *)&result[2];
    int idx = 0;

    for (typename std::list<T *>::const_iterator it = lst.begin(); it != lst.end(); ++it, ++idx) {
        T *elem = *it;

        switch (data.attr.datatype) { 
            case SSA_DATATYPE_INT:
                svalue[idx] = _buf_to_val<int, T>(data.attr.decimalpoint, elem);
                break; 
            case SSA_DATATYPE_BINARY: 
            case SSA_DATATYPE_UINT: 
            case SSA_DATATYPE_UINT2: 
            case SSA_DATATYPE_UINT3: 
                uvalue[idx] = _buf_to_val<unsigned int, T>(data.attr.decimalpoint, elem);
                break; 
            case SSA_DATATYPE_FLOAT: 
                fvalue[idx] = (float_T)_buf_to_val<double, T>(data.attr.decimalpoint, elem);
                break;
            default: 
                break;
        }
    }

    return (unsigned_T *)result;
}

inline std::string service_id::min_val_to_string() {
    if (data.min_val == NULL)
        return std::string("");

    return data_to_string(data.min_val);
}

inline std::string service_id::max_val_to_string() {
    return data_to_string(data.max_val);
}

inline std::string service_id::val_to_string() {
    return data_to_string(data.val);
}
        
inline void service_id::string_to_min_val(const std::string buf) {
    return string_to_data(buf, &data.min_val, &data.min_val_len);
}

inline void service_id::string_to_max_val(const std::string buf) {
    return string_to_data(buf, &data.max_val, &data.max_val_len);
}

inline void service_id::string_to_val(const std::string buf) {
    return string_to_data(buf, &data.val, &data.val_len);
}

}; // namespace robotkernel

#endif // __MODULE_SERCOS_ID_H__

