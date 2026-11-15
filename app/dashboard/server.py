from aiohttp import web
import aiohttp
import asyncio
import json
import os
import re

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
# Folder flight sim writes its output JSON files into (see
# docs/dashboard/sim_output_format.md). Override with SIM_OUTPUT_DIR.
SIM_OUTPUT_DIR = os.path.abspath(
    os.environ.get('SIM_OUTPUT_DIR') or os.path.join(PROJECT_ROOT, 'sim_output')
)
# Plain file names only: no path separators, no leading dot.
SIM_OUTPUT_NAME_RE = re.compile(r'^[A-Za-z0-9_][A-Za-z0-9_.\-]*\.json$')


def display_path(path):
    """Path relative to the repo when inside it, for showing in the UI."""
    rel = os.path.relpath(path, PROJECT_ROOT)
    return path if rel.startswith('..') else rel + '/'


class TestServer:
    def __init__(self):
        self.app = web.Application()
        self.setup_routes()
        self.clients = set()
        self.current_test = None

    def setup_routes(self):
        self.app.router.add_get('/', self.serve_index)
        self.app.router.add_get('/ws', self.websocket_handler)
        self.app.router.add_get('/api/sim-output', self.list_sim_output)
        self.app.router.add_get('/api/sim-output/{name}', self.get_sim_output)

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
        print(f'[dashboard] SIM_OUTPUT_DIR={SIM_OUTPUT_DIR} (exists={os.path.isdir(SIM_OUTPUT_DIR)})')

    async def serve_index(self, request):
        index_path = os.path.join(DASHBOARD_DIR, 'index.html')
        with open(index_path, encoding='utf-8') as f:
            return web.Response(text=f.read(), content_type='text/html')

    async def list_sim_output(self, request):
        """JSON files in SIM_OUTPUT_DIR, newest first."""
        exists = os.path.isdir(SIM_OUTPUT_DIR)
        files = []
        if exists:
            with os.scandir(SIM_OUTPUT_DIR) as it:
                for entry in it:
                    if entry.is_file() and SIM_OUTPUT_NAME_RE.match(entry.name):
                        st = entry.stat()
                        files.append({
                            'name': entry.name,
                            'size': st.st_size,
                            'modified': st.st_mtime,
                        })
        files.sort(key=lambda f: f['modified'], reverse=True)
        return web.json_response({
            'dir': display_path(SIM_OUTPUT_DIR),
            'exists': exists,
            'files': files,
        })

    async def get_sim_output(self, request):
        name = request.match_info['name']
        if not SIM_OUTPUT_NAME_RE.match(name):
            raise web.HTTPBadRequest(text='Invalid file name')
        path = os.path.join(SIM_OUTPUT_DIR, name)
        if not os.path.isfile(path):
            raise web.HTTPNotFound(text=f'{name} not found in {display_path(SIM_OUTPUT_DIR)}')
        return web.FileResponse(path, headers={
            'Content-Type': 'application/json',
            'Cache-Control': 'no-cache',
        })

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
            await self.handle_rc_commands(data['commands'], data.get('run_id'))

    async def handle_rc_commands(self, commands, run_id=None):
        command_str = ''.join(commands)
        print(f'[RC] Run {run_id}: received {len(commands)} commands: {command_str}')
        await self.broadcast({
            'type': 'log',
            'level': 'info',
            'step': 'rc-input',
            'message': f'Run {run_id}: RC commands queued ({len(commands)} steps): {command_str}'
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
            # return_exceptions: one closed tab must not stop the others.
            await asyncio.gather(
                *(client.send_json(message) for client in list(self.clients)),
                return_exceptions=True,
            )


def main():
    server = TestServer()
    # Overridable for hosting (see README "Dashboard Hosting"); defaults keep local dev unchanged.
    host = os.environ.get('DASHBOARD_HOST', 'localhost')
    port = int(os.environ.get('DASHBOARD_PORT', '8080'))
    web.run_app(server.app, host=host, port=port)


if __name__ == '__main__':
    main()
