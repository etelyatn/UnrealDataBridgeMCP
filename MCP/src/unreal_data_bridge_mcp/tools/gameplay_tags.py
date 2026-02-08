"""MCP tools for GameplayTag operations."""

import json
import logging
from ..tcp_client import UEConnection

logger = logging.getLogger(__name__)


def register_gameplay_tag_tools(mcp, connection: UEConnection):
    """Register all GameplayTag-related MCP tools."""

    @mcp.tool()
    def list_gameplay_tags(prefix: str = "") -> str:
        """List all registered GameplayTags, optionally filtered by prefix.

        Args:
            prefix: Only return tags starting with this prefix (e.g., 'Quest.', 'Patient.Type.').
        """
        try:
            response = connection.send_command("list_gameplay_tags", {"prefix": prefix})
            return json.dumps(response.get("data", {}), indent=2)
        except ConnectionError as e:
            return f"Error: {e}"

    @mcp.tool()
    def validate_gameplay_tag(tag: str) -> str:
        """Check if a GameplayTag is registered and valid.

        Args:
            tag: The tag string to validate (e.g., 'Quest.Generic.1').
        """
        try:
            response = connection.send_command("validate_gameplay_tag", {"tag": tag})
            return json.dumps(response.get("data", {}), indent=2)
        except ConnectionError as e:
            return f"Error: {e}"

    @mcp.tool()
    def register_gameplay_tag(tag: str, ini_file: str = "", dev_comment: str = "") -> str:
        """Register a new GameplayTag in the project.

        The tag is written to a .ini file in Config/Tags/. The target file is determined by:
        1. The ini_file parameter (if provided)
        2. The project's tag prefix mapping in plugin settings
        3. Default: Config/Tags/GameplayTags.ini

        Args:
            tag: Tag string to register (e.g., 'Quest.NewCategory.SubTag').
            ini_file: Override the target .ini file path (relative to Config/).
            dev_comment: Developer comment for the tag entry.
        """
        try:
            params = {"tag": tag}
            if ini_file:
                params["ini_file"] = ini_file
            if dev_comment:
                params["dev_comment"] = dev_comment
            response = connection.send_command("register_gameplay_tag", params)
            return json.dumps(response.get("data", {}), indent=2)
        except ConnectionError as e:
            return f"Error: {e}"

    @mcp.tool()
    def register_gameplay_tags(tags: str) -> str:
        """Batch register multiple GameplayTags.

        Args:
            tags: JSON string with array of objects, each containing 'tag' and optionally
                  'ini_file' and 'dev_comment'.
                  Example: '[{"tag": "Quest.New.1"}, {"tag": "Quest.New.2", "dev_comment": "Second quest"}]'
        """
        try:
            tags_data = json.loads(tags)
            response = connection.send_command("register_gameplay_tags", {"tags": tags_data})
            return json.dumps(response.get("data", {}), indent=2)
        except json.JSONDecodeError as e:
            return f"Error: Invalid JSON in tags: {e}"
        except ConnectionError as e:
            return f"Error: {e}"
