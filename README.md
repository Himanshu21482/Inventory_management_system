# Inventory Management System

## CS3104 — Class Project

A C++17 inventory management system with a REST API backend, an SQLite database, and a built-in web frontend called **Stockroom**. It manages inventory, products, suppliers, categories, stock movements, users, and basic inventory analytics.

The backend serves the Stockroom frontend directly at `http://localhost:8080`, so the whole system (browser → REST API → database) runs end to end with a single executable. The project is deliberately compact for a class project, and demonstrates C++ object-oriented design, REST API development, database management, authentication, authorization, and transactional inventory operations.

**Team:** Himanshu Gunwant (251127) · Raj Aryan Mahato (251145)

**Diagrams:** class, architecture, use case, sequence, and activity diagrams are in [`docs/diagrams/`](docs/diagrams/README.md).

---

## 1. Problem Statement

Inventory management becomes difficult when products, suppliers, stock quantities, and stock movements have to be tracked manually.

A simple manual or spreadsheet-based approach can result in:

- Incorrect stock quantities
- Difficulty tracking stock entering and leaving the system
- Difficulty identifying low-stock products
- Duplicate or inconsistent product information
- Lack of centralized supplier and category information
- Unauthorized modification of inventory data
- Difficulty maintaining a history of stock movements

The objective of this project is to develop an **Inventory Management System** that provides a centralized backend for storing and managing inventory information.

The system should allow authorized users to:

1. Manage product categories.
2. Manage suppliers.
3. Manage inventory items.
4. Track stock quantities.
5. Record stock-in and stock-out operations.
6. View stock movement history.
7. Identify products that have reached their low-stock level.
8. View basic inventory information through a dashboard.
9. Authenticate users and restrict operations based on their roles.

---

# 2. Project Objectives

The major objectives of the project are:

- Build a functional inventory management backend using C++.
- Design a relational database for inventory data.
- Develop RESTful API endpoints for frontend integration.
- Implement authentication and authorization.
- Maintain stock movement records.
- Prevent invalid inventory operations through transactional business logic.
- Provide low-stock and dashboard information.
- Organize the code using separate routes, services, database, middleware, and model components.
- Make the application easy to build and run locally.

---

# 3. Proposed Solution

The proposed system uses a backend REST API to provide a single point of access to inventory data.

The overall flow is:

```text
              Stockroom Frontend
            (HTML / CSS / JavaScript)
                        |
                        | HTTP + JSON
                        v
              +-------------------+
              |    REST API       |
              |      Crow         |
              +-------------------+
                        |
                        v
              +-------------------+
              |    Routes /       |
              |   HTTP Handlers   |
              +-------------------+
                        |
                        v
              +-------------------+
              |     Services      |
              | Business Logic    |
              +-------------------+
                        |
                        v
              +-------------------+
              |    Database       |
              | SQLite + SQL      |
              +-------------------+
```

The backend separates HTTP handling from inventory business logic and database operations. The full architecture diagram is in [`docs/diagrams/architecture-diagram.png`](docs/diagrams/architecture-diagram.png).

---

# 4. Technology Stack

| Technology | Purpose |
|---|---|
| C++17 | Backend programming language |
| Crow | REST API / HTTP framework |
| SQLite | Database |
| nlohmann/json | JSON request and response handling |
| CMake | Build system |
| HTML, CSS, JavaScript | Stockroom web frontend (no framework, no build step) |
| TikZ (LaTeX) | UML and architecture diagrams |
| Git | Version control |

The application creates an `inventory.db` SQLite database in the working directory and applies the database schema and seed data when the application starts.

---

# 5. Main Features

## 5.1 Authentication

The system supports user registration, login, and retrieving the currently authenticated user.

Available operations:

```text
POST /api/auth/register
POST /api/auth/login
GET  /api/auth/me
```

Authentication uses bearer tokens:

```text
Authorization: Bearer <token>
```

The first administrator account is created through the seed data.

---

## 5.2 Category Management

Administrators can manage product categories.

Operations include:

- View categories
- Create categories
- Update categories
- Delete categories

Endpoints:

```text
GET    /api/categories
POST   /api/categories
PUT    /api/categories/{id}
DELETE /api/categories/{id}
```

---

## 5.3 Supplier Management

The system stores supplier information and allows administrators to manage suppliers.

Endpoints:

```text
GET    /api/suppliers
POST   /api/suppliers
PUT    /api/suppliers/{id}
DELETE /api/suppliers/{id}
```

Supplier information can be associated with the inventory management workflow.

---

## 5.4 Item Management

Items represent the products maintained by the inventory system.

The system supports:

- Creating items
- Viewing items
- Updating items
- Deleting items
- Identifying low-stock items

Endpoints:

```text
GET    /api/items
POST   /api/items
GET    /api/items/{id}
PUT    /api/items/{id}
DELETE /api/items/{id}
GET    /api/items/low-stock
```

---

## 5.5 Stock Management

Stock operations are handled through dedicated endpoints.

```text
POST /api/stock/in
POST /api/stock/out
GET  /api/stock/movements
```

### Stock In

Used when inventory enters the system.

```text
Supplier / Purchase
       |
       v
   Stock In
       |
       v
 Inventory Quantity Increased
       |
       v
 Stock Movement Recorded
```

### Stock Out

Used when inventory leaves the system.

```text
Customer / Usage
       |
       v
   Stock Out
       |
       v
 Inventory Quantity Decreased
       |
       v
 Stock Movement Recorded
```

### Stock Movement History

Every stock operation can be inspected through the stock movement endpoint.

This provides a record of inventory changes rather than only storing the current quantity.

---

## 5.6 Low-Stock Monitoring

The system provides an endpoint for identifying items whose stock has reached their configured low-stock level.

```text
GET /api/items/low-stock
```

This allows a frontend application to display products that may need replenishment.

---

## 5.7 Dashboard

A dashboard endpoint provides a high-level view of inventory information.

```text
GET /api/dashboard
```

It returns the total number of items, the total stock value, the number of low-stock items, and the five most recent stock movements. The Stockroom Overview page is built from this endpoint.

---

# 6. Database Design

The project uses **SQLite** as its relational database.

The database schema is defined in:

```text
schema.sql
```

Sample data is defined in:

```text
seed.sql
```

The application applies these files when it starts.

The database has five tables:

| Table | Purpose |
|---|---|
| `users` | Accounts with a salted password hash and a role (`admin` or `staff`) |
| `categories` | Product categories |
| `suppliers` | Supplier contact details |
| `items` | Products: SKU, name, category, supplier, unit price, current quantity, reorder level |
| `stock_movements` | Every stock-in and stock-out: item, type, quantity, note, user, timestamp |

The current quantity lives on `items`, and `stock_movements` keeps the full history. Foreign keys link items to categories and suppliers (`ON DELETE SET NULL`), and movements to items (`ON DELETE CASCADE`) and users. `CHECK` constraints reject negative prices or quantities and invalid roles or movement types.

A simplified relationship is:

```text
Users
  |
  +---- Authentication / Authorization
  |
  +---- Stock Operations


Categories
  |
  +---- Items
           |
           +---- Stock
           |
           +---- Stock Movements


Suppliers
  |
  +---- Inventory Supply


Items
  |
  +---- Stock In / Stock Out
  |
  +---- Movement History
```

---

# 7. Inventory Workflow

The basic inventory workflow is:

```text
                    +-------------+
                    |   Supplier  |
                    +-------------+
                           |
                           v
                    +-------------+
                    |  Stock In   |
                    +-------------+
                           |
                           v
                    +-------------+
                    |  Inventory  |
                    +-------------+
                           |
              +------------+------------+
              |                         |
              v                         v
       +-------------+           +-------------+
       |  Stock Out  |           | Stock Check |
       +-------------+           +-------------+
              |                         |
              v                         v
       Movement Record            Low Stock
```

The system therefore maintains both the **current inventory state** and the **history of stock changes**.

---

# 8. Business Logic

The stock service is responsible for inventory-related business rules.

Stock operations are handled transactionally so that related database changes are kept consistent.

For example, a stock-in operation follows the general process:

```text
Receive Request
      |
      v
Validate Input
      |
      v
Begin Transaction
      |
      v
Update Stock
      |
      v
Create Stock Movement
      |
      v
Commit Transaction
```

If an operation fails, the transaction can be rolled back rather than leaving the database in an inconsistent state.

This logic is implemented in:

```text
src/services/stock_service.*
```

---

# 9. Authentication and Authorization

The system distinguishes between administrators and staff users.

### Administrator

Administrators can manage:

- Categories
- Suppliers
- Items
- Users

### Staff

Staff users can:

- View inventory items
- Record stock-in operations
- Record stock-out operations

Protected endpoints require a bearer token.

Example:

```http
Authorization: Bearer <token>
```

The middleware layer is responsible for token lookup and role checks.

---

# 10. API Overview

All API endpoints use the `/api/` prefix.

## Health

```text
GET /api/health
```

Used to check whether the backend is running.

## Authentication

```text
POST /api/auth/register
POST /api/auth/login
GET  /api/auth/me
```

## Categories

```text
GET    /api/categories
POST   /api/categories
PUT    /api/categories/{id}
DELETE /api/categories/{id}
```

## Suppliers

```text
GET    /api/suppliers
POST   /api/suppliers
PUT    /api/suppliers/{id}
DELETE /api/suppliers/{id}
```

## Items

```text
GET    /api/items
POST   /api/items
GET    /api/items/{id}
PUT    /api/items/{id}
DELETE /api/items/{id}
GET    /api/items/low-stock
```

## Stock

```text
POST /api/stock/in
POST /api/stock/out
GET  /api/stock/movements
```

## Dashboard

```text
GET /api/dashboard
```

---

# 11. API Response Format

Successful responses follow the general structure:

```json
{
    "success": true,
    "data": {}
}
```

Error responses use:

```json
{
    "success": false,
    "message": "Error message"
}
```

JSON is used for communication between the backend and frontend.

Detailed request examples and fields are available in:

```text
inventory-app/docs/api.md
```

---

# 12. Project Structure

```text
(Project Root)
│
├── README.md
├── docs/
│   └── diagrams/                 <-- UML + architecture diagrams (PNG, SVG, PDF, TikZ source)
│
└── inventory-app/                <-- C++ backend + Stockroom frontend
    ├── README.md
    ├── CMakeLists.txt
    ├── schema.sql
    ├── seed.sql
    │
    ├── docs/
    │   └── api.md                <-- API reference with request examples
    │
    ├── frontend/                 <-- Stockroom web UI, served by the backend
    │   ├── README.md
    │   ├── index.html
    │   ├── styles.css
    │   └── app.js
    │
    └── src/
        ├── main.cpp
        ├── routes/
        │   ├── auth.hpp
        │   ├── categories.hpp
        │   ├── suppliers.hpp
        │   ├── items.hpp
        │   ├── stock.hpp
        │   └── dashboard.hpp
        ├── services/
        │   ├── stock_service.hpp
        │   └── stock_service.cpp
        ├── database/
        │   ├── database.hpp
        │   ├── database.cpp
        │   └── sqlite.hpp
        ├── middleware/
        │   ├── cors.hpp
        │   ├── auth.hpp
        │   └── authorization.hpp
        ├── models/
        │   └── ...
        └── utils/
            ├── password.hpp
            ├── response.hpp
            └── validation.hpp
```

### Important Components

**`src/main.cpp`**

Opens the database, applies the schema and seed data, registers the API routes, serves the Stockroom frontend files, and starts the server on port 8080.

**`src/routes/`**

Contains HTTP handlers for the different resources.

**`src/services/`**

Contains business logic, particularly inventory and stock operations.

**`src/database/`**

Handles SQLite connections, database operations, prepared statements, and transactions.

**`src/middleware/`**

Contains the CORS middleware, the `TokenStore` used for bearer-token authentication, and the role checks used for authorization.

**`src/models/`**

Placeholder headers for the domain models (category, item, stock movement, supplier, user).

**`src/utils/`**

Password hashing (salted SHA-256), JSON response helpers, and input validation.

**`frontend/`**

The Stockroom web interface: sign-in, overview dashboard, items, stock movements, categories, suppliers, and team management.

**`schema.sql`**

Defines the database structure.

**`seed.sql`**

Provides sample data for local development and demonstration.

---

# 13. Steps Followed to Solve the Problem

The problem was approached incrementally rather than implementing all functionality at once.

## Step 1 — Identify Requirements

The first step was to identify the core entities and operations required by an inventory system.

The main entities identified were:

```text
Users
Categories
Suppliers
Items
Stock
Stock Movements
```

The major operations were:

```text
Authentication
CRUD Operations
Stock In
Stock Out
Low Stock Detection
Dashboard
```

---

## Step 2 — Select the Technology Stack

C++17 was selected as the primary programming language.

The following supporting technologies were selected:

```text
C++17
   +
Crow
   +
SQLite
   +
nlohmann/json
   +
CMake
```

This combination provides a lightweight backend suitable for a class project.

---

## Step 3 — Design the Database

The database schema was designed before implementing the API.

The database stores persistent information such as:

- Users
- Categories
- Suppliers
- Items
- Stock
- Stock movement history

The schema is stored in `schema.sql`.

Sample records are stored in `seed.sql`.

---

## Step 4 — Implement the Database Layer

A database layer was created to isolate SQLite-specific operations from the rest of the application.

This layer handles:

- Database connections
- SQL execution
- Prepared statements
- Transactions

This separation makes the rest of the backend independent of low-level database operations.

---

## Step 5 — Implement the Business Logic

The stock service was created to handle inventory-specific operations.

Instead of allowing route handlers to directly manipulate stock, the request is passed to the service layer.

For example:

```text
HTTP Request
     |
     v
Stock Route
     |
     v
Stock Service
     |
     v
Database
```

This keeps business rules separate from HTTP handling.

---

## Step 6 — Implement REST API Routes

Routes were then created for each major feature:

```text
Authentication
Categories
Suppliers
Items
Stock
Dashboard
```

Each route receives JSON input, performs the required operation, and returns a JSON response.

---

## Step 7 — Add Authentication and Authorization

Authentication was added so that users can log in and receive a token.

Middleware checks the token for protected routes.

Role checks are then used to restrict administrative operations.

---

## Step 8 — Add Seed Data

Sample data was added to make the application immediately usable for demonstration.

The seed data includes:

- An administrator account
- Sample categories
- A supplier
- Sample items
- Opening stock movements

Seeding is designed to be repeatable.

---

## Step 9 — Build the Stockroom Frontend

A static HTML/CSS/JavaScript frontend was built and served by the backend itself, which connects the browser, the REST API, and the database end to end.

---

## Step 10 — Test the API

The API can be tested using tools such as:

```text
curl
Postman
Stockroom frontend (browser)
```

The health endpoint can first be used to confirm that the server is running.

```bash
curl http://localhost:8080/api/health
```

Then authentication and protected endpoints can be tested using the generated token.

---

# 14. Installation Requirements

The project requires:

- CMake 3.20 or newer
- C++17 compiler
- Git
- Internet access during the first CMake configuration
- SQLite 3 development files

For Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake git libsqlite3-dev
```

For Windows, Visual Studio Build Tools with the Desktop C++ workload, CMake, Git, and SQLite through vcpkg can be used.

---

# 15. Building the Project

## Linux / macOS

First, navigate to the backend directory:
```bash
cd inventory-app
```

Configure the project:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Build:

```bash
cmake --build build --parallel
```

Run:

```bash
./build/inventory-backend
```

The executable should be run from the project directory so that it can locate:

```text
schema.sql
seed.sql
```

---

## Windows

First, navigate to the backend directory:
```powershell
cd inventory-app
```

Using vcpkg:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release
```

Build:

```powershell
cmake --build build --config Release
```

Run:

```powershell
.\build\Release\inventory-backend.exe
```

---

# 16. Running the Application

The server runs on:

```text
http://localhost:8080
```

Open this address in a browser to use the Stockroom frontend. Run the executable from the `inventory-app` folder so it can find `schema.sql`, `seed.sql`, and the `frontend/` files.

To check whether the backend is running:

```bash
curl http://localhost:8080/api/health
```

A successful response confirms that the API server is available.

---

# 17. Sample Login

The seed data creates a local administrator account for demonstration.

| Email | Password | Role |
|---|---|---|
| `admin@example.com` | `admin12345` | admin |

Example login request:

```bash
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "email": "admin@example.com",
    "password": "admin12345"
  }'
```

The returned token can then be used for protected requests:

```bash
curl http://localhost:8080/api/items \
  -H "Authorization: Bearer <token>"
```

The credentials are intended only for local college-project demonstrations and should not be used for production systems.

---

# 18. Frontend (Stockroom)

The project includes a working web frontend, **Stockroom**, in `inventory-app/frontend/`. It is plain HTML, CSS, and JavaScript, so it has no package dependencies or build step. The C++ backend serves it from the same origin as the API, so there is no separate frontend server and no cross-origin requests.

Pages:

| Page | What it does | API used |
|---|---|---|
| Sign in | Log in with email and password | `POST /api/auth/login` |
| Overview | Totals, stock value, low-stock count, recent movements | `GET /api/dashboard` |
| Items | Search, filter by category, paginate, add, edit, delete items | `/api/items` |
| Stock movements | Movement history and the "Record stock" dialog | `/api/stock/*` |
| Categories / Suppliers | Manage master data (admin) | `/api/categories`, `/api/suppliers` |
| Team | Register admin or staff accounts (admin) | `POST /api/auth/register` |

End-to-end request flow for a protected operation:

```text
Stockroom (browser)
  |
  | HTTP + JSON, Authorization: Bearer <token>
  v
Crow app + CORS middleware
  |
  v
Route handler  -->  TokenStore (authentication) + role check
  |
  v
StockService (business rules, transaction)
  |
  v
SQLite (inventory.db)
```

The bearer token is kept in the browser's local storage and removed when the user signs out. See `inventory-app/frontend/README.md` for details.

---

# 19. Testing the Main Workflow

A basic demonstration workflow can be performed as follows:

### 1. Start the backend

```bash
cd inventory-app
./build/inventory-backend
```

You can follow the same workflow in the browser at `http://localhost:8080`.

### 2. Check the server

```bash
curl http://localhost:8080/api/health
```

### 3. Login as administrator

```text
POST /api/auth/login
```

### 4. Retrieve items

```text
GET /api/items
```

### 5. Add stock

```text
POST /api/stock/in
```

### 6. Check stock movements

```text
GET /api/stock/movements
```

### 7. Remove stock

```text
POST /api/stock/out
```

### 8. Check low-stock products

```text
GET /api/items/low-stock
```

### 9. View dashboard information

```text
GET /api/dashboard
```

This demonstrates the main inventory lifecycle implemented by the project.

---

# 20. Security and Project Limitations

This project is designed primarily as a **CS3104 academic project and local demonstration system**, rather than a production-ready inventory platform.

The current implementation uses:

- Salted SHA-256 password hashing
- HMAC-SHA-256 signed tokens
- Process-local token storage
- SQLite
- A compact single-process architecture

Secrets are not read from environment variables yet: the token-signing secret is generated randomly when the server starts (so tokens are lost on restart), and the demo administrator password is fixed in `seed.sql`. Reading secrets with `std::getenv` is listed under Future Improvements.

For a production deployment, stronger password hashing such as Argon2 or bcrypt, persistent token management, more comprehensive access control, and a production database would be appropriate.

---

# 21. Future Improvements

The current system can be extended with additional functionality.

Possible improvements include:

### Inventory

- Multiple warehouse locations
- Stock transfers
- Inventory valuation
- Purchase orders
- Sales orders
- Automatic reorder suggestions

### Authentication

- Read secrets (token-signing key, initial admin password) from environment variables with `std::getenv`
- Refresh tokens
- Password reset
- Persistent sessions
- Stronger password hashing
- More granular permissions

### Reporting

- Sales reports
- Purchase reports
- Inventory valuation
- Historical stock charts
- Export to CSV/PDF

### Frontend

- Inventory charts and trends
- Low-stock notifications
- CSV/PDF export from the UI
- Mobile-friendly layouts

### Deployment

The system could also be extended with:

- Docker
- PostgreSQL
- Cloud deployment
- CI/CD
- Automated testing

---

# 22. Learning Outcomes

This project demonstrates several important concepts relevant to computer science and software engineering.

### C++ and Object-Oriented Programming

- C++17 programming
- Classes and objects, encapsulation, and abstraction
- RAII (`Database`, `sql::Transaction`, statement deleters)
- Composition and aggregation
- Function overloading and template-based (static) polymorphism
- Modular programming
- Exception/error handling
- Resource management

### Database Management

- Relational database design
- SQL
- SQLite
- Prepared statements
- Transactions
- Database constraints

### Web Development

- REST APIs
- HTTP methods
- JSON
- Authentication
- Authorization
- Middleware

### Software Engineering

- Layered architecture
- Separation of concerns
- Version control
- Build systems
- API documentation
- UML modelling (class, use case, sequence, activity diagrams)

---

# 23. Conclusion

The Inventory Management System provides a centralized backend for managing the basic operations involved in inventory tracking.

The system allows users to manage:

```text
Users
  ↓
Categories
  ↓
Suppliers
  ↓
Items
  ↓
Stock
  ↓
Stock Movements
  ↓
Reports / Dashboard
```

The project combines C++ programming, database management, REST API development, a web frontend, authentication, authorization, and transactional business logic into a single practical application.

As a CS3104 class project, it demonstrates how a real-world problem can be broken down into smaller software components and implemented using a structured backend architecture.

---

## Project Information

**Course:** CS3104  
**Project:** Inventory Management System  
**Team:** Himanshu Gunwant (251127), Raj Aryan Mahato (251145)  
**Frontend:** Stockroom (HTML, CSS, JavaScript)  
**Language:** C++17  
**Backend:** Crow  
**Database:** SQLite  
**API Format:** REST + JSON