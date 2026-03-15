import pytest
import aiohttp

BASE_URL = "http://localhost:8080"


class Response:
    def __init__(self, status: int, body):
        self.status = status
        self._body = body

    def json(self):
        return self._body


class ServiceClient:
    def __init__(self, session: aiohttp.ClientSession):
        self._session = session

    async def post(self, path, json=None, headers=None):
        async with self._session.post(
            BASE_URL + path, json=json, headers=headers
        ) as r:
            try:
                body = await r.json(content_type=None)
            except Exception:
                body = {}
            return Response(r.status, body)

    async def get(self, path, params=None, headers=None):
        async with self._session.get(
            BASE_URL + path, params=params, headers=headers
        ) as r:
            try:
                body = await r.json(content_type=None)
            except Exception:
                body = {}
            return Response(r.status, body)


@pytest.fixture
async def service_client():
    async with aiohttp.ClientSession() as session:
        yield ServiceClient(session)
