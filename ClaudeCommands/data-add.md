---
name: data-add
description: Add new DataTable rows or register GameplayTags
---

Add new data entries to Unreal Engine.

## Implementation Steps

1. **Check conversation context first.** If you already know the table path and
   field structure, skip discovery. Only call `get_data_catalog` or `get_datatable_schema`
   if you genuinely need to discover tables or understand field types.

2. **Parse target type:**
   - `row to table X named Y with {...}` → DataTable row
   - `tag X [with comment]` → GameplayTag

3. **For table rows:**
   - If schema unknown, call `get_datatable_schema` for full type details
   - Parse row data:
     - JSON format: `{"Field": "Value", "Number": 42}`
     - Natural language: `Name is "Item" and Damage is 50`
   - Validate against schema:
     - Check all required fields present
     - Validate data types
     - Check value ranges/constraints
   - Call `add_datatable_row` with validated data

4. **For GameplayTags:**
   - Validate tag format (hierarchical dot notation)
   - Check tag doesn't already exist with `validate_gameplay_tag`
   - Call `register_gameplay_tag` with tag and comment

5. **Confirm creation:**
   - Show what was created
   - Report success or errors

## Usage Patterns

```
/data-add row to table /Game/Data/DT_Items.DT_Items named Item_Axe_01 with {"Name": "Iron Axe", "Damage": 40}
/data-add tag Item.Weapon.Axe
/data-add tag Item.Weapon.Axe with comment "New weapon type for melee combat"
```

## Validation

- Check row name doesn't already exist
- Verify all required schema fields provided
- Validate data types match schema
- Ensure tag follows hierarchical naming convention
- Warn if tag already exists
