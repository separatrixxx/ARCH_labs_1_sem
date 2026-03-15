import uuid

import pytest


async def register(client, login, first_name="Ivan", last_name="Petrov", password="password123"):
    return await client.post(
        "/api/v1/auth/register",
        json={"login": login, "first_name": first_name,
              "last_name": last_name, "password": password},
    )


async def login(client, login_str, password="password123"):
    return await client.post(
        "/api/v1/auth/login",
        json={"login": login_str, "password": password},
    )


async def get_token(client, user_login="testuser"):
    await register(client, user_login)
    resp = await login(client, user_login)
    return resp.json()["token"]


@pytest.mark.asyncio
async def test_register_success(service_client):
    unique_login = f"user_reg_{uuid.uuid4().hex[:8]}"
    resp = await register(service_client, unique_login)
    assert resp.status == 201
    body = resp.json()
    assert body["login"] == unique_login
    assert "id" in body
    assert "password_hash" not in body


@pytest.mark.asyncio
async def test_register_duplicate(service_client):
    await register(service_client, "dup_user")
    resp = await register(service_client, "dup_user")
    assert resp.status == 409
    assert "error" in resp.json()


@pytest.mark.asyncio
async def test_register_missing_fields(service_client):
    resp = await service_client.post(
        "/api/v1/auth/register",
        json={"login": "x"},
    )
    assert resp.status == 400


@pytest.mark.asyncio
async def test_login_success(service_client):
    await register(service_client, "login_ok_user")
    resp = await login(service_client, "login_ok_user")
    assert resp.status == 200
    assert "token" in resp.json()


@pytest.mark.asyncio
async def test_login_wrong_password(service_client):
    await register(service_client, "login_bad_user")
    resp = await service_client.post(
        "/api/v1/auth/login",
        json={"login": "login_bad_user", "password": "wrongpass"},
    )
    assert resp.status == 401


@pytest.mark.asyncio
async def test_login_unknown_user(service_client):
    resp = await service_client.post(
        "/api/v1/auth/login",
        json={"login": "ghost", "password": "x"},
    )
    assert resp.status == 401


@pytest.mark.asyncio
async def test_find_user_by_login(service_client):
    token = await get_token(service_client, "find_by_login_user")
    resp = await service_client.get(
        "/api/v1/users",
        params={"login": "find_by_login_user"},
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 200
    users = resp.json()
    assert len(users) == 1
    assert users[0]["login"] == "find_by_login_user"


@pytest.mark.asyncio
async def test_find_user_by_name_mask(service_client):
    token = await get_token(service_client, "mask_user")
    await register(service_client, "mask_user2", first_name="Ivan", last_name="Sidorov")
    resp = await service_client.get(
        "/api/v1/users",
        params={"last_name": "Sidorov"},
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 200
    assert any(u["last_name"] == "Sidorov" for u in resp.json())


@pytest.mark.asyncio
async def test_users_no_auth(service_client):
    resp = await service_client.get("/api/v1/users", params={"login": "x"})
    assert resp.status == 401


@pytest.mark.asyncio
async def test_users_no_params(service_client):
    token = await get_token(service_client, "no_params_user")
    resp = await service_client.get(
        "/api/v1/users",
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 400


@pytest.mark.asyncio
async def test_create_service(service_client):
    token = await get_token(service_client, "svc_creator")
    resp = await service_client.post(
        "/api/v1/services",
        json={"title": "House cleaning", "description": "Deep clean", "price": 2500.0},
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 201
    body = resp.json()
    assert body["title"] == "House cleaning"
    assert body["price"] == 2500.0
    assert "id" in body


@pytest.mark.asyncio
async def test_create_service_no_auth(service_client):
    resp = await service_client.post("/api/v1/services", json={"title": "Test"})
    assert resp.status == 401


@pytest.mark.asyncio
async def test_list_services(service_client):
    token = await get_token(service_client, "list_svc_user")
    await service_client.post(
        "/api/v1/services",
        json={"title": "Plumbing", "description": "Fix pipes", "price": 1000.0},
        headers={"Authorization": f"Bearer {token}"},
    )
    resp = await service_client.get("/api/v1/services")
    assert resp.status == 200
    assert isinstance(resp.json(), list)
    assert any(s["title"] == "Plumbing" for s in resp.json())


@pytest.mark.asyncio
async def test_get_service_by_id(service_client):
    token = await get_token(service_client, "svc_by_id_user")
    create_resp = await service_client.post(
        "/api/v1/services",
        json={"title": "Tutoring", "price": 500.0},
        headers={"Authorization": f"Bearer {token}"},
    )
    svc_id = create_resp.json()["id"]
    resp = await service_client.get(f"/api/v1/services/{svc_id}")
    assert resp.status == 200
    assert resp.json()["id"] == svc_id


@pytest.mark.asyncio
async def test_get_service_by_id_not_found(service_client):
    resp = await service_client.get("/api/v1/services/99999")
    assert resp.status == 404


@pytest.mark.asyncio
async def test_create_order(service_client):
    token = await get_token(service_client, "order_creator")
    resp = await service_client.post(
        "/api/v1/orders",
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 201
    body = resp.json()
    assert body["status"] == "pending"
    assert body["service_ids"] == []


@pytest.mark.asyncio
async def test_get_order_by_id(service_client):
    token = await get_token(service_client, "order_by_id_user")
    create_resp = await service_client.post(
        "/api/v1/orders",
        headers={"Authorization": f"Bearer {token}"},
    )
    order_id = create_resp.json()["id"]
    resp = await service_client.get(
        f"/api/v1/orders/{order_id}",
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 200
    assert resp.json()["id"] == order_id


@pytest.mark.asyncio
async def test_get_order_by_id_not_found(service_client):
    token = await get_token(service_client, "order_notfound_user")
    resp = await service_client.get(
        "/api/v1/orders/99999",
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 404


@pytest.mark.asyncio
async def test_add_service_to_order(service_client):
    token = await get_token(service_client, "order_svc_user")
    svc_resp = await service_client.post(
        "/api/v1/services",
        json={"title": "Tutoring", "price": 800.0},
        headers={"Authorization": f"Bearer {token}"},
    )
    svc_id = svc_resp.json()["id"]
    order_resp = await service_client.post(
        "/api/v1/orders",
        headers={"Authorization": f"Bearer {token}"},
    )
    order_id = order_resp.json()["id"]
    resp = await service_client.post(
        f"/api/v1/orders/{order_id}/services",
        json={"service_id": svc_id},
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 200
    assert svc_id in resp.json()["service_ids"]


@pytest.mark.asyncio
async def test_add_service_to_nonexistent_order(service_client):
    token = await get_token(service_client, "nonexist_order_user")
    resp = await service_client.post(
        "/api/v1/orders/99999/services",
        json={"service_id": "1"},
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 404


@pytest.mark.asyncio
async def test_get_orders(service_client):
    token = await get_token(service_client, "get_orders_user")
    await service_client.post(
        "/api/v1/orders",
        headers={"Authorization": f"Bearer {token}"},
    )
    resp = await service_client.get(
        "/api/v1/orders",
        headers={"Authorization": f"Bearer {token}"},
    )
    assert resp.status == 200
    assert isinstance(resp.json(), list)
    assert len(resp.json()) >= 1


@pytest.mark.asyncio
async def test_orders_no_auth(service_client):
    resp = await service_client.get("/api/v1/orders")
    assert resp.status == 401
