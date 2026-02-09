---
name: unreal-data
description: Query and manipulate Unreal Engine data via natural language
---

Use the available UnrealDataBridge MCP tools to fulfill the user's request.

## Catalog-First Strategy

**Always start with `get_data_catalog`** — it returns a cached overview of ALL project data
(DataTables with top field names, GameplayTag prefixes, DataAsset classes, StringTables)
in a single call. Use it to plan targeted queries without exploratory discovery.

## Implementation Steps

1. **Call `get_data_catalog` first** (cached for 10 minutes, near-instant on repeat calls):
   - Identifies which tables exist and their row structs
   - Shows composite tables (`is_composite=true`) — prefer these for broad searches
   - Lists top field names per table (eliminates separate schema calls for discovery)
   - Shows tag prefix categories and data asset classes

2. **Use the catalog to plan targeted queries:**
   - Know the exact table path before calling `search_datatable_content` or `query_datatable`
   - Use `top_fields` to understand table structure without calling `get_datatable_schema`
   - Only call `get_datatable_schema` when you need full field details (types, enums, nested structs)

3. **Map to MCP tool calls:**
   - "get status" → `get_status`
   - "find/search [text] in [table]" → `search_datatable_content` (use catalog to find the right table)
   - "get table X row Y" → `get_datatable_row`
   - "list tables [path]" → `list_datatables` (or just read from catalog)
   - "show schema for table X" → `get_datatable_schema`
   - "search assets [query]" → `search_assets`
   - "list tags [prefix]" → `list_gameplay_tags`

4. **Load tools before calling:**
   - Use `ToolSearch` to load any deferred MCP tools before using them

5. **Format the response** for readability:
   - Use tables for structured data
   - Use lists for collections
   - Highlight important values

## Usage Examples

```
/unreal-data get status
/unreal-data find quest "Punk Is Dead"
/unreal-data get table /Game/Data/DT_Items.DT_Items row Item_Sword_01
/unreal-data list tables in /Game/Data/
/unreal-data show schema for table /Game/Data/DT_Items.DT_Items
/unreal-data search assets containing "Weapon"
/unreal-data list tags starting with "Item."
```

## Error Handling

- If table path is missing `.DT_` extension, suggest adding it
- If row not found, list similar row names
- If no results, suggest broader search terms
- Always use full asset paths: `/Game/Path/To/Asset.Asset`
- If cache seems stale, call `refresh_cache` to force fresh data
