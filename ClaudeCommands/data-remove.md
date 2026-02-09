---
name: data-remove
description: Remove DataTable rows
---

Remove data entries from Unreal Engine DataTables.

## Implementation Steps

1. **Parse table path and row identifier(s):**
   - `row X from table Y` → single row deletion
   - `rows matching pattern X from table Y` → batch deletion

2. **For single row:**
   - Verify row exists with `mcp__unreal-data-bridge__get_datatable_row`
   - Show row data for confirmation
   - Ask user to confirm deletion
   - Load and call `mcp__unreal-data-bridge__delete_datatable_row`

3. **For pattern matching:**
   - Load and call `mcp__unreal-data-bridge__query_datatable` with pattern
   - List matching row names
   - Show count of rows to be deleted
   - Ask user to confirm deletion
   - Load and call `mcp__unreal-data-bridge__delete_datatable_row` for each row

4. **Confirm deletion:**
   - Report how many rows were deleted
   - List deleted row names
   - Report any errors

## Usage Patterns

```
/data-remove row Item_Sword_01 from table /Game/Data/DT_Items.DT_Items
/data-remove rows matching Test_* from table /Game/Data/DT_Items.DT_Items
/data-remove rows matching *_OLD from table /Game/Data/DT_Items.DT_Items
```

## Safety

- **ALWAYS require user confirmation before deleting**
- Show what will be deleted before asking for confirmation
- For batch deletions, show count and list of affected rows
- Cannot be undone - make this clear to user
- Suggest backup/commit before large deletions

## Error Handling

- If row not found, list available row names
- If table not found, suggest using `/data-search tables`
- If pattern matches no rows, report and exit
- If deletion fails, report specific error
