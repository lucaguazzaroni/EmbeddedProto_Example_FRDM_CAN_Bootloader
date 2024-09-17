/*
 * CanNetbooting.cpp
 *
 */

#include <string.h>
#include "fsl_debug_console.h"
#include "netbooting.h"
#include "CanNetbooting.hpp"


#define CANNETBOOTING_DBG_PACKETS	(1)


NetbootingState CanNetbooting::state()
{
	return m_state;
}


ProgramFlash& CanNetbooting::flash() {
	return m_flash;
}


bool CanNetbooting::receiveCommand(Command &cmd)
{
	uint8_t chunk[8];
	bool bufferHasSpace;

	m_readBuffer.clear();

	do
	{
		size_t chunkBytes = receive(chunk, sizeof(chunk), 0);

		if (chunkBytes == 0) {
#if CANNETBOOTING_DBG_PACKETS
			PRINTF("(%s) Received EOF\r\n", __func__);
#endif
			break;
		}

#if CANNETBOOTING_DBG_PACKETS
		PRINTF("(%s) Received %d bytes\r\n", __func__, chunkBytes);
#endif

		for (size_t i=0; i<chunkBytes; i++) {
			bufferHasSpace = m_readBuffer.push(chunk[i]);
			if (!bufferHasSpace) {
				break;
			}
		}
	} while (bufferHasSpace);

	auto deserialize_status = cmd.deserialize(m_readBuffer);

	return (::EmbeddedProto::Error::NO_ERRORS == deserialize_status);
}


bool CanNetbooting::sendReply(Reply &reply)
{
	m_writeBuffer.clear();
	auto serialization_status = reply.serialize(m_writeBuffer);

	if(::EmbeddedProto::Error::NO_ERRORS == serialization_status)
	{
		uint8_t *data = m_writeBuffer.get_data();

		for (size_t i=0; i<m_writeBuffer.get_size(); i+=CanBus::PAYLOAD_SIZE) {
			size_t remaining_bytes = m_writeBuffer.get_size() - i;
	        size_t dlen = (remaining_bytes < CanBus::PAYLOAD_SIZE) ? remaining_bytes : CanBus::PAYLOAD_SIZE;
	        send(&data[i], dlen);
#if CANNETBOOTING_DBG_PACKETS
	        PRINTF("(%s) %d bytes sent\r\n", __func__, dlen);
#endif
		}

		// Send empty packet as EOF
		send(data, 0);
#if CANNETBOOTING_DBG_PACKETS
		PRINTF("(%s) EOF sent\r\n", __func__);
#endif
		return true;
	}

	return false;
}


void CanNetbooting::debugCommand(Command &cmd)
{
	PRINTF("(%s) Command received: acton\r\n", __func__);
	PRINTF("(%s) - action: %d\r\n", __func__, cmd.action());

	if (cmd.has_address()) {
		PRINTF("(%s) - address: 0x%x\r\n", __func__, cmd.address());
	}

	if (cmd.has_data()) {
		PRINTF("(%s) - data len: %d\r\n", __func__, cmd.data().len());
	}
}


NetbootingState CanNetbooting::waitingHook()
{
	Command command;

	if (!receiveCommand(command)) {
		PRINTF("(%s) Couldn't convert RX data into a command\r\n", __func__);
		return NetbootingState::WAITING_HOOK;
	}

	NetbootingState ret;
	Reply::Status status;

	if (command.action() == Action::HookUp) {
		status = Reply::Status::Succeed;
		ret = NetbootingState::HOOKED;
	} else {
		status = Reply::Status::Failed;
		ret = NetbootingState::WAITING_HOOK;
	}

	Reply reply;
	reply.set_action(command.action());
	reply.set_status(status);
	sendReply(reply);

	return ret;
}


NetbootingState CanNetbooting::hooked()
{
	Command command;

	if (!receiveCommand(command)) {
		PRINTF("(%s) Couldn't convert RX data into a command\r\n", __func__);
		return NetbootingState::HOOKED;
	}

	NetbootingState ret = NetbootingState::HOOKED;
	Reply::Status status;

	if (command.action() == Action::Jump) {
		status = Reply::Status::Succeed;
		ret = NetbootingState::JUMP;
	} else if (command.action() == Action::Quit) {
		status = Reply::Status::Succeed;
		ret = NetbootingState::QUIT;
	} else if (command.action() == Action::Erase) {
		PRINTF("(%s) Erasing 0x%x\r\n", __func__, command.address());
		if (m_flash.erase_sector(command.address())) {
			status = Reply::Status::Succeed;
		} else {
			status = Reply::Status::Failed;
		}
	} else if (command.action() == Action::Write) {
		PRINTF("(%s) Writting 0x%x\r\n", __func__, command.address());
		if (m_flash.write(command.address(), (uint32_t *)command.data().buf(), command.data().len())) {
			status = Reply::Status::Succeed;
		} else {
			status = Reply::Status::Failed;
		}
	} else {
		status = Reply::Status::Failed;
	}

	Reply reply;
	reply.set_action(command.action());
	reply.set_status(status);
	sendReply(reply);

	return ret;
}

typedef void (*FunctionPointer)(void);

void CanNetbooting::jump()
{
	PRINTF("(%s) Jumping to 0x%x (0x%x)\r\n", __func__, m_flash.startOffset(), m_flash.startAddress());

	close();
	__disable_irq();

	// Set the vector table base address to the application address
	SCB->VTOR = m_flash.startAddress();

	// Set the stack pointer to the application stack pointer
	uint32_t* stackPointer = (uint32_t*)m_flash.startAddress();
	__set_MSP(*stackPointer);

	// Set the program counter to the application reset handler
	FunctionPointer appResetHandler = (FunctionPointer)*(stackPointer + 1);
	appResetHandler();
}


void CanNetbooting::quit()
{
	PRINTF("(%s) Quiting netbooting, restarting..\r\n", __func__);
	NVIC_SystemReset();
}


NetbootingState CanNetbooting::process()
{
	switch(m_state)	{
		case NetbootingState::WAITING_HOOK: {
			m_state = waitingHook();
			break;
		}
		case NetbootingState::HOOKED: {
			m_state = hooked();
			break;
		}
		case NetbootingState::JUMP: {
			jump();
			break;
		}
		case NetbootingState::QUIT: {
			quit();
			break;
		}
		default: {
			m_state = NetbootingState::QUIT;
			break;
		}
	}

	return m_state;
}


const char* CanNetbooting::stateToStr(NetbootingState state)
{
	switch (state) {
		case NetbootingState::WAITING_HOOK:
			return "WAITING_HOOK";
		case NetbootingState::HOOKED:
			return "HOOKED";
		case NetbootingState::JUMP:
			return "JUMP";
		case NetbootingState::QUIT:
			return "QUIT";
		default:
			return "UNDEFINED";
	}
}

