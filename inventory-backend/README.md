# Inventory Management System — Backend

A small REST API for tracking products, suppliers, stock changes, and low-stock alerts. It is written in C++17 and uses Crow, SQLite, and nlohmann/json. The API listens on port **8080** and is designed to be consumed by a separate React frontend.

## Features

- Admin and staff accounts with bearer-token authentication
- Category, supplier, and item management
- Item search by name or SKU, category filtering, and pagination
- Stock-in and stock-out with movement history
- Transactional stock updates that reject stock-outs which would make inventory negative
- Low-stock list and dashboard totals with the five latest movements
- CORS access for `http://localhost:5173` and `http://localhost:3000`

## Requirements

- CMake 3.20 or later
- A C++17 compiler (GCC, Clang, or MinGW-w64)
- Ninja or another CMake-supported build tool
- SQLite 3 development headers and library
- Git and internet access for the first configure; CMake fetches Crow, standalone Asio, and nlohmann/json

On Debian or Ubuntu, install the compiler, CMake, Git, and SQLite development package:

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build git libsqlite3-dev
```

On Windows, install CMake, Git, Ninja, a C++17 toolchain, and SQLite development files. With vcpkg, install SQLite using a triplet compatible with your compiler, for example:

```powershell
vcpkg install sqlite3 --triplet x64-mingw-dynamic
```

## Build and run

Run these commands from the `inventory-backend` directory. Use the same CMake generator for a given build directory. The commands below use Ninja:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

If the existing `build` directory was already configured with Ninja, keep using it:

```powershell
cmake -S . -B build -G Ninja
cmake --build build --parallel
```

Start the server from this directory so it can find `schema.sql` and `seed.sql`:

```powershell
.\build\inventory-backend.exe
```

With a multi-configuration generator such as Visual Studio, the executable is usually under `build\Release`:

```powershell
.\build\Release\inventory-backend.exe
```

On Linux, the executable is `./build/inventory-backend`. The first configure downloads the CMake dependencies, so it requires network access. If CMake reports that it cannot find SQLite, install SQLite development files and re-run the configure command.

Verify the server is running:

```powershell
curl.exe http://localhost:8080/api/health
```

Expected response:

```json
{"success":true,"data":{"status":"ok"}}
```

## Local demo account

The app applies `schema.sql` and then the idempotent `seed.sql` at startup. The seed creates an administrator for local demonstrations:

| Email | Password | Role |
|---|---|---|
| `admin@example.com` | `admin12345` | `admin` |

These credentials are for local testing only. The seed also adds three categories, a supplier, three sample items, and opening stock movements. Do not use the demo password outside a local project demo.

Get a token by logging in:

```powershell
curl.exe -X POST http://localhost:8080/api/auth/login -H "Content-Type: application/json" --data-raw '{"email":"admin@example.com","password":"admin12345"}'
```

Copy the `data.token` value from the response and pass it to protected routes as `Authorization: Bearer <token>`.

## API endpoints

All routes are under `/api/`. Protected routes require a bearer token. Responses use `{ "success": true, "data": ... }` for success and `{ "success": false, "message": "..." }` for errors.

| Area | Endpoints | Access |
|---|---|---|
| Health | `GET /api/health` | Public |
| Auth | `POST /api/auth/register`, `POST /api/auth/login`, `GET /api/auth/me` | Login/register public initially; `/me` requires token; after seed, registration requires admin |
| Categories | `GET /api/categories`, `POST /api/categories`, `PUT /api/categories/{id}`, `DELETE /api/categories/{id}` | Admin |
| Suppliers | `GET /api/suppliers`, `POST /api/suppliers`, `PUT /api/suppliers/{id}`, `DELETE /api/suppliers/{id}` | Admin |
| Items | `GET /api/items`, `GET /api/items/{id}`, `POST /api/items`, `PUT /api/items/{id}`, `DELETE /api/items/{id}`, `GET /api/items/low-stock` | Read: admin/staff; changes: admin |
| Stock | `POST /api/stock/in`, `POST /api/stock/out`, `GET /api/stock/movements` | Admin/staff |
| Dashboard | `GET /api/dashboard` | Admin/staff |

Item listing accepts `q`, `category_id`, `page`, and `limit` query parameters. Movement history accepts an optional `item_id`. Full request and response examples are in [docs/api.md](docs/api.md).

## Project structure

```text
src/
├── database/      SQLite connection, prepared statements, transactions
├── middleware/    CORS, token lookup, role checks
├── models/        Domain model headers
├── routes/        HTTP handlers by resource
├── services/      Stock business rules
├── utils/         Response and password helpers
└── main.cpp       Startup, schema/seed initialization, route registration
schema.sql         Database tables and indexes
seed.sql           Local sample data and demo administrator
docs/api.md        Endpoint details and curl examples
```

## Project-level limitations

- Tokens expire after 24 hours and are kept in memory; a server restart invalidates them.
- Passwords use salted SHA-256 to avoid adding a password-hashing dependency. The code comments recommend Argon2 or bcrypt for production use.
- This is a small single-process educational backend. The seeded credentials and simplified authentication are intended for local demonstrations, not deployment with real user data.
