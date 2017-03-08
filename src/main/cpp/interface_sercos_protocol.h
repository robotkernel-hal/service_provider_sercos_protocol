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

#ifndef __INTERFACE_SERCOS_PROTOCOL_H__
#define __INTERFACE_SERCOS_PROTOCOL_H__

#include "service_id.h"

#include "robotkernel/service_provider_base.h"
#include "robotkernel/service_provider_intf.h"
#include "robotkernel/service.h"
#include "robotkernel/kernel.h"
#include "robotkernel/log_base.h"

namespace interface_sercos_protocol {
	extern const char* sercos_protocol_sp_magic;

	// forward declaration
	class sercos_protocol_handler;

	class sercos_protocol : 
		public robotkernel::service_provider_base<sercos_protocol_handler> {
			public:
				//! default construction
				/*!
				 * \param node configuration node
				 */
				sercos_protocol()
					: service_provider_base("sercos_protocol") {};

				~sercos_protocol() {};

				//! service provider magic 
				/*!
				 * \return return service provider magic string
				 */
				const char* get_sp_magic() 
				{ return sercos_protocol_sp_magic; };
		};

	class sercos_protocol_handler : public robotkernel::log_base {
		public:
			std::string mod_name;	//!< slave owner module
			std::string dev_name;	//!< service device name
			int slave_id;			//!< slave identifier
			
			typedef std::map<int, service_id *> id_map_t;
			id_map_t service_ids;

			//! handler construction
			sercos_protocol_handler(std::string mod_name, std::string dev_name, 
					int slave_id);

			//! handler destruction
			~sercos_protocol_handler();

			//! service callback request read id
			/*!
		 	 * \param request service request data
			 * \parma response service response data
			 * \return success
			 */
			int service_read_id(const robotkernel::service_arglist_t& request, 
					robotkernel::service_arglist_t& response);
			static const std::string service_definition_read_id;

			//! service callback request write id
			/*!
		 	 * \param request service request data
			 * \parma response service response data
			 * \return success
			 */
			int service_write_id(const robotkernel::service_arglist_t& request, 
					robotkernel::service_arglist_t& response);
			static const std::string service_definition_write_id;

			//! service callback request set command
			/*!
		 	 * \param request service request data
			 * \parma response service response data
			 * \return success
			 */
			int service_set_command(const robotkernel::service_arglist_t& request, 
					robotkernel::service_arglist_t& response);
			static const std::string service_definition_set_command;
	};

} // namespace interface

#endif // __INTERFACE_SERCOS_PROTOCOL_H__

