#!/usr/bin/env python3
"""
A Pymodbus TCP server designed to work with a C++ client,
supporting all four standard register types for read requests.
"""
from pymodbus.server import StartAsyncTcpServer
from pymodbus.datastore import (
    ModbusSequentialDataBlock,
    ModbusSlaveContext,
    ModbusServerContext,
)
import logging
import asyncio
import sys

# --- Configuration ---
HOST = "localhost"
PORT = 8080

# --- Logging Setup ---
logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
    level=logging.DEBUG
)
log = logging.getLogger(__name__)

# --- Helper Functions ---
def register_values_to_string(values):
    """Converts a list of 16-bit register values to a human-readable string."""
    byte_list = []
    for reg_value in values:
        byte_list.append((reg_value >> 8) & 0xFF)
        byte_list.append(reg_value & 0xFF)
    return bytes(byte_list).decode('ascii', errors='ignore')

# --- Custom Data Block for Logging Reads ---
class LoggingModbusDataBlock(ModbusSequentialDataBlock):
    """A data block that logs data being read or written."""
    
    def getValues(self, address, count):
        """Overrides getValues to log the data being returned to the client."""
        values = super().getValues(address, count)
        log.info(
            f"Client requested to read {count} entries starting at address {address}. "
            f"Returning data: {values}."
        )
        return values
        
    def setValues(self, address, values):
        """Overrides setValues to log the data being written by the client."""
        super().setValues(address, values)
        try:
            decoded_string = register_values_to_string(values)
            log.info(f"Client wrote '{decoded_string.strip()}' to address {address}.")
        except Exception as e:
            log.error(f"Error decoding data from client: {e}")

# --- Initialize Data Store ---
# Each register bank is initialized with distinct values to help with testing.
store = ModbusSlaveContext(
    # Coils (0xxxx): a series of boolean values
    co=LoggingModbusDataBlock(0, [1, 0, 1, 1, 0] * 10),
    # Discrete Inputs (1xxxx): read-only booleans
    di=LoggingModbusDataBlock(0, [0, 1, 0, 1, 1] * 10),
    # Input Registers (3xxxx): read-only 16-bit values
    ir=LoggingModbusDataBlock(0, [100, 200, 300, 400] * 25),
    # Holding Registers (4xxxx): read/write 16-bit values
    hr=LoggingModbusDataBlock(0, [11, 22, 33, 44] * 25),
)

context = ModbusServerContext(slaves=store, single=True)

# --- Server Start/Stop Logic ---
async def run_server():
    """Runs the Modbus TCP server with robust error handling."""
    log.info(f"Starting Modbus TCP server on {HOST}:{PORT}...")
    server = await StartAsyncTcpServer(
        context=context,
        address=(HOST, PORT)
    )
    log.info("Server is now ready to accept connections.")
    return server

if __name__ == "__main__":
    server = None
    try:
        loop = asyncio.get_event_loop()
        server_task = loop.create_task(run_server())
        loop.run_until_complete(server_task)
        server = server_task.result()
        loop.run_until_complete(server.serve_forever())
    except KeyboardInterrupt:
        log.info("Server is shutting down due to keyboard interrupt.")
        if server:
            loop.run_until_complete(server.shutdown())
    except Exception as e:
        log.error(f"An unexpected error occurred: {e}")
    finally:
        if 'loop' in locals() and not loop.is_closed():
            loop.close()

