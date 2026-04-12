from aiohttp import web
import aiohttp
import asyncio
import json
import os

# Absolute paths only: never rely on the process cwd.
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(BASE_DIR, '..', '..'))
DASHBOARD_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, 'app', 'dashboard'))
if os.path.normcase(DASHBOARD_DIR) != os.path.normcase(BASE_DIR):
    print(
        f'[dashboard] WARNING: dashboard path mismatch. __file__ dir is {BASE_DIR!r}, '
        f'expected {DASHBOARD_DIR!r} from PROJECT_ROOT. Serving static files from __file__ dir.'
    )
    DASHBOARD_DIR = BASE_DIR
TESTS_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, 'tests'))


class TestServer:
    def __init__(self):
        self.app = web.Application()
        self.setup_routes()
        self.clients = set()
        self.current_test = None

    def setup_routes(self):
        self.app.router.add_get('/', self.serve_index)
        self.app.router.add_get('/ws', self.websocket_handler)

        if os.path.isdir(TESTS_DIR):
            # Mount repo tests as a sub-app so requests are never handled by the
            # dashboard add_static('/') root (which otherwise returns 404 for /tests/...).
            tests_app = web.Application()
            tests_app.router.add_static('/', TESTS_DIR, show_index=True)
            self.app.add_subapp('/tests', tests_app)
        else:
            print(
                f'[dashboard] WARNING: tests directory not found at {TESTS_DIR!r} '
                f'(PROJECT_ROOT={PROJECT_ROOT!r}). /tests/ JSON will 404.'
            )

        self.app.router.add_static('/', path=DASHBOARD_DIR)

        print(f'[dashboard] PROJECT_ROOT={PROJECT_ROOT}')
        print(f'[dashboard] DASHBOARD_DIR={DASHBOARD_DIR}')
        print(f'[dashboard] TESTS_DIR={TESTS_DIR} (exists={os.path.isdir(TESTS_DIR)})')

    async def serve_index(self, request):
        index_path = os.path.join(DASHBOARD_DIR, 'index.html')
        with open(index_path, encoding='utf-8') as f:
            return web.Response(text=f.read(), content_type='text/html')

    async def websocket_handler(self, request):
        ws = web.WebSocketResponse()
        await ws.prepare(request)
        self.clients.add(ws)

        try:
            async for msg in ws:
                if msg.type == aiohttp.WSMsgType.TEXT:
                    data = json.loads(msg.data)
                    await self.handle_message(ws, data)
                elif msg.type == aiohttp.WSMsgType.ERROR:
                    print(f'WebSocket connection closed with error: {ws.exception()}')
        finally:
            self.clients.remove(ws)

        return ws

    async def handle_message(self, ws, data):
        if data['command'] == 'runTest':
            await self.run_test(data['testId'])
        elif data['command'] == 'rcCommands':
            await self.handle_rc_commands(data['commands'])

    async def handle_rc_commands(self, commands):
        command_str = ''.join(commands)
        print(f'[RC] Received {len(commands)} commands: {command_str}')
        await self.broadcast({
            'type': 'log',
            'level': 'info',
            'step': 'rc-input',
            'message': f'RC commands queued ({len(commands)} steps): {command_str}'
        })
        # TODO: forward commands to the test node over SPI/serial when that
        # interface is ready.

    async def run_test(self, test_id):
        self.current_test = test_id
        await self.broadcast({'type': 'status', 'status': f'Running test {test_id}'})

        for i in range(0, 101, 10):
            await asyncio.sleep(1)
            await self.broadcast({
                'type': 'progress',
                'value': i
            })
            await self.broadcast({
                'type': 'log',
                'level': 'info',
                'message': f'Test progress: {i}%'
            })

        await self.broadcast({
            'type': 'testComplete',
            'success': True,
            'message': f'Test {test_id} completed successfully'
        })

    async def broadcast(self, message):
        if self.clients:
            await asyncio.gather(
                *(client.send_json(message) for client in self.clients)
            )


def main():
    server = TestServer()
    web.run_app(server.app, host='localhost', port=8080)


if __name__ == '__main__':
    main()
