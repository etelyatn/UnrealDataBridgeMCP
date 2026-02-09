---
name: data-search
description: Search for DataTables, DataAssets, or GameplayTags
---

Search Unreal Engine data using the UnrealDataBridge MCP tools.

## Context-First Approach

**Check conversation context first.** If you already know the table paths from earlier
in the conversation, skip discovery and go directly to the search tool.
Only call `get_data_catalog` if you genuinely don't know what tables/assets exist.

## Implementation Steps

1. **Check what you already know:**
   - Table path known? → Go directly to step 3
   - Unknown? → Call `get_data_catalog` to discover tables, tags, and assets

2. **Identify search target type:**
   - Content in DataTables → `search_datatable_content` on the appropriate table
   - Rows by name pattern → `query_datatable` with `row_name_pattern`
   - Specific rows by name → `query_datatable` with `row_names` (comma-separated)
   - Rows by GameplayTag → `resolve_tags` against the target table's tag field
   - Assets by name → `search_assets`
   - Tags by prefix → `list_gameplay_tags`

3. **Use `fields` param** to keep responses small — only request fields you need.

4. **For multi-step lookups**, use `batch_query` to combine commands in one call.
   Example: search a quest table, then resolve its patient tags against the patient table.

5. **Load and call appropriate MCP tool:**
   - Use `ToolSearch` to load deferred tools before calling

6. **Format results:**
   - Tables: Name, Path, Row Count
   - Assets: Name, Class, Path
   - Tags: Tag Name, Comment
   - Rows: Row Name, Key Fields

## Usage Patterns

```
/data-search tables
/data-search tables /Game/Data/
/data-search assets Weapon
/data-search tags Item.
/data-search find "Punk Is Dead" in quests
/data-search rows Quest_Tutorial_01, Quest_Build_01 from quests with fields Title,QuestType
/data-search resolve tags Patient.NPC.Maria,Patient.NPC.Viktor against patient table
```

## Notes

- Prefer composite tables (e.g., `CQT_Quests`) for broad content searches
- Asset search is case-insensitive substring match
- Tag search supports prefix matching only
- Row search supports wildcards: `*` (any chars), `?` (single char)
