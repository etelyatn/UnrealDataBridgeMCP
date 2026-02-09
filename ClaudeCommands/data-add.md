---
name: data-add
description: Add new DataTable rows or register GameplayTags
---

Add new data entries to Unreal Engine.

## Implementation Steps

1. **Parse target type:**
   - `row to table X named Y with {...}` → DataTable row
   - `tag X [with comment]` → GameplayTag

2. **For table rows:**
   - Load and call `mcp__unreal-data-bridge__get_datatable_schema` to get structure
   - Parse row data:
     - JSON format: `{"Field": "Value", "Number": 42}`
     - Natural language: `Name is "Item" and Damage is 50`
   - Validate against schema:
     - Check all required fields present
     - Validate data types
     - Check value ranges/constraints
   - Load and call `mcp__unreal-data-bridge__add_datatable_row` with validated data

3. **For GameplayTags:**
   - Validate tag format (hierarchical dot notation)
   - Check tag doesn't already exist with `mcp__unreal-data-bridge__validate_gameplay_tag`
   - Load and call `mcp__unreal-data-bridge__register_gameplay_tag` with tag and comment

4. **Confirm creation:**
   - Show what was created
   - Report success or errors
   - For rows, show how to query: `/unreal-data get table X row Y`

## Usage Patterns

```
/data-add row to table /Game/Data/DT_Items.DT_Items named Item_Axe_01 with {"Name": "Iron Axe", "Damage": 40, "Price": 100}
/data-add row to table /Game/Data/DT_Items.DT_Items named Item_Bow_01 with Name is "Wooden Bow" and Damage is 30
/data-add tag Item.Weapon.Axe
/data-add tag Item.Weapon.Axe with comment "New weapon type for melee combat"
```

## Validation

- Check row name doesn't already exist
- Verify all required schema fields provided
- Validate data types match schema
- Ensure tag follows hierarchical naming convention
- Warn if tag already exists
