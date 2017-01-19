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
#include "robotkernel/interface_base.h"

namespace interface_sercos_protocol {
    
class sercos_protocol : public robotkernel::interface_base {
    typedef std::map<int, service_id *> id_map_t;
    id_map_t service_ids;

    public:
        //! default construction
        /*!
         * \param node configuration node
         */
        sercos_protocol(const YAML::Node& node);

        //! service callback request read id
        /*!
         * \param message service message
         * \return success
         */
        int service_read_id(YAML::Node& message);
        static const std::string service_definition_read_id;

        //! service callback request write id
        /*!
         * \param message service message
         * \return success
         */
        int service_write_id(YAML::Node& message);
        static const std::string service_definition_write_id;

        //! service callback request set command
        /*!
         * \param message service message
         * \return success
         */
        int service_set_command(YAML::Node& message);
        static const std::string service_definition_set_command;
};

} // namespace interface

#endif // __INTERFACE_SERCOS_PROTOCOL_H__

