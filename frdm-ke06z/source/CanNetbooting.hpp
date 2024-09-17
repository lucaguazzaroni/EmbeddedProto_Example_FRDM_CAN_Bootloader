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
