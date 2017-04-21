//! robotkernel interface sercos protocol
/*!
 * author: Robert Burger
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

#ifndef __SERVICE_PROVIDER__SERCOS_PROTOCOL__BASE__H__
#define __SERVICE_PROVIDER__SERCOS_PROTOCOL__BASE__H__

#include <list>

#include "robotkernel/service_requester_base.h"

namespace service_provider {

    namespace sercos_protocol {
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
        typedef uint8_t sercos_service_elements_t;

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

        //! service transfer data type
        typedef struct service_data {
            uint16_t structure;
            std::string name;            
            sercos_service_attribute attr;
            std::string unit;
            std::vector<uint16_t> min_value;
            std::vector<uint16_t> max_value;
            std::vector<uint16_t> value;
        } service_data_t;

        class base : public robotkernel::service_requester_base {
            public:
                //! construction
                base(std::string owner, std::string service_prefix)
                : robotkernel::service_requester_base(owner, service_prefix) {};

                //! destruction
                virtual ~base() = 0;

                //! read sercos id number
                /*!
                 * \param idn id number to read
                 * \param elements elements to read
                 * \param data data to read
                 */
                virtual void sercos_read_idn(const uint16_t& idn, 
                        const sercos_service_elements_t& elements, service_data_t& data) = 0;

                //! write sercos id number
                /*!
                 * \param idn id number to write
                 * \param elements elements to write
                 * \param data data to write
                 */
                virtual void sercos_write_idn(const uint16_t& idn, 
                        const sercos_service_elements_t& elements, service_data_t& data) = 0;
        };

        inline base::~base() { }

    }; // namespace sercos_protocol

}; // namespace service_provider

#endif // __SERVICE_PROVIDER__SERCOS_PROTOCOL__BASE__H__

