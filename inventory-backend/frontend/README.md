# Stockroom frontend

A small static HTML, CSS, and JavaScript interface for the Inventory API. It has no frontend package dependencies or build step.

## Run it

Start the backend from the `inventory-backend` folder:

```powershell
.\build\inventory-backend.exe
```

Open [http://localhost:8080](http://localhost:8080) and sign in with the local demo administrator shown in the root README. The backend serves the frontend files from its own origin, so the browser does not need a separate frontend server or cross-origin API requests.

The API URL follows the page origin in `app.js`. The bearer token is stored in browser local storage and removed when the user signs out. This is suitable for a local college project; production applications should use a stronger session design and HTTPS.
