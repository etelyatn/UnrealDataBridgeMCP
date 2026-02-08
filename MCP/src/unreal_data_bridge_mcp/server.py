"""MCP server exposing Unreal Engine data systems to AI tools."""

import json
import logging
import os
from mcp.server.fastmcp import FastMCP
from .tcp_client import UEConnection
from .tools.datatables import register_datatable_tools

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

mcp = FastMCP("unreal-data-bridge")

_connection = UEConnection(
    host=os.environ.get("UDB_HOST", "127.0.0.1"),
    port=int(os.environ.get("UDB_PORT", "8742")),
)


@mcp.tool()
def get_status() -> str:
    """Check connection status to Unreal Editor and get plugin/project info.

    Returns connection status, plugin version, engine version, and project name.
    Use this to verify the bridge is working before calling other tools.
    """
    try:
        response = _connection.send_command("get_status")
        return json.dumps(response.get("data", {}), indent=2)
    except ConnectionError as e:
        return f"Not connected to Unreal Editor: {e}"


# Register tool groups
register_datatable_tools(mcp, _connection)


def main():
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()
