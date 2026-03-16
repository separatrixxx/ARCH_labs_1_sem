import os
import re
import aiohttp
import pytest

_USE_GATEWAY = os.getenv("USE_GATEWAY", "0") == "1"

_GATEWAY_URL = "http://localhost:8080"
_ROUTES = [
    (re.compile(r"^/api/v1/(auth|users|notifications)"), "http://localhost:8081"),
    (re.compile(r"^/api/v1/services"),                   "http://localhost:8082"),
    (re.compile(r"^/api/v1/orders"),                     "http://localhost:8083"),
    (re.compile(r"^/(swagger|openapi\.yaml)"),           "http://localhost:8081"),
]


def _resolve(path: str) -> str:
    if _USE_GATEWAY:
        return _GATEWAY_URL + path
    
    for pattern, base in _ROUTES:
        if pattern.match(path):
            return base + path
        
    return "http://localhost:8081" + path


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
            _resolve(path), json=json, headers=headers
        ) as r:
            try:
                body = await r.json(content_type=None)
            except Exception:
                body = {}
            return Response(r.status, body)

    async def get(self, path, params=None, headers=None):
        async with self._session.get(
            _resolve(path), params=params, headers=headers
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
