#!/usr/bin/env python3
"""
A simple Pymodbus TCP server simulator with enhanced logging and data decoding.
"""
from pymodbus.server import StartTcpServer
from pymodbus.datastore import ModbusSequentialDataBlock
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
import logging

# --- Configuration ---
HOST = "127.0.0.0"
PORT = 8080

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

# Create a custom data block that logs the decoded data on write operations
class LoggingModbusSequentialDataBlock(ModbusSequentialDataBlock):
    def setValues(self, address, values):
        """Overrides the setValues method to also log the decoded data."""
        super().setValues(address, values)
        
        try:
            # Decode the values to a string
            decoded_string = register_values_to_string(values)
            log.info(f"Decoded data received: '{decoded_string.strip()}'")
            # Added a new log message to explicitly show that a response is sent
            log.info("Response sent to the client confirming the write operation.")
        except Exception as e:
            log.error(f"Error decoding data: {e}")

# --- Initialize Data Store ---
# Create a block of 100 holding registers, initialized to 0.
# Address 0 corresponds to register 40001 in classic notation.
store = ModbusSlaveContext(hr=LoggingModbusSequentialDataBlock(0, [0] * 100))
context = ModbusServerContext(slaves=store, single=True)

# --- Run the server ---
log.info(f"Starting Modbus TCP server on {HOST}:{PORT}...")
StartTcpServer(context=context, address=(HOST, PORT))

