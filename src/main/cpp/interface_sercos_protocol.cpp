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

#include "interface_sercos_protocol.h"
#include "service_id.h"

#include "robotkernel/exceptions.h"
#include "robotkernel/kernel.h"

INTERFACE_DEF(sercos_protocol, interface_sercos_protocol::sercos_protocol)

using namespace std;
using namespace robotkernel;
using namespace interface_sercos_protocol;
        
#if defined __VXWORKS__ || defined __QNX__
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define strnlen(a, b) \
    min(strlen((a)), (b))
#endif

//! default construction
/*!
 * \param node configuration node
 */
sercos_protocol::sercos_protocol(const YAML::Node& node) 
    : interface_base("sercos_protocol", node) {
    kernel& k = *kernel::get_instance();
    if (!k.clnt)
        throw str_exception("[interface_sercos_protocol|%s] no ln_connection!\n", 
                mod_name.c_str());
    
    stringstream base;
    base << k.clnt->name << "." << mod_name << "." << dev_name << ".";

    register_read_id(k.clnt, base.str() + "sercos_protocol.read_id");
    register_write_id(k.clnt, base.str() + "sercos_protocol.write_id");
    register_set_command(k.clnt, base.str() + "sercos_protocol.set_command");
}

//! service read id callback
int sercos_protocol::on_read_id(ln::service_request& req, 
        ln_service_robotkernel_sercos_protocol_read_id& svc) {
    
    if (service_ids.find(svc.req.idn) == service_ids.end()) {
        service_ids[svc.req.idn] = new service_id(mod_name, 
                slave_id, svc.req.idn, 0);
    }

    service_id& id = *service_ids[svc.req.idn];
    id.update_elements(svc.req.elements);

    if (id.status != "") {
        // error occured
        svc.resp.error_message = strdup(id.status.c_str());
        svc.resp.error_message_len = strlen(svc.resp.error_message);
        req.respond();
        free(svc.resp.error_message);
        return 0;
    }
    
    // get id name 
    if (svc.req.elements & SSE_NAME) {
        svc.resp.name = &id.data.name[4];
        svc.resp.name_len = strnlen(svc.resp.name, 
                ((uint16_t *)id.data.name)[0]);
    }
    
    // read structure
    if (svc.req.elements & SSE_STRC)
        svc.resp.structure = id.data.structure;
    
    // read unit
    if (svc.req.elements & SSE_UNIT) {
        uint16_t len = ((uint16_t *)id.data.unit)[0];

        if (len == 0) {
            svc.resp.unit = NULL;
            svc.resp.unit_len = 0;
        } else if (len <= 12) {
            svc.resp.unit = &id.data.unit[4];
            svc.resp.unit_len = len;
        }
    }
        
    // get idn attribute
    if (svc.req.elements & SSE_ATTR)
        svc.resp.attr = *(uint32_t *)&id.data.attr;

    if (svc.req.elements & SSE_MINVAL) {
        if (id.data.attr.datatype == SSA_DATATYPE_CHARSET)
            svc.resp.min_value = strdup("N/A");
        else {
            string tmp = id.min_val_to_string();
            svc.resp.min_value = strdup(tmp.c_str());
        }
        svc.resp.min_value_len = strlen(svc.resp.min_value);
    }
    
    if (svc.req.elements & SSE_MAXVAL) {
        if (id.data.attr.datatype == SSA_DATATYPE_CHARSET)
            svc.resp.max_value = strdup("N/A");
        else {
            string tmp = id.max_val_to_string();
            svc.resp.max_value = strdup(tmp.c_str());
        }
        svc.resp.max_value_len = strlen(svc.resp.max_value);
    }

    if (svc.req.elements & SSE_DATA) {
        string tmp = id.val_to_string();
        svc.resp.value = strdup(tmp.c_str());
        svc.resp.value_len = strlen(svc.resp.value);
    }

    req.respond();

    if (svc.resp.value)
        free(svc.resp.value);
    if (svc.resp.min_value)
        free(svc.resp.min_value);
    if (svc.resp.max_value)
        free(svc.resp.max_value);

    return 0;
}

//! service write id callback
int sercos_protocol::on_write_id(ln::service_request& req, 
        ln_service_robotkernel_sercos_protocol_write_id& svc) {
    string value(svc.req.value, svc.req.value_len);
    
    if (service_ids.find(svc.req.idn) == service_ids.end()) {
        service_ids[svc.req.idn] = new service_id(mod_name, 
                slave_id, svc.req.idn, 0);
    }

    service_id& id = *service_ids[svc.req.idn];
    id.update_elements(SSE_ATTR);

    if (svc.req.elements & SSE_NAME) {
//        svc.resp.name = &id.data.name[4];
//        svc.resp.name_len = ((uint16_t *)id.data.name)[0];
    }
    
    // read structure
    if (svc.req.elements & SSE_STRC)
        id.data.structure = svc.req.structure;
    
    // read unit
    if (svc.req.elements & SSE_UNIT) {
//        uint16_t len = ((uint16_t *)id.data.unit)[0];
//
//        if (len <= 12) {
//            svc.resp.unit = &id.data.unit[4];
//            svc.resp.unit_len = len;
//        }
    }
        
    // get idn attribute
    if (svc.req.elements & SSE_ATTR)
        id.data.attr = *(sercos_service_attribute *)&svc.req.attr;

    if (svc.req.elements & SSE_MINVAL) {
        string min_val(svc.req.min_value, svc.req.min_value_len);
        id.string_to_min_val(min_val.c_str());
    }
    
    if (svc.req.elements & SSE_MAXVAL) {
        string max_val(svc.req.max_value, svc.req.max_value_len);
        id.string_to_max_val(max_val.c_str());
    }

    if (svc.req.elements & SSE_DATA) {
        string val(svc.req.value, svc.req.value_len);
        id.string_to_val(val.c_str());
    }

    id.write_elements(svc.req.elements);

    if (id.status != "") {
        // error occured
        svc.resp.error_message = strdup(id.status.c_str());
        svc.resp.error_message_len = strlen(svc.resp.error_message);
    }

    req.respond();

    if (svc.resp.error_message)
        free(svc.resp.error_message);

    return 0;
}

//! service set command callback
int sercos_protocol::on_set_command(ln::service_request& req, 
        ln_service_robotkernel_sercos_protocol_set_command& svc) {
    sercos_set_command_t cmd = { slave_id, svc.req.cmd };

    // execute procedure command    
    int ret = kernel::request_cb(mod_name.c_str(), 
            MOD_REQUEST_SERCOS_SET_COMMAND, (void *)&cmd);

    if (ret != 0) {
        // error occured
        svc.resp.error_message = strdup("executing procedure command failed");
        svc.resp.error_message_len = strlen(svc.resp.error_message);
    }

    req.respond();
    
    if (svc.resp.error_message)
        free(svc.resp.error_message);

    return 0;
}

