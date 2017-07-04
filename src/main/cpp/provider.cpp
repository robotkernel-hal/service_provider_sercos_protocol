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

#include "provider.h"
#include "service_id.h"

#include "robotkernel/exceptions.h"
#include "robotkernel/kernel.h"

SERVICE_PROVIDER_DEF(sercos_protocol, service_provider::sercos_protocol::provider);

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;
using namespace service_provider;
using namespace string_util;

#if defined __VXWORKS__ || defined __QNX__
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define strnlen(a, b) \
    min(strlen((a)), (b))
#endif

//! handler construction
sercos_protocol::handler::handler(const robotkernel::sp_service_interface_t& req) 
    : log_base("sercos_protocol", req->owner + "." + req->device_name + ".sercos_protocol") {
    robotkernel::kernel& k = *robotkernel::kernel::get_instance();

    _instance = std::dynamic_pointer_cast<service_provider::sercos_protocol::base>(req);
    if (!_instance)
        throw str_exception("wrong base class");

    std::stringstream base;
    base << _instance->device_name << ".sercos_protocol.";

    k.add_service(_instance->owner, base.str() + "read_id", 
            service_definition_read_id,
            std::bind(&sercos_protocol::handler::service_read_id, this, _1, _2));
    k.add_service(_instance->owner, base.str() + "write_id", 
            service_definition_write_id,
            std::bind(&sercos_protocol::handler::service_write_id, this, _1, _2));
}

//! handler destruction
sercos_protocol::handler::~handler() {
    kernel& k = *kernel::get_instance();

    stringstream base;
    base << _instance->device_name << ".sercos_protocol.";
    k.remove_service(_instance->owner, base.str() + "read_id");
    k.remove_service(_instance->owner, base.str() + "write_id");
};

//! service callback request read id
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int sercos_protocol::handler::service_read_id(const robotkernel::service_arglist_t& request, 
        robotkernel::service_arglist_t& response) {
#define READ_ID_REQ_IDN			0
#define READ_ID_REQ_ELEMENTS	1
    uint16_t idn     = request[READ_ID_REQ_IDN];
    uint8_t elements = request[READ_ID_REQ_ELEMENTS];

    // sercos service transfer data
    service_id id;

    // default response values 
    string error_message = "", value = "", min_value = "N/A", max_value = "N/A";
    uint32_t attr = 0;

    if (    (elements & SSE_MINVAL) ||
            (elements & SSE_MAXVAL) ||
            (elements & SSE_DATA)) 
        elements |= SSE_ATTR;   // we have to read attribute to interpret data

    try {
        _instance->sercos_read_idn(idn, elements, id.data);
    
        if (elements & SSE_ATTR) 
            attr = *(uint32_t *)&id.data.attr;

        if (    (elements & SSE_MINVAL) &&
                (id.data.attr.datatype != SSA_DATATYPE_CHARSET))
            min_value = id.min_val_to_string();

        if (    (elements & SSE_MAXVAL) &&
                (id.data.attr.datatype == SSA_DATATYPE_CHARSET))
            max_value = id.max_val_to_string();

        if (elements & SSE_DATA)
            value = id.val_to_string();
    } catch (std::exception& e) {
        error_message = e.what();
    }

#define READ_ID_RESP_STRUCTURE		0
#define READ_ID_RESP_NAME			1
#define READ_ID_RESP_UNIT			2
#define READ_ID_RESP_ATTR			3
#define READ_ID_RESP_MIN_VALUE		4
#define READ_ID_RESP_MAX_VALUE		5
#define READ_ID_RESP_VALUE			6
#define READ_ID_RESP_ERROR_MESSAGE	7
    response.resize(8);
    response[READ_ID_RESP_STRUCTURE]     = id.data.structure;
    response[READ_ID_RESP_NAME]          = id.data.name;
    response[READ_ID_RESP_UNIT]          = id.data.unit;
    response[READ_ID_RESP_ATTR]          = attr;
    response[READ_ID_RESP_MIN_VALUE]     = min_value;
    response[READ_ID_RESP_MAX_VALUE]     = max_value;
    response[READ_ID_RESP_VALUE]         = value;
    response[READ_ID_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}

const std::string sercos_protocol::handler::service_definition_read_id =
"request:\n"
"   uint16_t: idn\n"
"   uint8_t: elements\n"
"response:\n"
"   uint16_t: structure\n"
"   string: name\n"
"   string: unit\n"
"   uint32_t: attr\n"
"   string: min_value\n"
"   string: max_value\n"
"   string: value\n"
"   string: error_message\n";

//! service callback request write id
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int sercos_protocol::handler::service_write_id(const robotkernel::service_arglist_t& request, 
        robotkernel::service_arglist_t& response) {
#define WRITE_ID_REQ_IDN		0
#define WRITE_ID_REQ_ELEMENTS	1
#define WRITE_ID_REQ_STRUCTURE	2
#define WRITE_ID_REQ_NAME		3
#define WRITE_ID_REQ_UNIT		4
#define WRITE_ID_REQ_ATTR		5
#define WRITE_ID_REQ_MIN_VALUE	6
#define WRITE_ID_REQ_MAX_VALUE  7
#define WRITE_ID_REQ_VALUE      8
    uint16_t idn        = request[WRITE_ID_REQ_IDN];
    uint8_t elements    = request[WRITE_ID_REQ_ELEMENTS];
    uint16_t structure  = request[WRITE_ID_REQ_STRUCTURE];
    string name         = request[WRITE_ID_REQ_NAME];
    string unit         = request[WRITE_ID_REQ_UNIT];
    uint32_t attr       = request[WRITE_ID_REQ_ATTR];
    string min_value    = request[WRITE_ID_REQ_MIN_VALUE];
    string max_value    = request[WRITE_ID_REQ_MAX_VALUE];
    string value        = request[WRITE_ID_REQ_VALUE];

    // response 
    string error_message = "";

    service_id id;    

    if (elements & SSE_NAME) {
        // TODO
    }

    // read structure
    if (elements & SSE_STRC)
        id.data.structure = structure;

    // read unit
    if (elements & SSE_UNIT) {
        // TODO
    }

    // get idn attribute
    if (elements & SSE_ATTR)
        id.data.attr = *(sercos_service_attribute *)&attr;

    if (elements & SSE_MINVAL)
        id.string_to_min_val(min_value.c_str());

    if (elements & SSE_MAXVAL)
        id.string_to_max_val(max_value.c_str());

    if (elements & SSE_DATA)
        id.string_to_val(value.c_str());

    try {
        _instance->sercos_write_idn(idn, elements, id.data);
    } catch (std::exception& e) {
        error_message = e.what();
    }

#define WRITE_ID_RESP_ERROR_MESSAGE  0
    response.resize(1);
    response[WRITE_ID_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}

const std::string sercos_protocol::handler::service_definition_write_id =
"request:\n"
"   uint16_t: idn\n"
"   uint8_t: elements\n"
"   uint16_t: structure\n"
"   string: name\n"
"   string: unit\n"
"   uint32_t: attr\n"
"   string: min_value\n"
"   string: max_value\n"
"   string: value\n"
"response:\n"
"   string: error_message\n";

