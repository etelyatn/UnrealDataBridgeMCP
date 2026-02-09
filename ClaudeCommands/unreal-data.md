---
name: unreal-data
description: Query and manipulate Unreal Engine data via natural language
---

Use the available UnrealDataBridge MCP tools to fulfill the user's request.

## Context-First Strategy

**Check conversation context before calling any discovery tools.** If you already know
the table paths, row structs, or field names from earlier in the conversation, go directly
to the data operation — don't waste context re-fetching the catalog.

Only call discovery tools when you genuinely don't know what exists:
- `get_data_catalog` — when you need a broad overview of ALL project data
- `list_datatables` — when you just need table paths
- `get_datatable_schema` — when you need full field type details

## Implementation Steps

1. **Check what you already know** from conversation context:
   - Do you already know the table path? → Skip discovery
   - Do you know the field names? → Skip schema calls
   - If you know nothing, call `get_data_catalog` (cached 10 min)

2. **Map to MCP tool calls:**
   - "find/search [text] in [table]" → `search_datatable_content`
   - "get table X row Y" → `get_datatable_row`
   - "get rows A, B, C from table X" → `query_datatable` with `row_names`
   - "list tables [path]" → `list_datatables`
   - "show schema for table X" → `get_datatable_schema`
   - "search assets [query]" → `search_assets`
   - "list tags [prefix]" → `list_gameplay_tags`
   - "resolve tags against table" → `resolve_tags`

3. **Use `fields` param** on `query_datatable` to select only needed fields.
   This skips serialization of heavy nested data and keeps responses small.

   **Use `row_names` param** on `query_datatable` to fetch specific rows by name
   in one call instead of multiple `get_datatable_row` calls.

4. **For cross-table lookups**, use `batch_query` to combine multiple commands
   in one round-trip. Example: fetch a quest row, then resolve its patient tags
   and product tags in one batch call.

5. **For tag-based joins**, use `resolve_tags` to find rows in a target table
   whose GameplayTag field matches given tags. This replaces manual iteration.

6. **Load tools before calling:**
   - Use `ToolSearch` to load any deferred MCP tools before using them

7. **Format the response** for readability:
   - Use tables for structured data
   - Use lists for collections
   - Highlight important values

## Common Table Paths

If the user mentions "quests", the composite table is likely `CQT_Quests`.
Use `search_datatable_content` on it directly without catalog lookup.

## Error Handling

- If table path is missing `.DT_` extension, suggest adding it
- If row not found, list similar row names
- If no results, suggest broader search terms
- Always use full asset paths: `/Game/Path/To/Asset.Asset`
- If cache seems stale, call `refresh_cache` to force fresh data
