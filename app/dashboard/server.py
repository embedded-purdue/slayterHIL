from aiohttp import web
import aiohttp
import asyncio
import json
import os

class TestServer:
    def __init__(self):
        self.app = web.Application()
        self.setup_routes()
        self.clients = set()
        self.current_test = None

    def setup_routes(self):
        self.app.router.add_get('/', self.serve_index)
        self.app.router.add_get('/ws', self.websocket_handler)
        self.app.router.add_static('/', path=os.path.dirname(__file__))

    async def serve_index(self, request):
        with open(os.path.join(os.path.dirname(__file__), 'index.html')) as f:
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

    async def run_test(self, test_id):
        # Simulate test execution
        self.current_test = test_id
        await self.broadcast({'type': 'status', 'status': f'Running test {test_id}'})

        # Simulate test progress
        for i in range(0, 101, 10):
            await asyncio.sleep(1)  # Simulate work
            await self.broadcast({
                'type': 'progress',
                'value': i
            })
            await self.broadcast({
                'type': 'log',
                'level': 'info',
                'message': f'Test progress: {i}%'
            })

        # Simulate test completion
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
