# Inventory API

Base URL: `http://localhost:8080`. All routes use JSON. Successful responses have the form `{ "success": true, "data": ... }`; failures have `{ "success": false, "message": "..." }`. Use `Authorization: Bearer <token>` on protected routes. Admin has full access. Staff can read items and record/view stock movements. CORS permits `http://localhost:5173` and `http://localhost:3000`.

For compactness, commands below use `TOKEN` as an environment variable in POSIX shells. On PowerShell, replace `$TOKEN` with the token string returned by login and use `curl.exe`.

```sh
TOKEN='<copy-token-from-login>'
```

## Health

`GET /api/health` is public.

```sh
curl http://localhost:8080/api/health
```

```json
{"success":true,"data":{"status":"ok"}}
```

## Authentication

### Register

`POST /api/auth/register`. The first account is admin; subsequent registrations require an admin token. Later registrations may provide `role` as `admin` or `staff` (defaults to admin).

```sh
curl -X POST http://localhost:8080/api/auth/register -H 'Content-Type: application/json' -d '{"name":"Store Staff","email":"staff@example.com","password":"staffpass123","role":"staff"}' -H "Authorization: Bearer $TOKEN"
```

The seed creates the initial administrator automatically. After startup, registration always requires an administrator token.

```json
{"success":true,"data":{"id":2,"name":"Store Staff","email":"staff@example.com","role":"staff","token":"<signed-token>"}}
```

### Login

`POST /api/auth/login` with `email` and `password`.

```sh
curl -X POST http://localhost:8080/api/auth/login -H 'Content-Type: application/json' -d '{"email":"admin@example.com","password":"admin12345"}'
```

```json
{"success":true,"data":{"id":1,"name":"Inventory Admin","email":"admin@example.com","role":"admin","token":"<signed-token>"}}
```

### Current user

`GET /api/auth/me` requires a valid token.

```sh
curl http://localhost:8080/api/auth/me -H "Authorization: Bearer $TOKEN"
```

## Categories (admin only)

`GET /api/categories` lists categories. `POST /api/categories` creates one with `{ "name": "Electronics" }`. `PUT /api/categories/{id}` replaces the name with the same body. `DELETE /api/categories/{id}` returns 204 on success.

```sh
curl http://localhost:8080/api/categories -H "Authorization: Bearer $TOKEN"
curl -X POST http://localhost:8080/api/categories -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' -d '{"name":"Electronics"}'
curl -X PUT http://localhost:8080/api/categories/1 -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' -d '{"name":"Office Electronics"}'
curl -X DELETE http://localhost:8080/api/categories/1 -H "Authorization: Bearer $TOKEN"
```

Example create response: `{"success":true,"data":{"id":1,"name":"Electronics"}}`.

## Suppliers (admin only)

`GET /api/suppliers` lists suppliers. Create/update accepts `name` and optional string fields `phone`, `email`, and `address`; omitted optional fields become empty strings.

```sh
curl http://localhost:8080/api/suppliers -H "Authorization: Bearer $TOKEN"
curl -X POST http://localhost:8080/api/suppliers -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' -d '{"name":"Northwind Supply","phone":"555-0101","email":"orders@northwind.example","address":"100 Market Street"}'
curl -X PUT http://localhost:8080/api/suppliers/1 -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' -d '{"name":"Northwind Supply Ltd","phone":"555-0101","email":"orders@northwind.example","address":"100 Market Street"}'
curl -X DELETE http://localhost:8080/api/suppliers/1 -H "Authorization: Bearer $TOKEN"
```

Example response: `{"success":true,"data":{"id":1,"name":"Northwind Supply","phone":"555-0101","email":"orders@northwind.example","address":"100 Market Street"}}`.

## Items

Admin can create, update, and delete items. Both roles can list and view items.

`GET /api/items` supports `q` (case-insensitive SQLite name/SKU substring search), `category_id`, `page` (default 1), and `limit` (default 20, maximum 100). The response puts `items`, `page`, `limit`, `total`, and `total_pages` inside `data`.

```sh
curl "http://localhost:8080/api/items?q=mouse&category_id=3&page=1&limit=10" -H "Authorization: Bearer $TOKEN"
curl http://localhost:8080/api/items/1 -H "Authorization: Bearer $TOKEN"
```

Create with `sku`, `name`, and non-negative `unit_price`; `category_id`, `supplier_id`, `quantity`, and `reorder_level` are optional. `category_id` and `supplier_id` may be null. SKU must be unique.

```sh
curl -X POST http://localhost:8080/api/items -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' -d '{"sku":"HD-001","name":"USB Hub","category_id":1,"supplier_id":1,"unit_price":19.99,"quantity":0,"reorder_level":4}'
curl -X PUT http://localhost:8080/api/items/1 -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' -d '{"sku":"HD-001","name":"USB Hub 4-Port","category_id":1,"supplier_id":1,"unit_price":22.99,"reorder_level":4}'
curl -X DELETE http://localhost:8080/api/items/1 -H "Authorization: Bearer $TOKEN"
```

Example item: `{"success":true,"data":{"id":1,"sku":"HD-001","name":"USB Hub","category_id":1,"supplier_id":1,"unit_price":19.99,"quantity":0,"reorder_level":4,"created_at":"2026-10-08 10:00:00"}}`.

`GET /api/items/low-stock` returns items with `quantity <= reorder_level`.

```sh
curl http://localhost:8080/api/items/low-stock -H "Authorization: Bearer $TOKEN"
```

## Stock

Admin and staff can record stock movements. Stock-in and stock-out bodies require positive integer `item_id` and `quantity`, plus optional `note` (up to 500 characters). Stock-out returns 409 when the available quantity is insufficient. The item update and movement insert are committed together.

```sh
curl -X POST http://localhost:8080/api/stock/in -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' -d '{"item_id":1,"quantity":12,"note":"supplier delivery"}'
curl -X POST http://localhost:8080/api/stock/out -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' -d '{"item_id":1,"quantity":2,"note":"store use"}'
```

Example response: `{"success":true,"data":{"item_id":1,"type":"IN","quantity":12,"new_quantity":12}}`.

`GET /api/stock/movements` returns movement history; optional `item_id` filters to one item.

```sh
curl http://localhost:8080/api/stock/movements -H "Authorization: Bearer $TOKEN"
curl "http://localhost:8080/api/stock/movements?item_id=1" -H "Authorization: Bearer $TOKEN"
```

## Dashboard

`GET /api/dashboard` returns `total_items`, `total_stock_value`, `low_stock_count`, and up to five recent movements.

```sh
curl http://localhost:8080/api/dashboard -H "Authorization: Bearer $TOKEN"
```

## Error responses

```json
{"success":false,"message":"Not enough stock to complete this stock out"}
```

The API uses 400 for invalid input, 401 for missing/invalid tokens, 403 for insufficient role, 404 for missing resources, 409 for duplicate SKU/category names or insufficient stock, and 500 for unexpected database failures. CORS preflight requests use `OPTIONS /api/{path}`.
