---
name: data-search
description: Search for DataTables, DataAssets, or GameplayTags
---

Search Unreal Engine data using the UnrealDataBridge MCP tools.

## Catalog-First Approach

**Start with `get_data_catalog`** to understand the project's data layout before searching.
The catalog shows all tables (with `is_composite` flags), tag prefixes, and asset classes.
Use composite tables (`is_composite=true`) for broad content searches — they aggregate
rows from multiple source tables.

## Implementation Steps

1. **Call `get_data_catalog`** to discover available tables, tags, and assets.

2. **Identify search target type from catalog:**
   - Content in DataTables → `search_datatable_content` on the appropriate table
   - Rows by name pattern → `query_datatable` with `row_name_pattern`
   - Assets by name → `search_assets`
   - Tags by prefix → `list_gameplay_tags`

3. **Extract filters:**
   - Path filters for tables (e.g., `/Game/Data/`)
   - Class filters for assets (e.g., `DataAsset`)
   - Prefix filters for tags (e.g., `Item.`)
   - Query patterns for rows (e.g., `Sword*`)

4. **Load and call appropriate MCP tool:**
   - Use `ToolSearch` to load deferred tools before calling

5. **Format results:**
   - Tables: Name, Path, Row Count
   - Assets: Name, Class, Path
   - Tags: Tag Name, Comment
   - Rows: Row Name, Key Fields

## Usage Patterns

```
/data-search tables
/data-search tables /Game/Data/
/data-search assets Weapon
/data-search assets class:DataAsset
/data-search tags Item.
/data-search rows in /Game/Data/DT_Items.DT_Items matching Sword*
/data-search find "Punk Is Dead" in quests
```

## Notes

- Omit path filter to search all tables
- Asset search is case-insensitive substring match
- Tag search supports prefix matching only
- Row search supports wildcards: `*` (any chars), `?` (single char)
- Use `search_datatable_content` for content-based search (searches field values)
- Use `query_datatable` for row-name-based search (searches row keys)
