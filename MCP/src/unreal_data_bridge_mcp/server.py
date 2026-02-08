"""MCP server exposing Unreal Engine data systems to AI tools."""

import logging
import os
from mcp.server.fastmcp import FastMCP
from .tcp_client import UEConnection

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
        return str(response.get("data", {}))
    except ConnectionError as e:
        return f"Not connected to Unreal Editor: {e}"


@mcp.tool()
def list_datatables(path_filter: str = "") -> str:
    """List all DataTables in the Unreal project with their row struct types.

    Args:
        path_filter: Filter by content path prefix (e.g., '/Game/Data/Quests/')
    """
    try:
        response = _connection.send_command("list_datatables", {"path_filter": path_filter})
        return str(response.get("data", {}))
    except ConnectionError as e:
        return f"Not connected to Unreal Editor: {e}"


def main():
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()
