---
name: data-update
description: Update DataTable rows or DataAsset properties with natural language
---

Update data using natural language descriptions.

## Implementation Steps

1. **Parse target type:**
   - `table X row Y set Field to Value` → DataTable row update
   - `asset /Path/To/Asset set Property to Value` → DataAsset update

2. **For table rows:**
   - Load and call `mcp__unreal-data-bridge__get_datatable_row` to get current data
   - Parse field changes:
     - `set Field to Value` → direct assignment
     - `increase Field by N` → add N to current value
     - `increase Field by N%` → multiply by (1 + N/100)
     - `decrease Field by N` → subtract N from current value
     - `decrease Field by N%` → multiply by (1 - N/100)
   - Calculate new values based on current data
   - Load and call `mcp__unreal-data-bridge__update_datatable_row` with updated row

3. **For data assets:**
   - Load and call `mcp__unreal-data-bridge__get_data_asset` to get current properties
   - Parse property changes (similar to table fields)
   - Load and call `mcp__unreal-data-bridge__update_data_asset` with updated properties

4. **Confirm changes:**
   - Show before/after values
   - Ask user to confirm before applying
   - Report success or errors

## Usage Patterns

```
/data-update table /Game/Data/DT_Items.DT_Items row Item_Sword_01 set Damage to 50
/data-update table /Game/Data/DT_Items.DT_Items row Item_Sword_01 increase Damage by 10
/data-update table /Game/Data/DT_Items.DT_Items row Item_Sword_01 increase Price by 20%
/data-update table /Game/Data/DT_Items.DT_Items row Item_Sword_01 decrease Weight by 5%
/data-update asset /Game/Data/DA_Settings.DA_Settings set MaxPlayers to 8
```

## Safety

- Always show changes before applying
- Require user confirmation for destructive operations
- Validate field/property names against schema
- Check data types before assignment
- Preserve unchanged fields
