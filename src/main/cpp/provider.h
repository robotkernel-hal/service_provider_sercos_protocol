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

#ifndef __SERVICE_PROVIDER__SERCOS_PROTOCOL__PROVIDER_H__
#define __SERVICE_PROVIDER__SERCOS_PROTOCOL__PROVIDER_H__

#include "service_id.h"

#include "robotkernel/service_provider_base.h"
#include "robotkernel/service_provider_intf.h"
#include "robotkernel/service.h"
#include "robotkernel/kernel.h"
#include "robotkernel/log_base.h"

namespace service_provider {
#ifdef EMACS
}
#endif

namespace sercos_protocol {
#ifdef EMACS
}
#endif

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

class handler : public robotkernel::log_base {
    public:
        typedef std::shared_ptr<service_provider::sercos_protocol::base> sp_sp_base_t;
        sp_sp_base_t _instance;

        //! handler construction
        handler(const robotkernel::sp_service_interface_t& req);

        //! handler destruction
        ~handler();

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
};

#ifdef EMACS
{
#endif
}; // sercos protocol

#ifdef EMACS
{
#endif
}; // service provider

#endif // __SERVICE_PROVIDER__SERCOS_PROTOCOL__PROVIDER_H__

