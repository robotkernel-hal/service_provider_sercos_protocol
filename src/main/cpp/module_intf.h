//! robotkernel interface sercos protocol requests
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

#ifndef __INTERFACE_SERCOS_PROTOCOL_MODULE_INTF_H__
#define __INTERFACE_SERCOS_PROTOCOL_MODULE_INTF_H__


//! sercos protocol specific service transfer
typedef struct sercos_service_transfer {
    int slave_id;                           //!< slave id
    uint16_t idn;                           //!< id number
    sercos_service_element_t element;       //!< element numbers
    sercos_service_direction_t direction;   //!< direction (to master or to slave)

    size_t buflen;                          //!< answer buffer length
    uint16_t *buf;                          //!< answer buffer
} sercos_service_transfer_t;

//! sercos execute procedure command on drive
typedef struct sercos_set_command {
    int slave_id;                           //! at number
    uint16_t cmd;                           //! procedure command
} sercos_set_command_t;

#define MOD_REQUEST_SERCOS_PROTOCOL_MAGIC  0x23
#define MOD_REQUEST_SERCOS_PROTOCOL(x, s) \
    __MOD_REQUEST((MOD_REQUEST_SERCOS_PROTOCOL_MAGIC), (x), __MOD_REQUEST_TYPE(s))

#define MOD_REQUEST_SERCOS_SERVICE_TRANSFER  MOD_REQUEST_SERCOS_PROTOCOL(0x0001, sercos_service_transfer_t)
#define MOD_REQUEST_SERCOS_GET_BAUDRATE      MOD_REQUEST_SERCOS_PROTOCOL(0x0002, int)
#define MOD_REQUEST_SERCOS_SET_BAUDRATE      MOD_REQUEST_SERCOS_PROTOCOL(0x0003, int)
#define MOD_REQUEST_SERCOS_GET_CYCLETIME     MOD_REQUEST_SERCOS_PROTOCOL(0x0004, int)
#define MOD_REQUEST_SERCOS_SET_CYCLETIME     MOD_REQUEST_SERCOS_PROTOCOL(0x0005, int)
#define MOD_REQUEST_SERCOS_GET_PHASE         MOD_REQUEST_SERCOS_PROTOCOL(0x0006, int)
#define MOD_REQUEST_SERCOS_SET_PHASE         MOD_REQUEST_SERCOS_PROTOCOL(0x0007, int)
#define MOD_REQUEST_SERCOS_GET_MASTERCLOCK   MOD_REQUEST_SERCOS_PROTOCOL(0x0008, int)
#define MOD_REQUEST_SERCOS_SET_MASTERCLOCK   MOD_REQUEST_SERCOS_PROTOCOL(0x0009, int)
#define MOD_REQUEST_SERCOS_SET_COMMAND       MOD_REQUEST_SERCOS_PROTOCOL(0x000A, sercos_set_command_t)

#endif // __INTERFACE_SERCOS_PROTOCOL_MODULE_INTF_H__

