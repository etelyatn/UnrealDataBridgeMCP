---
name: unreal-data
description: Query and manipulate Unreal Engine data via natural language
---

Use the available UnrealDataBridge MCP tools to fulfill the user's request.

## Implementation Steps

1. **Parse the natural language query** to identify:
   - Operation: get, list, show, search, describe, status
   - Target: table, row, asset, tag, schema
   - Identifiers: table paths, row names, search queries

2. **Map to MCP tool calls:**
   - "get status" → `mcp__unreal-data-bridge__get_status`
   - "get table X row Y" → `mcp__unreal-data-bridge__get_datatable_row`
   - "list tables [path]" → `mcp__unreal-data-bridge__list_datatables`
   - "show schema for table X" → `mcp__unreal-data-bridge__get_datatable_schema`
   - "search assets [query]" → `mcp__unreal-data-bridge__search_assets`
   - "list tags [prefix]" → `mcp__unreal-data-bridge__list_gameplay_tags`
   - "validate tag X" → `mcp__unreal-data-bridge__validate_gameplay_tag`

3. **Load tools before calling:**
   - Use `ToolSearch` to load any deferred MCP tools before using them
   - Example: `ToolSearch` with query "select:mcp__unreal-data-bridge__get_status"

4. **Format the response** for readability:
   - Use tables for structured data
   - Use lists for collections
   - Highlight important values

## Usage Examples

```
/unreal-data get status
/unreal-data get table /Game/Data/DT_Items.DT_Items row Item_Sword_01
/unreal-data list tables in /Game/Data/
/unreal-data show schema for table /Game/Data/DT_Items.DT_Items
/unreal-data search assets containing "Weapon"
/unreal-data list tags starting with "Item."
/unreal-data validate tag Item.Weapon.Sword
```

## Error Handling

- If table path is missing `.DT_` extension, suggest adding it
- If row not found, list similar row names
- If no results, suggest broader search terms
- Always use full asset paths: `/Game/Path/To/Asset.Asset`
