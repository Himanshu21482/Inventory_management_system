# Inventory Management System

## CS3104 — Class Project

A C++17-based backend system for managing inventory, products, suppliers, categories, stock movements, users, and basic inventory analytics.

The project provides a REST API that can be consumed by a frontend application such as React. The backend is intentionally kept compact and suitable for a class project while demonstrating important concepts in C++, REST API development, database management, authentication, authorization, and transactional inventory operations.

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
                    Frontend
                  (React / Other)
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

The backend separates HTTP handling from inventory business logic and database operations.

---

# 4. Technology Stack

| Technology | Purpose |
|---|---|
| C++17 | Backend programming language |
| Crow | REST API / HTTP framework |
| SQLite | Database |
| nlohmann/json | JSON request and response handling |
| CMake | Build system |
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

This can be used by a frontend to create an inventory dashboard containing summary information.

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

The database contains the information required for the main inventory operations, including users, categories, suppliers, items, stock, and stock movements.

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
docs/api.md
```

---

# 12. Project Structure

```text
(Project Root)
│
├── inventory-backend/           <-- C++ Backend Application
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── schema.sql
│   ├── seed.sql
│   │
│   ├── src/
│   │   ├── main.cpp
│   │   │
│   │   ├── routes/
│   │   │   ├── auth.hpp
│   │   │   ├── categories.hpp
│   │   │   ├── suppliers.hpp
│   │   │   ├── items.hpp
│   │   │   ├── stock.hpp
│   │   │   └── dashboard.hpp
│   │   │
│   │   ├── services/
│   │   │   └── stock_service.*
│   │   │
│   │   ├── database/
│   │   │   ├── database.*
│   │   │   └── ...
│   │   │
│   │   ├── middleware/
│   │   │   ├── cors.*
│   │   │   ├── auth.*
│   │   │   └── role.*
│   │   │
│   │   └── models/
│   │       └── ...
│   │
│   └── docs/
│       └── api.md
│
└── inventory-system/            <-- Frontend Application (To be developed)
```

### Important Components

**`src/main.cpp`**

Starts the application and registers the API routes.

**`src/routes/`**

Contains HTTP handlers for the different resources.

**`src/services/`**

Contains business logic, particularly inventory and stock operations.

**`src/database/`**

Handles SQLite connections, database operations, prepared statements, and transactions.

**`src/middleware/`**

Contains middleware for CORS, authentication, token lookup, and role checks.

**`src/models/`**

Contains the domain model definitions.

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

## Step 9 — Test the API

The API can be tested using tools such as:

```text
curl
Postman
Browser
React frontend
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
cd inventory-backend
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
cd inventory-backend
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

# 18. Frontend Integration

The backend has been designed so that a frontend can be developed separately.

A React application can communicate with the backend using HTTP requests.

For example:

```text
React
  |
  | GET /api/items
  v
C++ Backend
  |
  v
SQLite
```

For a protected operation:

```text
React
  |
  | Authorization: Bearer <token>
  v
Authentication Middleware
  |
  v
Route
  |
  v
Service
  |
  v
Database
```

The frontend can therefore be developed independently from the backend.

---

# 19. Testing the Main Workflow

A basic demonstration workflow can be performed as follows:

### 1. Start the backend

```bash
cd inventory-backend
./build/inventory-backend
```

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

A React frontend could provide:

- Admin dashboard
- Product management
- Supplier management
- Stock management
- Low-stock alerts
- Inventory charts
- User management

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

### C++

- C++17 programming
- Classes and objects
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

The project combines C++ programming, database management, REST API development, authentication, authorization, and transactional business logic into a single practical application.

As a CS3104 class project, it demonstrates how a real-world problem can be broken down into smaller software components and implemented using a structured backend architecture.

---

## Project Information

**Course:** CS3104  
**Project:** Inventory Management System  
**Language:** C++17  
**Backend:** Crow  
**Database:** SQLite  
**API Format:** REST + JSON