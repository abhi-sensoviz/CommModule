#!/usr/bin/env python3
"""
A simple Pymodbus RTU server simulator with enhanced logging and data decoding.
"""
from pymodbus.server import StartAsyncSerialServer
from pymodbus.datastore import ModbusSequentialDataBlock
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
import logging
import asyncio
import sys

# --- Configuration ---
# Set your serial port and baudrate here.
# For testing with socat, use one of the virtual port names.
SERIAL_PORT = "/dev/pts/2"
BAUDRATE = 9600

# Configure logging to show all debug messages from the server and our script
logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
    level=logging.DEBUG
)

# Get a logger for the current script
log = logging.getLogger(__name__)

# The pymodbus library has its own loggers, let's set them to DEBUG too
modbus_log = logging.getLogger("pymodbus.server")
modbus_log.setLevel(logging.DEBUG)

def register_values_to_string(values):
    """Converts a list of 16-bit register values to a human-readable string."""
    byte_list = []
    for reg_value in values:
        # Each register is a 16-bit word, convert it to two bytes
        byte_list.append((reg_value >> 8) & 0xFF)
        byte_list.append(reg_value & 0xFF)
    # Decode the byte list to an ASCII string, ignoring any non-decodable characters
    return bytes(byte_list).decode('ascii', errors='ignore')

# Create a custom data block that logs the decoded data on write and read operations
class LoggingModbusSequentialDataBlock(ModbusSequentialDataBlock):
    def getValues(self, address, count):
        """Overrides getValues to log the data being returned to the client."""
        values = super().getValues(address, count)
        log.info(
            f"Client requested to read {count} entries starting at address {address}. "
            f"Returning data: {values}."
        )
        return values
        
    def setValues(self, address, values):
        """Overrides the setValues method to also log the decoded data."""
        super().setValues(address, values)
        
        try:
            # Decode the values to a string
            decoded_string = register_values_to_string(values)
            log.info(f"Decoded data received: '{decoded_string.strip()}'")
            log.info("Response sent to the client confirming the write operation.")
        except Exception as e:
            log.error(f"Error decoding data: {e}")

# --- Initialize Data Store ---
# Create a block of 100 holding registers with some initial values.
# Address 0 corresponds to register 40001 in classic notation.
initial_data = [11, 22, 33, 44, 55] * 20
store = ModbusSlaveContext(hr=LoggingModbusSequentialDataBlock(0, initial_data))
context = ModbusServerContext(slaves=store, single=True)

# --- Run the server ---
async def run_server():
    """Runs the Modbus RTU server with robust error handling."""
    log.info(f"Starting Modbus RTU server on {SERIAL_PORT}:{BAUDRATE}...")
    server = await StartAsyncSerialServer(
        context=context,
        port=SERIAL_PORT,
        baudrate=BAUDRATE
    )
    log.info("Server is now ready to accept connections.")
    await server.serve_forever()

if __name__ == "__main__":
    try:
        # Try to run the server in the main thread
        asyncio.run(run_server())
    except Exception as e:
        log.error(f"An unexpected error occurred: {e}")

