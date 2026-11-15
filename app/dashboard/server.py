from aiohttp import web
import aiohttp
import asyncio
import json
import os
from uuid import uuid4

try:
    from .validation import compare_trajectory, finite_number, validate_limits, validate_samples
except ImportError:
    from validation import compare_trajectory, finite_number, validate_limits, validate_samples

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
PRESET_FILES = {name: f'{name}_test.json' for name in ('hover', 'circular', 'step', 'trapezoidal')}
MAX_RUN_SAMPLES = 100000


class TestServer:
    def __init__(self):
        self.app = web.Application()
        self.setup_routes()
        self.clients = set()
        self.current_test = None
        self.runs = {}

    def setup_routes(self):
        self.app.router.add_get('/', self.serve_index)
        self.app.router.add_get('/ws', self.websocket_handler)
        self.app.router.add_post('/api/runs', self.start_run_http)
        self.app.router.add_post('/api/simulation/samples', self.samples_http)
        self.app.router.add_post('/api/simulation/complete', self.complete_http)

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
                    try:
                        data = json.loads(msg.data)
                        response = await self.handle_message(ws, data)
                        if response is not None:
                            await ws.send_json(response)
                    except (ValueError, KeyError, TypeError, OverflowError) as error:
                        await ws.send_json({'type': 'error', 'message': str(error)})
                elif msg.type == aiohttp.WSMsgType.ERROR:
                    print(f'WebSocket connection closed with error: {ws.exception()}')
        finally:
            self.clients.remove(ws)

        return ws

    async def handle_message(self, ws, data):
        if not isinstance(data, dict):
            raise ValueError('Message must be an object')
        # Flight simulation may send the agreed packet without a command field.
        if 'samples' in data:
            return await self.receive_samples(data)
        command = data.get('command')
        if command == 'runTest':
            return await self.run_test(data.get('testId', 'custom'), data.get('reference'),
                                       data.get('limits'), data.get('max_sample_gap_s'))
        if command == 'rcCommands':
            commands = data.get('commands')
            if not isinstance(commands, list) or not commands or not all(isinstance(c, str) for c in commands):
                raise ValueError('commands must be a nonempty list of strings')
            response = None
            if 'testId' in data or 'reference' in data:
                response = await self.run_test(data.get('testId', 'custom'), data.get('reference'),
                                               data.get('limits'), data.get('max_sample_gap_s'))
            await self.handle_rc_commands(commands)
            return response
        if command == 'simulationComplete':
            return await self.complete_run(data)
        raise ValueError('Unknown command')

    async def http_action(self, request, action):
        try:
            data = await request.json()
            if not isinstance(data, dict):
                raise ValueError('Request must be a JSON object')
            return web.json_response(await action(data))
        except (ValueError, KeyError, TypeError, OverflowError) as error:
            return web.json_response({'type': 'error', 'message': str(error)}, status=400)

    async def start_run_http(self, request):
        async def start(data):
            return await self.run_test(data.get('testId', 'custom'), data.get('reference'),
                                       data.get('limits'), data.get('max_sample_gap_s'))
        return await self.http_action(request, start)

    async def samples_http(self, request):
        return await self.http_action(request, self.receive_samples)

    async def complete_http(self, request):
        return await self.http_action(request, self.complete_run)

    def active_run(self, data):
        run_id = data.get('run_id')
        if not isinstance(run_id, str) or run_id not in self.runs:
            raise ValueError('Unknown run_id')
        run = self.runs[run_id]
        if run['status'] != 'collecting':
            raise ValueError('Run is already finalized')
        return run_id, run

    async def receive_samples(self, data):
        run_id, run = self.active_run(data)
        batch = validate_samples(data.get('samples'), minimum=1)
        if len(run['actual']) + len(batch) > MAX_RUN_SAMPLES:
            raise ValueError('Run sample limit exceeded')
        if run['actual']:
            # Check batch boundaries too; do not silently replace duplicates.
            validate_samples([run['actual'][-1]] + batch, minimum=2)
        run['actual'].extend(batch)
        response = {'type': 'samplesAccepted', 'run_id': run_id,
                    'samples_received': len(run['actual'])}
        await self.broadcast(response)
        return response

    async def complete_run(self, data):
        run_id, run = self.active_run(data)
        failure = data.get('error')
        if failure is not None and (not isinstance(failure, str) or not failure.strip()):
            raise ValueError('error must be a nonempty string')
        run['status'] = 'validating'
        try:
            if failure:
                raise ValueError(f'Simulation failed: {failure}')
            result = await asyncio.to_thread(compare_trajectory, run['reference'], run['actual'],
                                            limits=run['limits'], max_sample_gap_s=run['max_sample_gap_s'])
        except (ValueError, OverflowError) as error:
            result = {'data_valid': False, 'passed': None, 'error': str(error)}
        run['status'] = 'completed' if result['data_valid'] else 'invalid'
        run['result'] = result
        message = {'type': 'validationResult', 'run_id': run_id, 'result': result}
        await self.broadcast(message)
        return message

    async def handle_rc_commands(self, commands):
        command_str = ''.join(commands)
        print(f'[RC] Received {len(commands)} commands: {command_str}')
        await self.broadcast({
            'type': 'log',
            'level': 'info',
            'step': 'rc-input',
            'message': f'RC commands received ({len(commands)} steps): {command_str}'
        })
        # TODO: forward commands to the test node over SPI/serial when that
        # interface is ready.

    async def run_test(self, test_id, reference=None, limits=None, max_sample_gap_s=None):
        if not isinstance(test_id, str) or not test_id:
            raise ValueError('testId must be a nonempty string')
        if reference is None:
            if test_id not in PRESET_FILES:
                raise ValueError('Provide reference samples for a custom test')
            path = os.path.join(TESTS_DIR, 'generateTests', 'JSONtests', PRESET_FILES[test_id])
            with open(path, encoding='utf-8') as source:
                reference = json.load(source)
        reference = validate_samples(reference, 'reference')
        limits = validate_limits(limits)
        if max_sample_gap_s is not None and (not finite_number(max_sample_gap_s) or max_sample_gap_s <= 0):
            raise ValueError('max_sample_gap_s must be positive and finite')
        if any(run['status'] in ('collecting', 'validating') for run in self.runs.values()):
            raise ValueError('Complete the active run before starting another')
        run_id = uuid4().hex
        self.runs[run_id] = {'reference': reference, 'actual': [], 'limits': limits,
                             'max_sample_gap_s': max_sample_gap_s, 'status': 'collecting',
                             'test_id': test_id}
        self.current_test = test_id
        message = {'type': 'runStarted', 'run_id': run_id, 'test_id': test_id,
                   'status': 'Waiting for simulation samples'}
        await self.broadcast(message)
        return message

    async def broadcast(self, message):
        if self.clients:
            await asyncio.gather(
                *(client.send_json(message) for client in tuple(self.clients)),
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
