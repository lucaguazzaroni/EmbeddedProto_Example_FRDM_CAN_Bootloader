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

#include <CanBus.hpp>
#include <string.h>

#include "fsl_mscan.h"
#include "pin_mux.h"
#include "board.h"


void CanBus::open()
{
	mscan_config_t mscanConfig;

	MSCAN_GetDefaultConfig(&mscanConfig);

	/* baud rate */
	mscanConfig.baudRate = m_baudRate;

	/* Acceptance filter configuration. */
	mscanConfig.filterConfig.u32IDAR0 = MSCAN_RX_MB_EXT_MASK(m_nodeId);
	mscanConfig.filterConfig.u32IDAR1 = MSCAN_RX_MB_EXT_MASK(m_nodeId);
	mscanConfig.filterConfig.u32IDMR0 = (MSCAN_REIDR3_RERTR_MASK | (MSCAN_REIDR1_RSRR_MASK | MSCAN_REIDR1_REIDE_MASK) << 16U);
	mscanConfig.filterConfig.u32IDMR1 = (MSCAN_REIDR3_RERTR_MASK | (MSCAN_REIDR1_RSRR_MASK | MSCAN_REIDR1_REIDE_MASK) << 16U);

	/* Initialize MSCAN module. */
	MSCAN_Init(MSCAN, &mscanConfig, CLOCK_GetFreq(kCLOCK_Osc0ErClk));

	/* Create MSCAN handle structure and set call back function. */
	m_flags.rxComplete = false;
	m_flags.txComplete = false;

	MSCAN_TransferCreateHandle(MSCAN, &m_handle, CanBus::isrCallback, &m_flags);
}


void CanBus::close()
{
	MSCAN_Deinit(MSCAN);
}


void CanBus::send(const uint8_t *data, const size_t dlen)
{
	if (data == nullptr) {
		return;
	}

	/* Prepare Tx Frame for sending. */
	mscan_frame_t txFrame {};
	txFrame.ID_Type.ID = m_nodeId;
	txFrame.format     = kMSCAN_FrameFormatExtend;
	txFrame.type       = kMSCAN_FrameTypeData;
	txFrame.DLR        = (dlen > CanBus::PAYLOAD_SIZE) ? CanBus::PAYLOAD_SIZE : dlen;

	memcpy(txFrame.DSR, data, txFrame.DLR);

	/* Send data through Tx Message Buffer. */
	mscan_mb_transfer_t txXfer {};
	txXfer.frame = &txFrame;
	txXfer.mask  = kMSCAN_TxEmptyInterruptEnable;
	MSCAN_TransferSendNonBlocking(MSCAN, &m_handle, &txXfer);

	while (!m_flags.txComplete){};
	m_flags.txComplete = false;
}


size_t CanBus::receive(uint8_t *dst, const size_t dlen, const uint32_t timeoutMs)
{
	mscan_mb_transfer_t rxXfer {};
	mscan_frame_t rxFrame {};

	/* Start receive data through Rx Message Buffer. */
	rxXfer.frame = &rxFrame;
	rxXfer.mask  = kMSCAN_RxFullInterruptEnable;
	MSCAN_TransferReceiveNonBlocking(MSCAN, &m_handle, &rxXfer);

	/* Waiting for Message receive finish. */
	while (!m_flags.rxComplete){};
	m_flags.rxComplete = false;

	if (dlen < rxFrame.DLR) {
		return 0;
	}

	memcpy(dst, rxFrame.DSR, rxFrame.DLR);
	return static_cast<size_t>(rxFrame.DLR);
}


void CanBus::isrCallback(MSCAN_Type *base, mscan_handle_t *handle, status_t status, void *userData)
{
	volatile CanBusTransferFlags_t *flags = static_cast<CanBusTransferFlags_t*>(userData);

	switch (status)
    {
        /* Process MSCAN Rx event. */
        case kStatus_MSCAN_RxIdle:
        	flags->rxComplete = true;
            break;

        /* Process MSCAN Tx event. */
        case kStatus_MSCAN_TxIdle:
        	flags->txComplete = true;
            break;

        default:
            break;
    }
}
