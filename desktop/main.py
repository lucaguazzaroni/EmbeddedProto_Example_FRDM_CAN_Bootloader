import argparse
import can
import subprocess
import time

from generated import netbooting_pb2 as pb


DEF_CAN_INTERFACE = 'can0'
DEF_CAN_BITRATE = 1000000
DEF_TARGET_CAN_ID = 0x801


class RemoteDeviceNotHookedError(Exception):
    def __init__(self, msg='Remote device is not hooked', *args, **kwargs):
        super().__init__(msg, *args, **kwargs)


class CanNetBooting:
    PAYLOAD_SIZE = 8
    FLASH_SECTOR_SIZE = 512
    FLASH_WRITE_MAX_SIZE = FLASH_SECTOR_SIZE // 2
    FLASH_AVAILABLE_SIZE = 0x10000

    def __init__(self, interface: str, bitrate: int, remote_id: int):
        self._interface = interface
        self._bitrate = bitrate
        self._remote_id = remote_id
        self._bus = None
        self._hooked_up = False

    def __enter__(self):
        if not self._can_interface_is_up():
            self._set_can_interface()

        self._bus = can.Bus(interface="socketcan", channel=self._interface, bitrate=self._bitrate)
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        self._bus.shutdown()
        if exc_type is not None:
            raise exc_type(exc_value)
        return True
    
    def _receive_packet(self, timeout : float = 1.0):
        if self._bus is None:
            return

        msg = self._bus.recv(timeout)
        if msg is None:
            raise TimeoutError(f"No message received after '{timeout}' secs")
        #print(type(msg), msg)
        return msg
    
    def _send_packet(self, payload: bytes):
        if len(payload) > self.PAYLOAD_SIZE:
            raise ValueError(f"CAN can send up to {self.PAYLOAD_SIZE} bytes")

        msg = can.Message(
            arbitration_id=self._remote_id,
            data=payload,
            is_extended_id=True,
            is_remote_frame=False
        )

        try:
            self._bus.send(msg)    
        except can.CanError as e:
            raise e
  
    def _can_interface_is_up(self):
        # Run 'ip -details link show {interface}' to get CAN interface details
        result = subprocess.run(['ip', '-details', 'link', 'show', self._interface], capture_output=True, text=True)
        
        if result.returncode != 0:
            raise RuntimeError(f"CAN interface '{self._interface}' is not available or not up. Please connect the USB to can device")

        # Check if the output contains the bitrate and state
        output = result.stdout

        if 'state UP' in output and f'bitrate {self._bitrate}' in output:
            print(f"CAN self._interface '{self._interface}' is already up with bitrate {self._bitrate}.")
            return True
        else:
            print(f"CAN interface '{self._interface}' is either down or set with a different bitrate.")
            return False

    def _set_can_interface(self):
        try:
            print(f"Setting up the '{self._interface}' interface with bitrate '{self._bitrate}'")
            # Define the command to set the CAN interface
            command = ['sudo', 'ip', 'link', 'set', self._interface, 'up', 'type', 'can', 'bitrate', str(self._bitrate)]
            subprocess.run(command, check=True)
            print(f"CAN interface '{self._interface}' set up successfully.")
        except subprocess.CalledProcessError as e:
            print(f"Failed to set up CAN interface: {e}")
            raise e
        
    def _receive_reply(self, timeout: float = 1.0):
        data = bytes()
        start = time.time()

        while time.time() - start < timeout:
            msg = self._receive_packet()
            
            if msg.arbitration_id != self._remote_id:
                continue

            # Empty message means EOF
            if msg.dlc == 0:
                break

            data += msg.data

        reply = pb.Reply()
        reply.ParseFromString(data)
        return reply
 
    def _send_command(self, command: pb.Command, timeout: float = 1.0):
        serialized_command = command.SerializeToString()
        for i in range(0, len(serialized_command), self.PAYLOAD_SIZE):
            chunk = serialized_command[i:i + self.PAYLOAD_SIZE]
            self._send_packet(chunk)
            time.sleep(0.02)

        # Send an empty message as EOF
        self._send_packet(bytes([]))
        time.sleep(0.02)

        reply = self._receive_reply(timeout=timeout)

        if reply.status != pb.Reply.Status.Succeed:
            command_str = str(command).replace("\n", "").replace("\r", "")
            raise RuntimeError(f"Command {command_str} failed")
        elif reply.action != command.action:
            raise RuntimeError(f"Action missmatch, received {reply.action} while expecting {command.action}")

        return reply

    def hook_up(self, attempts: int = 10, timeout: float = 1.0):
        command = pb.Command()
        command.action = pb.Action.HookUp

        for i in range(attempts):
            try:
                _ = cnb._send_command(command, timeout=timeout)
            except TimeoutError:
                print(f"Timeout for hook up in the {i+1} attempt")
                continue
            except RuntimeError as e:
                print(e)
                continue

            self._hooked_up = True
            break
        else:
            raise TimeoutError(f"Couldn't hook up after {attempts} attempts")

    def quit(self, timeout: float = 1.0):
        if not self._hooked_up:
            raise RemoteDeviceNotHookedError()
        
        command = pb.Command()
        command.action = pb.Action.Quit
        _ = cnb._send_command(command, timeout=timeout)
        self._hooked_up = False

    def jump(self, timeout: float = 1.0):
        if not self._hooked_up:
            raise RemoteDeviceNotHookedError()

        command = pb.Command()
        command.action = pb.Action.Jump
        _ = cnb._send_command(command, timeout=timeout)
        self._hooked_up = False

    def erase_sector(self, address: int, timeout: float = 1.0):
        if not self._hooked_up:
            raise RemoteDeviceNotHookedError()

        if address % self.FLASH_SECTOR_SIZE:
            address = address // self.FLASH_SECTOR_SIZE

        print("erasing 0x{}".format(hex(address)))
        command = pb.Command()
        command.action = pb.Action.Erase
        command.address = address
        _ = cnb._send_command(command, timeout=timeout)

    def write(self, address: int, data: bytes, timeout: float = 1.0):
        if not self._hooked_up:
            raise RemoteDeviceNotHookedError()
        
        data_len = len(data)
        if data_len > self.FLASH_WRITE_MAX_SIZE:
            raise ValueError("Data too long ({data_len}), max_size is {self.FLASH_WRITE_MAX_SIZE}")

        print("writting 0x{}".format(hex(address)))
        command = pb.Command()
        command.action = pb.Action.Write
        command.address = address
        command.data.len = data_len
        command.data.buf = data
        _ = cnb._send_command(command, timeout=timeout)

    def flash_binary(self, binary: bytes):
        binary_len = len(binary)
        if binary_len > self.FLASH_AVAILABLE_SIZE:
            raise ValueError(f"Binary too long ({binary_len}), max size is {self.FLASH_AVAILABLE_SIZE}")
        
        for address in range(0, binary_len, self.FLASH_WRITE_MAX_SIZE):
            if address % self.FLASH_SECTOR_SIZE == 0:
                self.erase_sector(address)

            data = binary[address:address + self.FLASH_WRITE_MAX_SIZE]
            self.write(address, data)


def commandline():
    parser = argparse.ArgumentParser(description='Set up CAN interface with a specific bitrate.')
    
    parser.add_argument('binary',
                        type=str,
                        help="Path to the firmware binary")
    
    parser.add_argument('-i', '--interface',
                        type=str, 
                        default=DEF_CAN_INTERFACE,
                        help=f'CAN interface to use (default: {DEF_CAN_INTERFACE})')
    
    parser.add_argument('-b', '--bitrate',
                        type=str,
                        default=DEF_CAN_BITRATE,
                        help=f'Bitrate to set on the CAN interface (default: {DEF_CAN_BITRATE})')
    
    parser.add_argument('-t', '--target-id',
                        type=str,
                        default=DEF_TARGET_CAN_ID,
                        help=f'Target device CAN ID (default: {DEF_TARGET_CAN_ID})')

    return parser.parse_args()


if __name__ == '__main__':
    args = commandline()

    with CanNetBooting(args.interface, args.bitrate, args.target_id) as cnb:
        print("Hooking up...")
        cnb.hook_up()
        print("Device hooked")

        try:
            with open(args.binary, 'rb') as f:
                binary_data = f.read()
                
            print(f"Starting to flash {args.binary} ({len(binary_data)} bytes)...")
            cnb.flash_binary(binary_data)
            print(f"{args.binary} Flashed")

            print("Jumping to the app...")
            cnb.jump()
            print("Bootloader jumped!")

        except KeyboardInterrupt:
            print("Quiting the net booting...")
            cnb.quit()
            print("Quit succeed")
