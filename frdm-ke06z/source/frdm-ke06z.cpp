/*
 * Copyright 2016-2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    frdm-ke06z.cpp
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
/* TODO: insert other include files here. */
#include <CanNetbooting.hpp>
/* TODO: insert other definitions and declarations here. */

#define CAN_NODE_ID     	0x801
#define CAN_BAUDRATE    	1000000U
#define JUMP_ADDRESS		0x10000

/*
 * @brief   Application entry point.
 */
int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    auto nb = CanNetbooting(CAN_NODE_ID, CAN_BAUDRATE, JUMP_ADDRESS);

    PRINTF("(%s) Opening CAN interface\r\n", __func__);
    nb.open();

    PRINTF("(%s) Opening Flash driver\r\n", __func__);
    nb.flash().open();
    PRINTF("(%s) - Start address 0x%x \r\n", __func__, nb.flash().startAddress());
    PRINTF("(%s) - Sector size %d \r\n", __func__, nb.flash().sectorSize());

    auto lastState = nb.state();
    PRINTF("(%s) Netbooting state: %s\r\n", __func__, CanNetbooting::stateToStr(lastState));

    while(1)
    {
    	auto state = nb.process();
    	if (state != lastState) {
    		PRINTF("(%s) Netbooting state changed from '%s' to '%s'\r\n",
    				__func__,
    				CanNetbooting::stateToStr(lastState),
					CanNetbooting::stateToStr(state));

    		if (state == NetbootingState::HOOKED) {
    			LED_GREEN1_ON();
    		} else if (state == NetbootingState::JUMP) {
    			LED_GREEN1_OFF();
    		}

    		lastState = state;
    	}
    }

    /* Force the counter to be placed into memory. */
    volatile static int i = 0 ;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
        i++ ;
        /* 'Dummy' NOP to allow source level single stepping of
            tight while() loop */
        __asm volatile ("nop");
    }
    return 0 ;
}
