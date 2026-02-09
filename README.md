# Unreal Data Bridge MCP

Bridge AI tools to Unreal Engine's data systems via the Model Context Protocol.

| | |
|---|---|
| **Version** | 0.1.0 (beta) |
| **License** | MIT |
| **Engine** | Unreal Engine 5.5+ |
| **Python** | 3.10+ |

## How It Works

The bridge has two components:

1. **UE C++ Editor Plugin** -- A TCP server (default port 8742) running inside Unreal Editor that exposes DataTables, GameplayTags, DataAssets, and StringTables via JSON commands over a persistent TCP connection.
2. **Python MCP Server** -- A FastMCP server using stdio transport that translates MCP tool calls from AI coding assistants into TCP commands sent to the editor plugin.

```
AI Tool (Claude Code)  <--stdio-->  Python MCP Server  <--TCP-->  UE Editor Plugin
```

## Quick Start

**1. Enable the plugin**

Copy `UnrealDataBridge/` to your project's `Plugins/` folder and add it to your `.uproject`:

```json
{
  "Plugins": [
    { "Name": "UnrealDataBridge", "Enabled": true }
  ]
}
```

Build the project. The TCP server starts automatically when the editor launches.

**2. Install the Python MCP server**

```bash
cd Plugins/UnrealDataBridge/MCP
pip install -e .
```

**3. Configure Claude Code**

Add the MCP server to your Claude Code config (`.mcp.json` in your project root, or `~/.claude/settings.json` for global):

```json
{
  "mcpServers": {
    "unreal-data-bridge": {
      "command": "python",
      "args": ["-m", "unreal_data_bridge_mcp"],
      "env": {
        "UDB_HOST": "127.0.0.1",
        "UDB_PORT": "8742"
      }
    }
  }
}
```

**4. Verify**

With the editor running, call `get_status` from your AI tool. It should return the plugin version, engine version, and project name.

## Plugin Installation

The editor plugin module loads at `PostEngineInit` phase and starts a TCP server on localhost. It is an editor-only module -- it does not ship with cooked builds.

**Requirements:**
- Unreal Engine 5.5 or later
- The `StructUtils` plugin (bundled with UE, enabled automatically as a dependency)

**Steps:**

1. Copy the `UnrealDataBridge/` folder into your project's `Plugins/` directory.
2. Add the plugin to your `.uproject` file:
   ```json
   { "Name": "UnrealDataBridge", "Enabled": true }
   ```
3. Regenerate project files and build.
4. Launch the editor. Check **Output Log** for `LogUDB: TCP server listening on 127.0.0.1:8742`.

## MCP Server Setup

The Python MCP server lives in `Plugins/UnrealDataBridge/MCP/`. It requires Python 3.10+ and the `mcp` package (FastMCP).

**Install (development mode):**

```bash
cd Plugins/UnrealDataBridge/MCP
pip install -e .
```

**Install (from package):**

```bash
pip install ./Plugins/UnrealDataBridge/MCP
```

After installation, the `unreal-data-bridge-mcp` command is available, or you can run it as a module:

```bash
python -m unreal_data_bridge_mcp
```

The server uses stdio transport -- it is designed to be launched by an MCP client (like Claude Code), not run standalone.

## Configuration

### Editor Plugin Settings

Open **Project Settings > Plugins > Unreal Data Bridge** in the Unreal Editor:

| Setting | Default | Description |
|---------|---------|-------------|
| Port | 8742 | TCP server port (range: 1024--65535) |
| Auto Start | true | Start TCP server automatically when editor loads |
| Log Commands | false | Log all incoming commands to Output Log (verbose mode) |
| Tag Prefix To Ini File | (empty) | Map GameplayTag prefixes to specific `.ini` files for `register_gameplay_tag` |

### Environment Variables (Python MCP Server)

| Variable | Default | Description |
|----------|---------|-------------|
| `UDB_HOST` | `127.0.0.1` | TCP host to connect to |
| `UDB_PORT` | `8742` | TCP port to connect to |
| `UDB_LOG_LEVEL` | `INFO` | Logging level (`DEBUG`, `INFO`, `WARNING`, `ERROR`) |

## Available Tools (21)

### Status

| Tool | Description |
|------|-------------|
| `get_status` | Check connection to Unreal Editor and get plugin version, engine version, project name |

### DataTables (9)

| Tool | Description |
|------|-------------|
| `list_datatables` | List all loaded DataTables with name, path, row struct, and row count |
| `get_datatable_schema` | Get row struct schema showing fields, types, and constraints |
| `query_datatable` | Query rows with wildcard filtering, field selection, and pagination |
| `get_datatable_row` | Get a specific row by row name |
| `get_struct_schema` | Get schema for any UStruct type by name |
| `add_datatable_row` | Add a new row to a DataTable |
| `update_datatable_row` | Update an existing row (partial update, unspecified fields unchanged) |
| `delete_datatable_row` | Delete a row from a DataTable |
| `import_datatable_json` | Bulk import rows with create/upsert/replace modes and dry-run support |

### GameplayTags (4)

| Tool | Description |
|------|-------------|
| `list_gameplay_tags` | List registered tags, optionally filtered by prefix |
| `validate_gameplay_tag` | Check if a specific tag is registered |
| `register_gameplay_tag` | Register a new tag, writing to the appropriate `.ini` file |
| `register_gameplay_tags` | Batch register multiple tags in one call |

### DataAssets (3)

| Tool | Description |
|------|-------------|
| `list_data_assets` | List DataAssets filtered by class or path |
| `get_data_asset` | Read all properties of a DataAsset |
| `update_data_asset` | Update properties on a DataAsset (partial update) |

### Localization (3)

| Tool | Description |
|------|-------------|
| `list_string_tables` | List StringTable assets with name, path, and entry count |
| `get_translations` | Read translation entries with optional wildcard key filtering |
| `set_translation` | Set a translation entry (creates or updates) |

### Asset Search (1)

| Tool | Description |
|------|-------------|
| `search_assets` | Search the Asset Registry by name, class, or path |

## Architecture

```
Plugins/UnrealDataBridge/
  Source/
    UnrealDataBridge/           # Editor-only C++ module
      Public/
        UnrealDataBridgeModule.h
        UDBTcpServer.h          # TCP server, handles connections
        UDBCommandHandler.h     # Routes commands to operations
        UDBSerializer.h         # UStruct <-> JSON serialization
        UDBSettings.h           # Developer settings (port, etc.)
      Private/
        Operations/             # One file per command group
        ...
    UnrealDataBridgeTests/      # Automation tests
  MCP/
    pyproject.toml              # Python package definition
    src/
      unreal_data_bridge_mcp/
        server.py               # FastMCP server, tool registration
        tcp_client.py           # TCP connection to UE plugin
        tools/
          datatables.py         # DataTable tools
          gameplay_tags.py      # GameplayTag tools
          data_assets.py        # DataAsset tools
          localization.py       # StringTable/localization tools
          assets.py             # Asset search tools
  UnrealDataBridge.uplugin      # Plugin descriptor
  LICENSE                       # MIT License
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Connection refused | Make sure Unreal Editor is running with the plugin enabled. Check Output Log for `LogUDB: TCP server listening`. |
| Port conflict | Change the port in **Project Settings > Plugins > Unreal Data Bridge** and update `UDB_PORT` in your MCP config to match. |
| MCP server not connecting | Verify `UDB_PORT` matches the editor port. Check that no firewall is blocking localhost connections. |
| Write operations not persisting | Write operations (add/update/delete row, set translation, update asset) mark assets as dirty but do not auto-save. Use **File > Save All** in the editor. |
| GameplayTag not found after register | The tag is written to the `.ini` file and the in-memory tag tree is refreshed. If the tag still does not appear, restart the editor. |
| Command times out | The default receive timeout is 60 seconds. Large operations (bulk import) may take longer. Check editor performance and Output Log for errors. |
| `pip install` fails | Ensure Python 3.10+ is installed. The only dependency is the `mcp` package (version 1.2.0+). |

## Protocol Details

Communication uses newline-delimited JSON over TCP.

**Request format:**
```json
{"command": "get_datatable_row", "params": {"table_path": "/Game/Data/DT_Example.DT_Example", "row_name": "Row1"}}
```

**Success response:**
```json
{"success": true, "data": {"row_name": "Row1", "row_struct": "FExampleRow", "row_data": {"Field1": "value"}}}
```

**Error response:**
```json
{"success": false, "error": {"code": "ROW_NOT_FOUND", "message": "Row 'Row1' not found in DataTable"}}
```

Each message is a single JSON object terminated by a newline (`\n`). The TCP connection is persistent -- the MCP server reconnects automatically if the connection drops.

## License

MIT License. See [LICENSE](LICENSE) for details.
