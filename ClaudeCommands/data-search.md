---
name: data-search
description: Search for DataTables, DataAssets, or GameplayTags
---

Search Unreal Engine data using the UnrealDataBridge MCP tools.

## Implementation Steps

1. **Identify search target type:**
   - `tables [path_filter]` → list_datatables
   - `assets [query] [class_filter]` → search_assets
   - `tags [prefix]` → list_gameplay_tags
   - `rows in table X matching Y` → query_datatable

2. **Extract filters:**
   - Path filters for tables (e.g., `/Game/Data/`)
   - Class filters for assets (e.g., `DataAsset`)
   - Prefix filters for tags (e.g., `Item.`)
   - Query patterns for rows (e.g., `Sword*`)

3. **Load and call appropriate MCP tool:**
   - Use `ToolSearch` to load deferred tools before calling
   - Pass filters as parameters

4. **Format results:**
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
```

## Notes

- Omit path filter to search all tables
- Asset search is case-insensitive substring match
- Tag search supports prefix matching only
- Row search supports wildcards: `*` (any chars), `?` (single char)
