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

#include "provider.h"
#include "service_id.h"
#include "service_definitions.h"

#include "robotkernel/exceptions.h"

SERVICE_PROVIDER_DEF(sercos_protocol, service_provider_sercos_protocol::provider);

using namespace std;
using namespace robotkernel;
using namespace service_provider_sercos_protocol;

//! handler construction
handler::handler(const robotkernel::sp_service_interface_t& req) 
    : log_base(req->owner, "sercos_protocol", req->device_name) 
{
    _instance = std::dynamic_pointer_cast<service_provider_sercos_protocol::base>(req);
    if (!_instance)
        throw runtime_error(string("wrong base class"));

    add_svc_read_id(_instance->owner, _instance->device_name + ".read_id");
    add_svc_write_id(_instance->owner, _instance->device_name + ".write_id");
}

//! svc_read_id
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void handler::svc_read_id(const struct svc_req_read_id& req, struct svc_resp_read_id& resp) {
    uint8_t elements = req.elements;

    if (    (elements & SSE_MINVAL) ||
            (elements & SSE_MAXVAL) ||
            (elements & SSE_DATA)) 
        elements |= SSE_ATTR;   // we have to read attribute to interpret data

    try {
        service_id id;
        _instance->sercos_read_idn(req.idn, elements, id.data);
    
        if (elements & SSE_ATTR) 
            resp.attr = *(uint32_t *)&id.data.attr;

        if (    (elements & SSE_MINVAL) &&
                (id.data.attr.datatype != SSA_DATATYPE_CHARSET))
            resp.min_value = id.min_val_to_string();

        if (    (elements & SSE_MAXVAL) &&
                (id.data.attr.datatype == SSA_DATATYPE_CHARSET))
            resp.max_value = id.max_val_to_string();

        if (elements & SSE_DATA)
            resp.value = id.val_to_string();
    } catch (std::exception& e) {
        resp.error_message = e.what();
    }
}


//! svc_write_id
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void handler::svc_write_id(const struct svc_req_write_id& req, struct svc_resp_write_id& resp) {
    service_id id;    

    if (req.elements & SSE_NAME) {
        // TODO
    }

    // read structure
    if (req.elements & SSE_STRC)
        id.data.structure = req.structure;

    // read unit
    if (req.elements & SSE_UNIT) {
        // TODO
    }

    // get idn attribute
    if (req.elements & SSE_ATTR)
        id.data.attr = *(sercos_service_attribute *)&req.attr;
    else {
        try {
            _instance->sercos_read_idn(req.idn, SSE_ATTR, id.data);
        } catch (std::exception& e) {
            resp.error_message = e.what();
            return;
        }
    }

    if (req.elements & SSE_MINVAL)
        id.string_to_min_val(req.min_value.c_str());

    if (req.elements & SSE_MAXVAL)
        id.string_to_max_val(req.max_value.c_str());

    if (req.elements & SSE_DATA)
        id.string_to_val(req.value.c_str());

    try {
        _instance->sercos_write_idn(req.idn, req.elements, id.data);
    } catch (std::exception& e) {
        resp.error_message = e.what();
    }
}

