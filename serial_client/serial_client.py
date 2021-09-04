import time
import numpy as np

import serial
import serial.tools.list_ports as portlist

CMD_HANDSHAKE = 0
CMD_ACK = 1
CMD_NACK = 2
CMD_STREAM = 3
CMD_RESET = 4
CMD_SETUP = 5

class SerialUSB():

    def __init__(self, port, config, baudrate=115200, flipdata=False):
        self.device = None
        self.port = port
        self.config = config
        self.baudrate = baudrate
        self.flipdata = flipdata
    
    def connect(self, con_evt, timeout: int = 1) -> bool:
        try:
            with serial.Serial(port=self.port, baudrate=self.baudrate, timeout=timeout) as device:
                self.device = device

                time.sleep(0.3) # wait a bit to establish a connection
                print(f"Connected")

                # Close and reopen
                device.close()
                device.open()

                # Chill out while everything gets set
                time.sleep(0.4)

                # Set a long timeout to complete handshake
                timeout = device.timeout
                device.timeout = 2

                # Call event
                con_evt(self)

                # Reset the timeout
                device.timeout = timeout

                return True
        except:
            return False
    
    def __command(self, cmd) -> bool:
        # Send request to device
        self.device.write(bytes(cmd))

        # Wait until there is data
        while self.device.inWaiting() == 0: pass

        # Read in what the device sent
        msg = self.device.read()
        ret = -1
        try:
            ret = ord(msg)
        except:
            ok = False

        # Verify returned message
        ok = ret == CMD_ACK
        if not ok:
            print("> Command '{cmd}' NOT ok: msg={msg} {type_msg}, ret={ret} {type_ret}".format(
                cmd=np.squeeze(cmd), msg=msg, type_msg=type(msg), ret=ret, type_ret=type(ret)))

        return ok

    def handshake(self) -> bool:
        """Make sure connection is established by sending and receiving bytes."""

        # Read and discard everything that may be in the input buffer
        _ = self.device.read_all()

        return self.__command([CMD_HANDSHAKE])
    
    def setup(self, config=None) -> bool:
        if config is None: config = self.config
        return self.__command([CMD_SETUP]) and self.__command(config)

    def reset(self) -> bool:
        return self.__command([CMD_RESET])

    def stream(self, data) -> bool:
        if self.flipdata: data = np.flip(data)

        ok = self.__command([CMD_STREAM]) and self.__command(data)

        #while self.device.inWaiting() == 0: pass
        #byt = self.device.readline()
        #print("in", " ".join(str(c) for c in byt[0:len(byt)-2]))
        
        return ok
    
    @staticmethod
    def find_port():
        """ Get the name of the port that is connected to Arduino. """

        port = None
        for p in portlist.comports():
            if p.manufacturer is not None and "Arduino" in p.manufacturer:
                port = p.device
        
        return port