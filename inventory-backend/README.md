# Inventory Management Backend

A small C++17 REST API for inventory tracking. It uses Crow for HTTP, SQLite for storage, and nlohmann/json for request parsing and JSON responses. The database file (`inventory.db`) is created in the current working directory; the application applies `schema.sql` and the idempotent `seed.sql` when it starts.

## Requirements

- CMake 3.20 or newer
- A C++17 compiler (GCC 9+, Clang 10+, or Visual Studio 2019/2022)
- Git and internet access during the first CMake configure (Crow and nlohmann/json are fetched by CMake)
- SQLite 3 development files

On Debian/Ubuntu:

```sh
sudo apt update
sudo apt install build-essential cmake git libsqlite3-dev
```

On Windows, install Visual Studio Build Tools with the Desktop C++ workload, CMake, Git, and vcpkg. Install SQLite with `vcpkg install sqlite3`, then configure CMake with the vcpkg toolchain file shown below.

## Build and run

Linux/macOS with SQLite development files available to CMake:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/inventory-backend
```

Windows PowerShell with vcpkg:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\inventory-backend.exe
```

Run the executable from this directory so it can read `schema.sql` and `seed.sql`. The server listens on port **8080**. Check startup with:

```sh
curl http://localhost:8080/api/health
```

PowerShell users can run `curl.exe` instead of `curl`.

## Local seed login

The seed creates this local-only administrator account:

| Email | Password | Role |
|---|---|---|
| `admin@example.com` | `admin12345` | admin |

The password is included only for college-project demos. Change it before using the app for anything beyond local testing. Sample categories, a supplier, three items, and their opening stock movements are also seeded. Seeding is idempotent and runs each time the application starts.

## API overview

Every API route is under `/api/`. Send and receive JSON. Successful responses use `{ "success": true, "data": ... }`; errors use `{ "success": false, "message": "..." }`. Protected routes accept `Authorization: Bearer <token>`. The first account is seeded as admin; subsequent registrations require an admin token. Tokens expire after 24 hours and are held in memory, so restarting the server invalidates them.

| Area | Endpoints |
|---|---|
| Health | `GET /api/health` |
| Auth | `POST /api/auth/register`, `POST /api/auth/login`, `GET /api/auth/me` |
| Categories (admin) | `GET/POST /api/categories`, `PUT/DELETE /api/categories/{id}` |
| Suppliers (admin) | `GET/POST /api/suppliers`, `PUT/DELETE /api/suppliers/{id}` |
| Items | `GET/POST /api/items`, `GET/PUT/DELETE /api/items/{id}`, `GET /api/items/low-stock` |
| Stock | `POST /api/stock/in`, `POST /api/stock/out`, `GET /api/stock/movements` |
| Dashboard | `GET /api/dashboard` |

Staff can view items and record stock in/out. Admin credentials are needed to manage categories, suppliers, and items. See [docs/api.md](docs/api.md) for request examples, fields, and responses.

## Project layout

- `src/main.cpp` starts the server and registers routes.
- `src/routes/` contains HTTP handlers for each resource.
- `src/services/stock_service.*` implements transactional stock rules.
- `src/database/` owns the SQLite connection, prepared-statement helpers, and transaction helper.
- `src/middleware/` contains CORS, token lookup, and role checks.
- `src/models/` contains the simple domain model file locations.
- `schema.sql` defines tables and indexes; `seed.sql` provides local sample data.

## Notes

Passwords use salted SHA-256 to keep this student project dependency-light. The code comments identify Argon2/bcrypt as the production choice. Tokens are signed with HMAC-SHA-256 using a process-local secret. This is intentionally a compact single-process design, not a production authentication system.
