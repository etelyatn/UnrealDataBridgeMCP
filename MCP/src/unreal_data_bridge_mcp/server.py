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

_TTL_CATALOG = 600  # 10 min


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


@mcp.tool()
def get_data_catalog() -> str:
    """CALL THIS FIRST before any other data operations.

    Returns a compact overview of ALL project data in a single call:
    DataTables (with top field names), GameplayTag prefixes, DataAsset classes,
    and StringTables. Use this to understand the project's data structure and
    plan targeted queries without exploratory discovery calls.

    The catalog is cached for 10 minutes. Use refresh_cache if data has changed
    externally (e.g., new tables created in editor, C++ recompile).

    Returns:
        JSON with:
        - datatables: Array of {name, path, row_struct, row_count, is_composite,
          parent_tables, top_fields} for each loaded DataTable
        - tag_prefixes: Array of {prefix, count} for GameplayTag top-level categories
        - data_asset_classes: Array of {class_name, count, example_path} grouped by class
        - string_tables: Array of {name, path, entry_count} for each StringTable
    """
    try:
        response = _connection.send_command_cached(
            "get_data_catalog", {}, ttl=_TTL_CATALOG
        )
        return json.dumps(response.get("data", {}), indent=2)
    except ConnectionError as e:
        return f"Error: {e}"


@mcp.tool()
def refresh_cache() -> str:
    """Clear all cached data and force fresh reads from Unreal Editor.

    Use this when you know data has changed outside of MCP tools
    (e.g., new DataTables created in editor, C++ structs recompiled,
    assets imported). The cache also auto-clears on editor reconnect.

    Returns:
        JSON with cache stats before clearing.
    """
    stats = _connection._cache.stats
    _connection.invalidate_cache(None)
    return json.dumps({"cleared": True, "previous_stats": stats})


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
