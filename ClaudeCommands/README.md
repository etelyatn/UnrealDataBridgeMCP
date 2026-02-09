# UnrealDataBridge Claude Commands

Natural language commands for working with Unreal Engine data via the UnrealDataBridge MCP.

## Prerequisites

1. UnrealDataBridge plugin installed and enabled in your project
2. MCP server configured in `.mcp.json` (see main README)
3. Unreal Editor running with project open

## Installation

### Option 1: Symlink (Recommended for Development)

**Windows (Git Bash):**
```bash
cd /d/path/to/YourProject
mkdir -p .claude/commands
cd .claude/commands
ln -s ../../Plugins/UnrealDataBridge/ClaudeCommands/*.md .
```

**Windows (Command Prompt as Administrator):**
```cmd
cd D:\path\to\YourProject\.claude\commands
mklink unreal-data.md ..\..\Plugins\UnrealDataBridge\ClaudeCommands\unreal-data.md
mklink data-search.md ..\..\Plugins\UnrealDataBridge\ClaudeCommands\data-search.md
mklink data-update.md ..\..\Plugins\UnrealDataBridge\ClaudeCommands\data-update.md
mklink data-add.md ..\..\Plugins\UnrealDataBridge\ClaudeCommands\data-add.md
mklink data-remove.md ..\..\Plugins\UnrealDataBridge\ClaudeCommands\data-remove.md
```

### Option 2: Copy Files

```bash
cp Plugins/UnrealDataBridge/ClaudeCommands/*.md .claude/commands/
```

**Note:** With copying, you'll need to manually update commands when the plugin updates.

### Verify Installation

1. Restart Claude Code
2. Run `/help` and verify commands appear:
   - `/unreal-data`
   - `/data-search`
   - `/data-update`
   - `/data-add`
   - `/data-remove`

## Available Commands

| Command | Purpose | Example |
|---------|---------|---------|
| `/unreal-data` | Flexible natural language queries | `/unreal-data get table DT_Items row Sword_01` |
| `/data-search` | Search tables, assets, tags | `/data-search assets Weapon` |
| `/data-update` | Modify table rows or assets | `/data-update table DT_Items row Sword_01 set Damage to 50` |
| `/data-add` | Add rows or register tags | `/data-add row to table DT_Items named Axe_01 with {...}` |
| `/data-remove` | Delete table rows | `/data-remove row Sword_01 from table DT_Items` |

## Usage Examples

### Query Data
```
/unreal-data get status
/unreal-data get table /Game/Data/DT_Items.DT_Items row Item_Sword_01
/unreal-data list tables in /Game/Data/
/unreal-data show schema for table /Game/Data/DT_Items.DT_Items
/data-search tables /Game/Data/
/data-search assets containing "Weapon"
/data-search tags Item.
```

### Modify Data
```
/data-update table /Game/Data/DT_Items.DT_Items row Sword_01 set Damage to 50
/data-update table /Game/Data/DT_Items.DT_Items row Sword_01 increase Price by 20%
/data-update table /Game/Data/DT_Items.DT_Items row Sword_01 decrease Weight by 5%
```

### Add Data
```
/data-add row to table /Game/Data/DT_Items.DT_Items named Axe_01 with {"Name": "Iron Axe", "Damage": 40}
/data-add tag Item.Weapon.Axe with comment "New weapon type"
```

### Remove Data
```
/data-remove row Test_Item from table /Game/Data/DT_Items.DT_Items
/data-remove rows matching Test_* from table /Game/Data/DT_Items.DT_Items
```

## Troubleshooting

**Commands not appearing:**
- Restart Claude Code after installation
- Verify files exist in `.claude/commands/`
- Check file permissions (symlinks require admin on Windows)

**"MCP tool not found" errors:**
- Verify `.mcp.json` is configured (see main README)
- Ensure Unreal Editor is running
- Test MCP connection: `/unreal-data get status`

**"Table not found" errors:**
- Use full asset paths: `/Game/Data/DT_Items.DT_Items`
- List tables: `/data-search tables /Game/`

## Advanced Usage

### Batch Operations

Update multiple rows by combining commands:
```
/data-update table DT_Items row Sword_01 increase Damage by 10%
/data-update table DT_Items row Sword_02 increase Damage by 10%
/data-update table DT_Items row Sword_03 increase Damage by 10%
```

### Schema Inspection

Before adding rows, check the schema:
```
/unreal-data show schema for table /Game/Data/DT_Items.DT_Items
```

### Tag Management

Register tags in bulk:
```
/data-add tag Item.Weapon.Sword
/data-add tag Item.Weapon.Axe
/data-add tag Item.Weapon.Bow
```

## Command Design

These commands are **thin wrappers** around UnrealDataBridge MCP tools:

1. **Parse** natural language → extract parameters
2. **Validate** inputs → check format, existence
3. **Call** MCP tools → delegate to UnrealDataBridge
4. **Format** output → make responses readable

No business logic duplication - commands orchestrate, MCP does the work.

## Contributing

Commands are versioned with the UnrealDataBridge plugin. To modify:

1. Edit `.md` files in `Plugins/UnrealDataBridge/ClaudeCommands/`
2. Test changes in your project
3. Commit to the UnrealDataBridge repository
4. Users update via `git pull` in plugin directory
