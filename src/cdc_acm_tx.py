from dataclasses import dataclass
from serial import Serial, SerialException
from serial.tools.list_ports import comports
import logging
import signal
import time

@dataclass
class SnifferPacket:
    content: bytes
    timestamp: int
    lqi: int
    rssi: int

class Nrf802154Sniffer:
    NORDICSEMI_VID = 0x1915
    SNIFFER_802154_PID = 0x154B

    def __init__(self):
        self.logger = logging.getLogger(__name__)
        self.dev = None
        self.channel = 11
        self.thread = None
        self.running = True

    def start_sniffing(self, port: str):
        try:
            serial = Serial(port, exclusive=True, timeout=0.1)
            serial.write(b"sleep\r\n")
            serial.write(b"channel " + str(self.channel).encode() + b"\r\n")
            serial.write(b"receive\r\n")
            serial.flush()
            
            print(f"Started sniffing on channel {self.channel}")
            print("Press Ctrl+C to stop")

            while self.running:
                try:
                    line = serial.readline()
                    if line:
                        print(line.decode('utf-8', errors='replace').strip())
                except SerialException as e:
                    self.logger.error(f"Serial error: {e}")
                    break

        except SerialException as e:
            self.logger.error(f"Failed to open port {port}: {e}")
        finally:
            if 'serial' in locals():
                serial.close()

    def stop_sniffing(self):
        self.running = False
        if self.thread:
            self.thread.join()

    @staticmethod
    def find_sniffer():
        for port in comports():
            if (port.vid == Nrf802154Sniffer.NORDICSEMI_VID and 
                port.pid == Nrf802154Sniffer.SNIFFER_802154_PID):
                return port.device
        return None

def main():
    logging.basicConfig(level=logging.INFO)
    
    sniffer = Nrf802154Sniffer()
    port = sniffer.find_sniffer()
    
    if not port:
        print("No sniffer device found!")
        return

    def signal_handler(sig, frame):
        sniffer.stop_sniffing()

    signal.signal(signal.SIGINT, signal_handler)
    sniffer.start_sniffing(port)

if __name__ == "__main__":
    main()