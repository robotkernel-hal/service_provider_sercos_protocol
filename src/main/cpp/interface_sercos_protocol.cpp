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
    
    stringstream base;
    base << mod_name << "." << dev_name << ".sercos_protocol.";

    k.add_service(mod_name, base.str() + "read_id", 
            service_definition_read_id,
            boost::bind(&sercos_protocol::service_read_id, this, _1));
    k.add_service(mod_name, base.str() + "write_id", 
            service_definition_write_id,
            boost::bind(&sercos_protocol::service_write_id, this, _1));
    k.add_service(mod_name, base.str() + "set_command", 
            service_definition_set_command,
            boost::bind(&sercos_protocol::service_set_command, this, _1));
}

//! service callback request read id
/*!
 * \param message service message
 * \return success
 */
int sercos_protocol::service_read_id(YAML::Node& message) {
    uint16_t idn = get_as<uint16_t>(message["request"], "idn");
    uint8_t elements = get_as<uint8_t>(message["request"], "elements");
   
    // default response values 
    message["response"]["state"]         = 0;
    message["response"]["structure"]     = 0;
    message["response"]["name"]          = "";
    message["response"]["unit"]          = "";
    message["response"]["attr"]          = 0;
    message["response"]["min_value"]     = "";
    message["response"]["max_value"]     = "";
    message["response"]["value"]         = "";
    message["response"]["error_message"] = "";

    if (service_ids.find(idn) == service_ids.end()) {
        service_ids[idn] = new service_id(mod_name, 
                slave_id, idn, 0);
    }

    service_id& id = *service_ids[idn];
    id.update_elements(elements);

    if (id.status != "") {
        // error occured
        message["response"]["error_message"] = id.status;
        return 0;
    }
    
    // get id name 
    if (elements & SSE_NAME)
        message["response"]["name"] = 
            string(&id.data.name[4], ((uint16_t *)id.data.name)[0]);
    
    // read structure
    if (elements & SSE_STRC)
        message["response"]["structure"] = id.data.structure;
    
    // read unit
    if (elements & SSE_UNIT) {
        uint16_t len = ((uint16_t *)id.data.unit)[0];

        if ((len != 0) && (len <= 12)) 
            message["response"]["unit"] = 
                string(&id.data.unit[4], len);
    }
        
    // get idn attribute
    if (elements & SSE_ATTR)
        message["response"]["attr"] = *(uint32_t *)&id.data.attr;

    if (elements & SSE_MINVAL) {
        if (id.data.attr.datatype == SSA_DATATYPE_CHARSET)
            message["response"]["min_value"] = string("N/A");
        else
            message["response"]["min_value"] = id.min_val_to_string();
    }
    
    if (elements & SSE_MAXVAL) {
        if (id.data.attr.datatype == SSA_DATATYPE_CHARSET)
            message["response"]["max_value"] = string("N/A");
        else
            message["response"]["max_value"] = id.max_val_to_string();
    }

    if (elements & SSE_DATA)
        message["response"]["value"] = id.val_to_string();

    return 0;
}

const std::string sercos_protocol::service_definition_read_id =
    "request:\n"
    "   uint16_t: idn\n"
    "   uint8_t: elements\n"
    "response:\n"
    "   int32_t: state\n"
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
 * \param message service message
 * \return success
 */
int sercos_protocol::service_write_id(YAML::Node& message) {
    uint16_t idn = get_as<uint16_t>(message["request"], "idn");
    uint8_t elements = get_as<uint8_t>(message["request"], "elements");
    
    if (service_ids.find(idn) == service_ids.end()) {
        service_ids[idn] = new service_id(mod_name, 
                slave_id, idn, 0);
    }

    service_id& id = *service_ids[idn];
    id.update_elements(SSE_ATTR);

    if (elements & SSE_NAME) {
        // TODO
        // string name = get_as<string>(message["request"], "name");
        // id.string_to_data(name.c_str(), &id.data.name, 
        //         &id.data.name_len);
    }
    
    // read structure
    if (elements & SSE_STRC)
        id.data.structure = get_as<uint16_t>(message["request"], "structure");
    
    // read unit
    if (elements & SSE_UNIT) {
        // TODO
        // string unit = get_as<string>(message["request"], "unit");
        // id.string_to_data(unit.c_str(), &id.data.unit, 
        //         &id.data.unit_len);
    }
        
    // get idn attribute
    if (elements & SSE_ATTR) {
        uint32_t attr = get_as<uint32_t>(message["response"], "attr");
        id.data.attr = *(sercos_service_attribute *)&attr;
    }

    if (elements & SSE_MINVAL) {
        string min_val = get_as<string>(message["request"], "min_value");
        id.string_to_min_val(min_val.c_str());
    }
    
    if (elements & SSE_MAXVAL) {
        string max_val = get_as<string>(message["request"], "max_value");
        id.string_to_max_val(max_val.c_str());
    }

    if (elements & SSE_DATA) {
        string value = get_as<string>(message["request"], "value");
        id.string_to_val(value.c_str());
    }

    id.write_elements(elements);

    if (id.status != "")
        // error occured
        message["response"]["error_message"] = id.status;

    return 0;
}

const std::string sercos_protocol::service_definition_write_id =
    "request:\n"
    "   uint16_t: idn\n"
    "   uint8_t: elements\n"
    "   int32_t: state\n"
    "   uint16_t: structure\n"
    "   string: name\n"
    "   string: unit\n"
    "   uint32_t: attr\n"
    "   string: min_value\n"
    "   string: max_value\n"
    "   string: value\n"
    "response:\n"
    "   string: error_message\n";

//! service callback request set command
/*!
 * \param message service message
 * \return success
 */
int sercos_protocol::service_set_command(YAML::Node& message) {
    sercos_set_command_t cmd = { slave_id, 
        get_as<int32_t>(message["request"], "cmd") };

    // default response values 
    message["response"]["error_message"] = "";

    // execute procedure command    
    if (kernel::request_cb(mod_name.c_str(), 
                MOD_REQUEST_SERCOS_SET_COMMAND, (void *)&cmd) != 0) {
        // error occured
        message["response"]["error_message"] = 
            "executing procedure command failed";
    }

    return 0;
}

const std::string sercos_protocol::service_definition_set_command =
    "request:\n"
    "   int32_t: cmd\n"
    "response:\n"
    "   string: error_message\n";

