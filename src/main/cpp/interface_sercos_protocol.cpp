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

SERVICE_PROVIDER_DEF(sercos_protocol, interface_sercos_protocol::sercos_protocol);

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;
using namespace interface_sercos_protocol;

const char* interface_sercos_protocol::sercos_protocol_sp_magic = "sercos_protocol"; 

#if defined __VXWORKS__ || defined __QNX__
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define strnlen(a, b) \
	min(strlen((a)), (b))
#endif

//! handler construction
sercos_protocol_handler::sercos_protocol_handler(
		std::string mod_name, std::string dev_name, int slave_id) : 
	log_base(mod_name, (mod_name + "." + dev_name + ".sercos_protocol")), 
	mod_name(mod_name), dev_name(dev_name), slave_id(slave_id) {
	kernel& k = *kernel::get_instance();

	stringstream base;
	base << mod_name << "." << dev_name << ".sercos_protocol.";

	k.add_service(mod_name, base.str() + "read_id", 
			service_definition_read_id,
			std::bind(&sercos_protocol_handler::service_read_id, this, _1, _2));
	k.add_service(mod_name, base.str() + "write_id", 
			service_definition_write_id,
			std::bind(&sercos_protocol_handler::service_write_id, this, _1, _2));
	k.add_service(mod_name, base.str() + "set_command", 
			service_definition_set_command,
			std::bind(&sercos_protocol_handler::service_set_command, this, _1, _2));
}

//! service callback request read id
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int sercos_protocol_handler::service_read_id(const robotkernel::service_arglist_t& request, 
		robotkernel::service_arglist_t& response) {
#define READ_ID_REQ_IDN			0
#define READ_ID_REQ_ELEMENTS	1
	uint16_t idn     = request[READ_ID_REQ_IDN];
	uint8_t elements = request[READ_ID_REQ_ELEMENTS];

	// default response values 
	string error_message = "", name = "", value = "",
		   min_value = "N/A", max_value = "N/A", unit = "";
	int32_t state = 0;
	uint16_t structure = 0;
	uint32_t attr = 0;

	if (service_ids.find(idn) == service_ids.end()) {
		service_ids[idn] = new service_id(mod_name, 
				slave_id, idn, 0);
	}

	service_id& id = *service_ids[idn];
	id.update_elements(elements);

	if (id.status != "") {
		// error occured
		error_message = id.status;
		goto read_id_exit;
	}

	// get id name 
	if (elements & SSE_NAME)
		name = string(&id.data.name[4], ((uint16_t *)id.data.name)[0]);

	// read structure
	if (elements & SSE_STRC)
		structure = id.data.structure;

	// read unit
	if (elements & SSE_UNIT) {
		uint16_t len = ((uint16_t *)id.data.unit)[0];

		if ((len != 0) && (len <= 12)) 
			unit = string(&id.data.unit[4], len);
	}

	// get idn attribute
	if (elements & SSE_ATTR)
		attr = *(uint32_t *)&id.data.attr;

	if (elements & SSE_MINVAL) {
		if (id.data.attr.datatype == SSA_DATATYPE_CHARSET)
			min_value = string("N/A");
		else
			min_value = id.min_val_to_string();
	}

	if (elements & SSE_MAXVAL) {
		if (id.data.attr.datatype == SSA_DATATYPE_CHARSET)
			max_value = string("N/A");
		else
			max_value = id.max_val_to_string();
	}

	if (elements & SSE_DATA)
		value = id.val_to_string();

read_id_exit:
#define READ_ID_RESP_STATE			0
#define READ_ID_RESP_STRUCTURE		1
#define READ_ID_RESP_NAME			2
#define READ_ID_RESP_UNIT			3
#define READ_ID_RESP_ATTR			4
#define READ_ID_RESP_MIN_VALUE		5
#define READ_ID_RESP_MAX_VALUE		6
#define READ_ID_RESP_VALUE			7
#define READ_ID_RESP_ERROR_MESSAGE	8
	response.resize(9);
	response[READ_ID_RESP_STATE]         = state;
	response[READ_ID_RESP_STRUCTURE]     = structure;
	response[READ_ID_RESP_NAME]          = name;
	response[READ_ID_RESP_UNIT]          = unit;
	response[READ_ID_RESP_ATTR]          = attr;
	response[READ_ID_RESP_MIN_VALUE]     = min_value;
	response[READ_ID_RESP_MAX_VALUE]     = max_value;
	response[READ_ID_RESP_VALUE]         = value;
	response[READ_ID_RESP_ERROR_MESSAGE] = error_message;

	return 0;
}

const std::string sercos_protocol_handler::service_definition_read_id =
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
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int sercos_protocol_handler::service_write_id(const robotkernel::service_arglist_t& request, 
		robotkernel::service_arglist_t& response) {
#define WRITE_ID_REQ_IDN		0
#define WRITE_ID_REQ_ELEMENTS	1
#define WRITE_ID_REQ_STATE		2
#define WRITE_ID_REQ_STRUCTURE	3
#define WRITE_ID_REQ_NAME		4
#define WRITE_ID_REQ_UNIT		5
#define WRITE_ID_REQ_ATTR		6
#define WRITE_ID_REQ_MIN_VALUE	7
#define WRITE_ID_REQ_MAX_VALUE  8
#define WRITE_ID_REQ_VALUE      9
	uint16_t idn        = request[WRITE_ID_REQ_IDN];
	uint8_t elements    = request[WRITE_ID_REQ_ELEMENTS];
	int32_t state       = request[WRITE_ID_REQ_STATE];
	uint16_t structure  = request[WRITE_ID_REQ_STRUCTURE];
	string name         = request[WRITE_ID_REQ_NAME];
	string unit         = request[WRITE_ID_REQ_UNIT];
	uint32_t attr       = request[WRITE_ID_REQ_ATTR];
	string min_value    = request[WRITE_ID_REQ_MIN_VALUE];
	string max_value    = request[WRITE_ID_REQ_MAX_VALUE];
	string value        = request[WRITE_ID_REQ_VALUE];

	// response 
	string error_message = "";

	if (service_ids.find(idn) == service_ids.end())
		service_ids[idn] = new service_id(mod_name, slave_id, idn, 0);

	service_id& id = *service_ids[idn];
	id.update_elements(SSE_ATTR);

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

	id.write_elements(elements);

	if (id.status != "")
		// error occured
		error_message = id.status;

#define WRITE_ID_RESP_ERROR_MESSAGE  0
	response.resize(1);
	response[WRITE_ID_RESP_ERROR_MESSAGE] = error_message;

	return 0;
}

const std::string sercos_protocol_handler::service_definition_write_id =
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
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int sercos_protocol_handler::service_set_command(const robotkernel::service_arglist_t& request, 
		robotkernel::service_arglist_t& response) {
#define SET_COMMAND_REQ_CMD		0
	uint16_t cmd_req = request[SET_COMMAND_REQ_CMD];
	sercos_set_command_t cmd = { slave_id, cmd_req }; 

	// default response values 
	string error_message = "";

	// execute procedure command    
	if (kernel::request_cb(mod_name.c_str(), 
				MOD_REQUEST_SERCOS_SET_COMMAND, (void *)&cmd) != 0) {
		// error occured
		error_message = "executing procedure command failed";
	}

#define SET_COMMAND_RESP_ERROR_MESSAGE	0
	response.resize(1);
	response[SET_COMMAND_RESP_ERROR_MESSAGE] = error_message;

	return 0;
}

const std::string sercos_protocol_handler::service_definition_set_command =
"request:\n"
"   uint16_t: cmd\n"
"response:\n"
"   string: error_message\n";

