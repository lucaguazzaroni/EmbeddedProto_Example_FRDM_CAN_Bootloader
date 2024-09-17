/*
 * CanBus.hpp
 *
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
