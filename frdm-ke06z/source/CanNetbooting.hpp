/*
 * CanNetbooting.hpp
 *
 */

#ifndef CANNETBOOTING_HPP_
#define CANNETBOOTING_HPP_

#include "netbooting.h"
#include "ReadBufferFixedSize.h"
#include "WriteBufferFixedSize.h"
#include "CanBus.hpp"
#include "ProgramFlash.hpp"


#define BUF_SIZE		(256 + 48)


enum class NetbootingState {
	UNDEFINED,
	WAITING_HOOK,
	HOOKED,
	JUMP,
	QUIT,
};


class CanNetbooting : public CanBus
{
public:
	CanNetbooting(uint32_t nodeId, uint32_t baudRate, uint32_t jump_address) :
		CanBus(nodeId, baudRate), m_flash(jump_address) {}

	NetbootingState process();
	NetbootingState state();
	ProgramFlash& flash();
	void jump();

	static const char* stateToStr(NetbootingState state);

private:
	bool receiveCommand(Command &cmd);
	bool sendReply(Reply &reply);

	void debugCommand(Command &cmd);

	NetbootingState waitingHook();
	NetbootingState hooked();
	void quit();

	uint32_t const *sysTicks;
	ProgramFlash m_flash;
	NetbootingState m_state = NetbootingState::WAITING_HOOK;

	::EmbeddedProto::ReadBufferFixedSize<BUF_SIZE> m_readBuffer;
	::EmbeddedProto::WriteBufferFixedSize<BUF_SIZE> m_writeBuffer;
};


#endif /* CANNETBOOTING_HPP_ */
