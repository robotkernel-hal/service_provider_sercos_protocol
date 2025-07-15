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

#ifndef SERVICE_PROVIDER_SERCOS_PROTOCOL__PROVIDER_H
#define SERVICE_PROVIDER_SERCOS_PROTOCOL__PROVIDER_H

#include "service_id.h"

// Robotkernel includes
#include "robotkernel/service_provider_base.h"
#include "robotkernel/service.h"
#include "robotkernel/log_base.h"

// Service provider includes 
#include "service_definitions.h"

namespace service_provider_sercos_protocol {

// forward declaration
class handler;

class provider : public robotkernel::service_provider_base<handler, base> {
    public:
        //! default construction
        /*!
         * \param node configuration node
         */
        provider(const std::string& name)
            : service_provider_base(name, "sercos_protocol") {};
};

class handler : 
    public robotkernel::log_base,
    public svc_base_read_id,
    public svc_base_write_id
{
    public:
        typedef std::shared_ptr<service_provider_sercos_protocol::base> sp_sp_base_t;
        sp_sp_base_t _instance;

        //! handler construction
        handler(const robotkernel::sp_service_interface_t& req);

        //! handler destruction
        ~handler() {}

        //! svc_read_id
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        virtual void svc_read_id(const struct svc_req_read_id& req, struct svc_resp_read_id& resp);

        //! svc_write_id
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        virtual void svc_write_id(const struct svc_req_write_id& req, struct svc_resp_write_id& resp);
};

}; // service_provider_sercos_protocol

#endif // SERVICE_PROVIDER_SERCOS_PROTOCOL__PROVIDER_H

