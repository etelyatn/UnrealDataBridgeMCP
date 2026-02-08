"""MCP tools for DataTable read operations."""

import json
import logging
from ..tcp_client import UEConnection

logger = logging.getLogger(__name__)


def register_datatable_tools(mcp, connection: UEConnection):
    """Register all DataTable-related MCP tools."""

    @mcp.tool()
    def list_datatables(path_filter: str = "") -> str:
        """List all DataTables currently loaded in the Unreal Editor.

        Returns each table's name, asset path, row struct type, and row count.
        Use this to discover available DataTables before querying their contents.

        Args:
            path_filter: Optional prefix filter for asset paths (e.g., '/Game/Data/Quests/').
                         Only returns tables whose path starts with this prefix.

        Returns:
            JSON with 'datatables' array, each containing:
            - name: Asset name (e.g., 'DT_QuestDefinitions')
            - path: Full asset path (e.g., '/Game/Data/Quests/DT_QuestDefinitions.DT_QuestDefinitions')
            - row_struct: Name of the row struct type (e.g., 'FRipQuestDefinition')
            - row_count: Number of rows in the table
        """
        try:
            response = connection.send_command("list_datatables", {"path_filter": path_filter})
            return json.dumps(response.get("data", {}), indent=2)
        except ConnectionError as e:
            return f"Error: {e}"

    @mcp.tool()
    def get_datatable_schema(table_path: str, include_inherited: bool = True) -> str:
        """Get the row struct schema for a DataTable, showing all fields, types, and constraints.

        Call this before reading or writing DataTable rows to understand the row structure.
        For TInstancedStruct fields, this shows the base struct and known subtypes.

        Args:
            table_path: Full asset path to the DataTable (e.g., '/Game/Data/Quests/DT_QuestDefinitions.DT_QuestDefinitions').
                        Use list_datatables to discover available paths.
            include_inherited: Whether to include fields inherited from parent structs (default: True).

        Returns:
            JSON with 'schema' containing field definitions, each with:
            - name: Field name
            - type: Human-readable type (e.g., 'int32', 'FString', 'TArray<FGameplayTag>')
            - cpp_type: C++ type name
            - enum_values: For enum fields, array of valid string values
            - element_type: For array fields, the element type schema
            - fields: For struct fields, nested field schemas
        """
        try:
            response = connection.send_command("get_datatable_schema", {
                "table_path": table_path,
                "include_inherited": include_inherited,
            })
            return json.dumps(response.get("data", {}), indent=2)
        except ConnectionError as e:
            return f"Error: {e}"

    @mcp.tool()
    def query_datatable(
        table_path: str,
        row_name_pattern: str = "",
        fields: str = "",
        limit: int = 100,
        offset: int = 0,
    ) -> str:
        """Query rows from a DataTable with optional filtering, field selection, and pagination.

        Use get_datatable_schema first to understand the row structure.

        Args:
            table_path: Full asset path to the DataTable.
            row_name_pattern: Optional wildcard pattern to filter row names (e.g., 'Quest_*', '*_Boss').
                              Uses Unreal wildcard matching (* for any chars, ? for single char).
            fields: Optional comma-separated list of field names to include in results.
                    Leave empty to include all fields. Example: 'QuestName,Difficulty,Rewards'.
            limit: Maximum number of rows to return (default: 100).
            offset: Number of rows to skip for pagination (default: 0).

        Returns:
            JSON with:
            - rows: Array of {row_name, row_data} objects
            - total_count: Total matching rows (before pagination)
            - offset: Applied offset
            - limit: Applied limit
        """
        try:
            params = {
                "table_path": table_path,
                "limit": limit,
                "offset": offset,
            }
            if row_name_pattern:
                params["row_name_pattern"] = row_name_pattern
            if fields:
                params["fields"] = [f.strip() for f in fields.split(",")]
            response = connection.send_command("query_datatable", params)
            return json.dumps(response.get("data", {}), indent=2)
        except ConnectionError as e:
            return f"Error: {e}"

    @mcp.tool()
    def get_datatable_row(table_path: str, row_name: str) -> str:
        """Get a specific row from a DataTable by its row name.

        Use list_datatables to find available tables, and query_datatable to discover row names.

        Args:
            table_path: Full asset path to the DataTable.
            row_name: The row name/key to look up (e.g., 'Quest_Tutorial_01').

        Returns:
            JSON with:
            - row_name: The requested row name
            - row_struct: The struct type name
            - row_data: Object with all row field values
        """
        try:
            response = connection.send_command("get_datatable_row", {
                "table_path": table_path,
                "row_name": row_name,
            })
            return json.dumps(response.get("data", {}), indent=2)
        except ConnectionError as e:
            return f"Error: {e}"

    @mcp.tool()
    def get_struct_schema(struct_name: str, include_subtypes: bool = False) -> str:
        """Get the schema for any UStruct type by name, showing fields, types, and constraints.

        Use this to understand struct types used in DataTable rows, especially for
        TInstancedStruct fields that can hold different subtypes.

        Args:
            struct_name: Name of the struct (e.g., 'FRipQuestDefinition', 'FGameplayTag').
                         The 'F' prefix is optional — both 'RipQuestDefinition' and
                         'FRipQuestDefinition' will work.
            include_subtypes: For TInstancedStruct base types, include a list of known
                              subtypes and their schemas (default: False).

        Returns:
            JSON with 'schema' containing field definitions, and optionally 'subtypes' array.
        """
        try:
            response = connection.send_command("get_struct_schema", {
                "struct_name": struct_name,
                "include_subtypes": include_subtypes,
            })
            return json.dumps(response.get("data", {}), indent=2)
        except ConnectionError as e:
            return f"Error: {e}"
