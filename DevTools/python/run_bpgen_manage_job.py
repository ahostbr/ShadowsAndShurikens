import asyncio
import json
import sys
from pathlib import Path
from typing import Any, Dict, Iterable

from mcp.client.session import ClientSession
from mcp.client.stdio import StdioServerParameters, stdio_client

PROJECT_ROOT = Path(__file__).resolve().parents[1]
PYTHON_EXE = Path(sys.executable)
SERVER_SCRIPT = PROJECT_ROOT / 'DevTools' / 'python' / 'sots_mcp_server' / 'server.py'
BLUEPRINT_BASE = '/Game/BPGen/BP_BPGen_Testing'
BLUEPRINT_FULL = f'{BLUEPRINT_BASE}.BP_BPGen_Testing'
FUNCTION_NAME = 'EventGraph'

SERVER_ENV = {
    'SOTS_ALLOW_APPLY': '1',
    'SOTS_ALLOW_BPGEN_APPLY': '1',
}

NODE_DEFINITIONS: Iterable[Dict[str, Any]] = [
    {
        'title': 'BeginPlay event',
        'params': {
            'node_id': 'BeginPlay',
            'node_type': 'K2Node_Event',
            'spawner_key': '/Script/Engine.Actor:ReceiveBeginPlay',
            'function_path': '/Script/Engine.Actor:ReceiveBeginPlay',
        },
        'position': [-640, 0],
    },
    {
        'title': 'Delay call',
        'params': {
            'node_id': 'Delay',
            'node_type': 'K2Node_CallFunction',
            'spawner_key': '/Script/Engine.KismetSystemLibrary:Delay',
            'function_path': '/Script/Engine.KismetSystemLibrary:Delay',
        },
        'position': [-200, 0],
        'extra': {'Duration': '3.0'},
    },
    {
        'title': 'Get SOTS UI router',
        'params': {
            'node_id': 'GetUIRouter',
            'node_type': 'K2Node_GetSubsystem',
            'spawner_key': '/Script/BlueprintGraph.K2Node_GetSubsystem',
            'function_path': '',
        },
        'position': [-200, -220],
        'extra': {
            'CustomClass': '/Script/CoreUObject.Class\'/Script/SOTS_UI.SOTS_UIRouterSubsystem\'',
        },
    },
    {
        'title': 'Show notification',
        'params': {
            'node_id': 'ShowNotification',
            'node_type': 'K2Node_CallFunction',
            'spawner_key': '/Script/SOTS_UI.SOTS_UIRouterSubsystem:ShowNotification',
            'function_path': '/Script/SOTS_UI.SOTS_UIRouterSubsystem:ShowNotification',
        },
        'position': [320, 0],
        'extra': {
            'Message': 'Merry Christmas Ryan !',
            'DurationSeconds': '3.0',
            'CategoryTag': 'SAS.UI',
        },
    },
]

CONNECTIONS = [
    {
        'FromNodeId': 'BeginPlay',
        'FromPinName': 'then',
        'ToNodeId': 'Delay',
        'ToPinName': 'execute',
    },
    {
        'FromNodeId': 'Delay',
        'FromPinName': 'Completed',
        'ToNodeId': 'ShowNotification',
        'ToPinName': 'execute',
    },
    {
        'FromNodeId': 'GetUIRouter',
        'FromPinName': 'ReturnValue',
        'ToNodeId': 'ShowNotification',
        'ToPinName': 'self',
    },
]


def _dump(title: str, payload: Dict[str, Any]) -> None:
    print(f'\n--- {title} ---')
    print(json.dumps(payload, ensure_ascii=False, indent=2))


def _tool_payload(action: str, node: Dict[str, Any]) -> Dict[str, Any]:
    data: Dict[str, Any] = {
        'action': action,
        'blueprint_asset_path': BLUEPRINT_FULL,
        'function_name': FUNCTION_NAME,
        'node_params': node['params'],
        'position': node['position'],
    }
    extra = node.get('extra')
    if extra:
        data['extra'] = extra
    return data


async def main() -> None:
    server_params = StdioServerParameters(
        command=str(PYTHON_EXE),
        args=[str(SERVER_SCRIPT)],
        env=SERVER_ENV,
        cwd=str(PROJECT_ROOT),
    )

    async with stdio_client(server_params) as (read_stream, write_stream):
        session = ClientSession(read_stream, write_stream)
        await session.initialize()

        create_blueprint_payload = {
            'asset_path': BLUEPRINT_BASE,
            'AssetPath': BLUEPRINT_BASE,
            'parent_class_path': '/Script/Engine.Actor',
            'ParentClassPath': '/Script/Engine.Actor',
        }
        create_result = await session.call_tool('bpgen_create_blueprint', create_blueprint_payload)
        _dump('bpgen_create_blueprint', create_result.model_dump())

        for node in NODE_DEFINITIONS:
            payload = _tool_payload('create', node)
            result = await session.call_tool('bpgen_manage_blueprint_node', payload)
            _dump('bpgen_manage_blueprint_node (create ' + node['title'] + ')', result.model_dump())

        connect_payload = {
            'action': 'connect_pins',
            'blueprint_asset_path': BLUEPRINT_FULL,
            'function_name': FUNCTION_NAME,
            'connections': CONNECTIONS,
        }
        connect_result = await session.call_tool('bpgen_manage_blueprint_node', connect_payload)
        _dump('bpgen_manage_blueprint_node (connect_pins)', connect_result.model_dump())


if __name__ == '__main__':
    asyncio.run(main())
