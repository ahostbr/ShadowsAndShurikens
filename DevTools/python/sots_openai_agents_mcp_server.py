from __future__ import annotations

import anyio
from mcp.server import InitializationOptions, NotificationOptions, Server
from mcp.server.stdio import stdio_server
from mcp.types import TextContent, Tool

# Minimal MCP stdio server placeholder. Extend tools once the real agent-backed surface is ready.
server = Server("openai-agents-wrapper")


@server.list_tools()
async def list_tools() -> list[Tool]:
    return [
        Tool(
            name="ping",
            description="Health check for the OpenAI Agents MCP wrapper.",
            inputSchema={"type": "object", "properties": {}, "required": []},
        )
    ]


@server.call_tool()
async def call_tool(name: str, arguments: dict | None) -> list[TextContent]:
    if name != "ping":
        raise ValueError(f"Unknown tool: {name}")
    return [TextContent(type="text", text="pong")]


async def main() -> None:
    async with stdio_server() as (read, write):
        init_opts = InitializationOptions(
            server_name="openai-agents-wrapper",
            server_version="0.0.1",
            capabilities=server.get_capabilities(
                notification_options=NotificationOptions(),
                experimental_capabilities={},
            ),
        )
        await server.run(read, write, initialization_options=init_opts)


if __name__ == "__main__":
    anyio.run(main)
