/*
 *  Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved
 *
 *  This file is part of Embedded Proto.
 *
 *  Embedded Proto is open source software: you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation, version 3 of the license.
 *
 *  Embedded Proto  is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
 *
 *  For commercial and closed source application please visit:
 *  <https://EmbeddedProto.com/license/>.
 *
 *  Embedded AMS B.V.
 *  Info:
 *    info at EmbeddedProto dot com
 *
 *  Postal address:
 *    Atoomweg 2
 *    1627 LE, Hoorn
 *    the Netherlands
 */

#ifndef CANBUS_HPP_
#define CANBUS_HPP_

#include "fsl_mscan.h"


typedef struct {
	bool txComplete;
	bool rxComplete;
} CanBusTransferFlags_t;


class CanBus {
public:
	static constexpr size_t PAYLOAD_SIZE = 8;

	CanBus(uint32_t nodeId, uint32_t baudRate): m_nodeId(nodeId), m_baudRate(baudRate) {}

	void open();
	void close();

	void send(const uint8_t *data, const size_t dlen);
	size_t receive(uint8_t *dst, const size_t dlen, const uint32_t timeoutMs);

private:
	static void isrCallback(MSCAN_Type *base, mscan_handle_t *handle, status_t status, void *userData);

	const uint32_t m_nodeId;
	const uint32_t m_baudRate;

	mscan_handle_t m_handle;
	CanBusTransferFlags_t m_flags;
};


#endif /* CANBUS_HPP_ */
