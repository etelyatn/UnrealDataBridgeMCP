---
name: data-remove
description: Remove DataTable rows
---

Remove data entries from Unreal Engine DataTables.

## Implementation Steps

1. **Check conversation context first.** If you already know the table path,
   skip discovery. Only call discovery tools if genuinely needed.

2. **Parse table path and row identifier(s):**
   - `row X from table Y` → single row deletion
   - `rows matching pattern X from table Y` → batch deletion

3. **For single row:**
   - Verify row exists with `get_datatable_row`
   - Show row data for confirmation
   - Ask user to confirm deletion
   - Call `delete_datatable_row`

4. **For multiple specific rows:**
   - Call `query_datatable` with `row_names` to fetch all target rows at once
   - Show row data for confirmation
   - Use `batch_query` to combine all `delete_datatable_row` calls into one round-trip

5. **For pattern matching:**
   - Call `query_datatable` with pattern and `fields` to keep response small
   - List matching row names
   - Show count of rows to be deleted
   - Ask user to confirm deletion
   - Use `batch_query` to combine all `delete_datatable_row` calls into one round-trip

6. **Confirm deletion:**
   - Report how many rows were deleted
   - List deleted row names
   - Report any errors

## Usage Patterns

```
/data-remove row Item_Sword_01 from table /Game/Data/DT_Items.DT_Items
/data-remove rows matching Test_* from table /Game/Data/DT_Items.DT_Items
```

## Safety

- **ALWAYS require user confirmation before deleting**
- Show what will be deleted before asking for confirmation
- For batch deletions, show count and list of affected rows
- Cannot be undone - make this clear to user
- Suggest backup/commit before large deletions

## Error Handling

- If row not found, list available row names
- If table not found, suggest checking table paths
- If pattern matches no rows, report and exit
- If deletion fails, report specific error
