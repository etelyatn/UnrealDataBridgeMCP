"""MCP server exposing Unreal Engine data systems to AI tools."""

import json
import logging
import os
import sys
from mcp.server.fastmcp import FastMCP
from .tcp_client import UEConnection
from .tools.datatables import register_datatable_tools
from .tools.gameplay_tags import register_gameplay_tag_tools
from .tools.data_assets import register_data_asset_tools
from .tools.localization import register_localization_tools
from .tools.assets import register_asset_tools

_log_level = getattr(logging, os.environ.get("UDB_LOG_LEVEL", "INFO").upper(), logging.INFO)
logging.basicConfig(
    level=_log_level,
    stream=sys.stderr,
    format="%(levelname)s:%(name)s:%(message)s",
)
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
register_gameplay_tag_tools(mcp, _connection)
register_data_asset_tools(mcp, _connection)
register_localization_tools(mcp, _connection)
register_asset_tools(mcp, _connection)


def main():
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()
