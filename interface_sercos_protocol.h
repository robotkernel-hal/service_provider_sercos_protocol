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

#define LN_UNREGISTER_SERVICE_IN_BASE_DETOR  
#include "ln_messages.h"
#undef LN_UNREGISTER_SERVICE_IN_BASE_DETOR

#define INTFNAME "[interface_sercos_protocol] "

namespace interface_sercos_protocol {
    
class sercos_protocol : 
    public ln_service_read_id_base,
    public ln_service_write_id_base,
    public ln_service_set_command_base 
{
    const std::string _mod_name;
    const std::string _dev_name;
    const int _slave_id;

    typedef std::map<int, service_id *> id_map_t;
    id_map_t service_ids;

    public:
        //! default construction
        /*!
         * \param mod_name module name to register for
         * \param dev_name device name
         * \parma slave_id module slave id
         */
        sercos_protocol(const std::string& mod_name, 
                const std::string& dev_name, const int& slave_id);

        //! service read id callback
        int on_read_id(ln::service_request& req, 
                ln_service_robotkernel_sercos_protocol_read_id& svc);

        //! service write id callback
        int on_write_id(ln::service_request& req, 
                ln_service_robotkernel_sercos_protocol_write_id& svc);

        //! service set command callback
        int on_set_command(ln::service_request& req, 
                ln_service_robotkernel_sercos_protocol_set_command& svc);
};

} // namespace interface

#endif // __INTERFACE_SERCOS_PROTOCOL_H__

