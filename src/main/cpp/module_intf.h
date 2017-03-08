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

//! direction of service transfers
typedef enum sercos_service_direction {
    SSD_MASTER_TO_DRIVE = 0,            //!< data download to drive
    SSD_DRIVE_TO_MASTER = 1             //!< data upload from drive
} sercos_service_direction_t;

//! defines for element codes */
typedef enum sercos_service_element {
    SSE_NONE   = 0x00,                  //!< place holder for element
    SSE_STRC   = 0x02,                  //!< id structure
    SSE_NAME   = 0x04,                  //!< name element of sercos id
    SSE_ATTR   = 0x08,                  //!< attribute of sercos id
    SSE_UNIT   = 0x10,                  //!< unit identifier of sercos id
    SSE_MINVAL = 0x20,                  //!< minimum value of sercos id data
    SSE_MAXVAL = 0x40,                  //!< maximum value of sercos id data
    SSE_DATA   = 0x80                   //!< sercos id data
} sercos_service_element_t;

typedef enum sercos_service_attribute_datalength {
    SSA_DATALENGTH_RESERVED   = 0x0,
    SSA_DATALENGTH_2BYTEFIX   = 0x1,    //!< 2 byte fixed width
    SSA_DATALENGTH_4BYTEFIX   = 0x2,    //!< 4 byte fixed width
    SSA_DATALENGTH_8BYTEFIX   = 0x3,    //!< 8 byte fixed width
    SSA_DATALENGTH_1BYTEVAR   = 0x4,    //!< variable width with one byte length
    SSA_DATALENGTH_2BYTEVAR   = 0x5,    //!< variable width with two byte length
    SSA_DATALENGTH_4BYTEVAR   = 0x6,    //!< variable width with four byte length
    SSA_DATALENGTH_8BYTEVAR   = 0x7     //!< variable width with eight byte length
} sercos_service_attribute_datalength_t;

typedef enum sercos_service_attribute_datatype {
    SSA_DATATYPE_BINARY   = 0x0,        //!< binary data
    SSA_DATATYPE_UINT     = 0x1,        //!< unsigned integer
    SSA_DATATYPE_INT      = 0x2,        //!< unsinged integer
    SSA_DATATYPE_UINT2    = 0x3,        //!< unsigned hex integer
    SSA_DATATYPE_CHARSET  = 0x4,        //!< strings
    SSA_DATATYPE_UINT3    = 0x5,        //!< another unsigned integer
    SSA_DATATYPE_FLOAT    = 0x6,        //!< floats
    SSA_DATATYPE_RESERVED = 0x7,        //!< nothing to say 
} sercos_service_attribute_datatype_t;

typedef struct sercos_service_attribute {
    uint16_t conversionfactor;          //!< conversion factor
    unsigned datalength   : 3;          //!< data length in bytes
    unsigned function     : 1;          //!< function
    unsigned datatype     : 3;          //!< data type
    unsigned reserved1    : 1;          //!< n/a
    unsigned decimalpoint : 4;          //!< decimal point
    unsigned wp_cp2       : 1;          //!< write protect phase 2
    unsigned wp_cp3       : 1;          //!< write protect phase 3
    unsigned wp_cp4       : 1;          //!< write protect phase 4
    unsigned reserved2    : 1;          //!< n/a
} sercos_service_attribute_t;

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

