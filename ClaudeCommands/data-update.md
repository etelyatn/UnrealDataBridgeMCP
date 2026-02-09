---
name: data-update
description: Update DataTable rows or DataAsset properties with natural language
---

Update data using natural language descriptions.

## Implementation Steps

1. **Check conversation context first.** If you already know the table path and
   row structure, skip discovery. Only call discovery tools if genuinely needed.

2. **Parse target type:**
   - `table X row Y set Field to Value` → DataTable row update
   - `asset /Path/To/Asset set Property to Value` → DataAsset update

3. **For table rows:**
   - For a single row: call `get_datatable_row` to get current data
   - For multiple rows: call `query_datatable` with `row_names` to fetch all at once
   - Parse field changes:
     - `set Field to Value` → direct assignment
     - `increase Field by N` → add N to current value
     - `increase Field by N%` → multiply by (1 + N/100)
     - `decrease Field by N` → subtract N from current value
     - `decrease Field by N%` → multiply by (1 - N/100)
   - Calculate new values based on current data
   - Call `update_datatable_row` with updated row
   - For multiple updates, use `batch_query` to combine all `update_datatable_row`
     calls into one round-trip

4. **For data assets:**
   - Call `get_data_asset` to get current properties
   - Parse property changes (similar to table fields)
   - Call `update_data_asset` with updated properties

5. **Confirm changes:**
   - Show before/after values
   - Ask user to confirm before applying
   - Report success or errors

## Usage Patterns

```
/data-update table /Game/Data/DT_Items.DT_Items row Item_Sword_01 set Damage to 50
/data-update table /Game/Data/DT_Items.DT_Items row Item_Sword_01 increase Damage by 10
/data-update asset /Game/Data/DA_Settings.DA_Settings set MaxPlayers to 8
```

## Safety

- Always show changes before applying
- Require user confirmation for destructive operations
- Validate field/property names against schema
- Check data types before assignment
- Preserve unchanged fields
