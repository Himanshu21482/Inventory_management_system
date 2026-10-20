const API = `${window.location.origin}/api`;
const state = { token: localStorage.getItem("inventory_token") || "", user: null, page: "dashboard", items: [], categories: [], suppliers: [], pageNumber: 1, pageSize: 10, totalPages: 1, query: "", categoryFilter: "", movementType: "IN", toastTimer: null };
const $ = (selector, root = document) => root.querySelector(selector);
const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];
const esc = (value = "") => String(value).replace(/[&<>"']/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);
const money = value => new Intl.NumberFormat("en-IN", { style: "currency", currency: "INR" }).format(Number(value || 0));
const dateText = value => value ? new Date(value.replace(" ", "T") + (value.endsWith("Z") ? "" : "Z")).toLocaleString([], { month: "short", day: "numeric", hour: "numeric", minute: "2-digit" }) : "—";

async function api(path, options = {}) {
  const headers = { ...(options.body ? { "Content-Type": "application/json" } : {}), ...(state.token ? { Authorization: `Bearer ${state.token}` } : {}), ...options.headers };
  let response;
  try { response = await fetch(`${API}${path}`, { ...options, headers }); }
  catch { throw new Error("Cannot reach the API. Start inventory-backend.exe and try again."); }
  if (response.status === 204) return null;
  const result = await response.json().catch(() => ({}));
  if (!response.ok || result.success === false) {
    if (response.status === 401 && state.token) logout(false);
    throw new Error(result.message || `Request failed (${response.status})`);
  }
  return result.data;
}
function notify(message) { const toast = $("#toast"); toast.textContent = message; toast.classList.add("show"); clearTimeout(state.toastTimer); state.toastTimer = setTimeout(() => toast.classList.remove("show"), 2800); }
function showMessage(id, message = "") { $(id).textContent = message; }
function showLogin(message = "") { $("#loginView").classList.remove("hidden"); $("#appView").classList.add("hidden"); showMessage("#loginMessage", message); }
function logout(show = true) { localStorage.removeItem("inventory_token"); state.token = ""; state.user = null; if (show) showLogin(); }
function setRoleUi() { const admin = state.user?.role === "admin"; $("#adminNav").classList.toggle("hidden", !admin); $("#userName").textContent = state.user?.name || "Inventory User"; $("#userRole").textContent = state.user?.role || "staff"; $("#userAvatar").textContent = (state.user?.name || "I").trim().charAt(0).toUpperCase(); }
function fmtDateToday() { $("#todayLabel").textContent = new Intl.DateTimeFormat(undefined, { weekday: "short", month: "short", day: "numeric" }).format(new Date()); }

async function boot() {
  if (!state.token) return showLogin();
  try { state.user = await api("/auth/me"); setRoleUi(); showApp(); await loadPage(state.page); }
  catch (error) { if (!state.token) return; showLogin(error.message); }
}
function showApp() { $("#loginView").classList.add("hidden"); $("#appView").classList.remove("hidden"); fmtDateToday(); }
async function loadPage(page) {
  state.page = page; state.pageNumber = 1;
  $$(".nav-item").forEach(button => button.classList.toggle("active", button.dataset.page === page));
  const labels = { dashboard: "Overview", items: "Items", movements: "Stock movements", categories: "Categories", suppliers: "Suppliers", users: "Team" };
  $("#pageHeading").textContent = labels[page] || "Overview";
  const content = $("#pageContent"); content.innerHTML = '<div class="empty-state">Loading your inventory…</div>';
  try {
    if (page === "dashboard") await renderDashboard();
    else if (page === "items") await renderItems();
    else if (page === "movements") await renderMovements();
    else if (page === "categories") await renderCategories();
    else if (page === "suppliers") await renderSuppliers();
    else if (page === "users") renderUsers();
  } catch (error) { content.innerHTML = `<div class="error-box">${esc(error.message)}</div>`; }
}
function intro(kicker, title, subtitle, action = "") { return `<div class="page-intro"><div><p class="eyebrow">${kicker}</p><h1>${title}</h1><p>${subtitle}</p></div>${action ? `<div class="page-actions">${action}</div>` : ""}</div>`; }
function recordButton(label = "Record stock") { return `<button class="button primary" data-action="record">＋ ${label}</button>`; }

async function renderDashboard() {
  const [summary, itemsData] = await Promise.all([api("/dashboard"), api("/items?page=1&limit=5")]);
  const low = await api("/items/low-stock");
  const lowList = Array.isArray(low) ? low : [];
  $("#navLowStock").textContent = lowList.length; $("#navLowStock").classList.toggle("hidden", lowList.length === 0);
  const movements = summary.recent_movements || [];
  const dateCaption = `${new Intl.DateTimeFormat("en", { weekday: "long" }).format(new Date()).toUpperCase()}, YOUR INVENTORY AT A GLANCE`;
  $("#pageContent").innerHTML = `${intro(dateCaption, "Overview", "A quick look at your stockroom today.", recordButton())}
    <section class="stat-grid">
      <article class="stat-card"><div class="stat-top">Total items <span class="stat-icon">▤</span></div><div class="stat-value">${Number(summary.total_items || 0).toLocaleString()}</div><div class="stat-foot">Items in your catalog</div></article>
      <article class="stat-card"><div class="stat-top">Stock value <span class="stat-icon">＄</span></div><div class="stat-value">${money(summary.total_stock_value)}</div><div class="stat-foot">Quantity × unit price</div></article>
      <article class="stat-card alert"><div class="stat-top">Low stock <span class="stat-icon">⌁</span></div><div class="stat-value">${lowList.length}</div><div class="stat-foot">At or below reorder level</div></article>
      <article class="stat-card"><div class="stat-top">Recent activity <span class="stat-icon">↕</span></div><div class="stat-value">${movements.length}</div><div class="stat-foot">Latest recorded movements</div></article>
    </section>
    <section class="split-grid">
      <article class="panel"><div class="panel-heading"><div><h2>Recent movements</h2><p>The latest stock changes in your workspace</p></div><button class="text-button" data-goto="movements">View all →</button></div>${movementTable(movements)}</article>
      <article class="panel"><div class="panel-heading"><div><h2>Low stock watch</h2><p>Items that may need a reorder</p></div><button class="text-button" data-goto="items">View items →</button></div>${lowList.length ? `<div class="low-list">${lowList.slice(0, 5).map(item => `<div class="low-row"><span class="item-thumb">▤</span><span class="low-copy"><strong>${esc(item.name)}</strong><small>${esc(item.sku)}</small></span><span class="low-qty">${item.quantity} left<small>reorder at ${item.reorder_level}</small></span></div>`).join("")}</div>` : '<div class="empty-state">Everything is stocked up.</div>'}</article>
    </section>
    <section class="panel" style="margin-top:16px"><div class="panel-heading"><div><h2>Recently added items</h2><p>Your latest catalog additions</p></div><button class="text-button" data-goto="items">Browse inventory →</button></div>${itemsTable(itemsData.items || [], false)}</section>`;
}
function movementTable(rows) { if (!rows.length) return '<div class="empty-state">No movements have been recorded yet.</div>'; return `<div class="table-wrap"><table><thead><tr><th>Item</th><th>Movement</th><th>Quantity</th><th>When</th></tr></thead><tbody>${rows.map(m => `<tr><td><div class="item-cell"><span class="item-thumb">▤</span><span><strong>${esc(m.sku || `Item #${m.item_id}`)}</strong><span class="subtext">${esc(m.note || "No note")}</span></span></div></td><td><span class="badge ${m.type === "IN" ? "in" : "out"}">${m.type === "IN" ? "↙ IN" : "↗ OUT"}</span></td><td><strong>${m.type === "IN" ? "+" : "−"}${m.quantity}</strong></td><td>${dateText(m.created_at)}</td></tr>`).join("")}</tbody></table></div>`; }

async function ensureLookups() { if (!state.categories.length) state.categories = await api("/categories"); if (!state.suppliers.length) state.suppliers = await api("/suppliers"); }
async function renderItems() {
  if (state.user?.role === "admin") await ensureLookups();
  const params = new URLSearchParams({ q: state.query, page: state.pageNumber, limit: state.pageSize }); if (state.categoryFilter) params.set("category_id", state.categoryFilter);
  const data = await api(`/items?${params}`); state.items = data.items || []; state.totalPages = Math.max(1, data.total_pages || 1);
  const categoryOptions = state.user?.role === "admin" ? state.categories.map(c => `<option value="${c.id}" ${String(c.id) === state.categoryFilter ? "selected" : ""}>${esc(c.name)}</option>`).join("") : "";
  const actions = state.user?.role === "admin" ? `<button class="button primary" data-action="new-item">＋ Add item</button>` : recordButton();
  $("#pageContent").innerHTML = `${intro("YOUR PRODUCT CATALOG", "Items", "Manage products and keep track of on-hand quantities.", actions)}
    <section class="panel"><div class="panel-heading"><div><h2>All items</h2><p>Search by product name or SKU</p></div><span class="result-count">${data.total} total</span></div>
      <div class="toolbar"><div class="search-wrap"><span>⌕</span><input id="itemSearch" value="${esc(state.query)}" placeholder="Search items or SKU…" autocomplete="off" /></div>${state.user?.role === "admin" ? `<select id="categoryFilter" class="control"><option value="">All categories</option>${categoryOptions}</select>` : ""}<span class="result-count">Page ${data.page} of ${state.totalPages}</span></div>
      ${itemsTable(state.items, state.user?.role === "admin")}<div class="pagination"><span>Showing ${state.items.length ? (data.page - 1) * state.pageSize + 1 : 0}–${Math.min(data.page * state.pageSize, data.total)} of ${data.total}</span><button data-page-step="-1" ${data.page <= 1 ? "disabled" : ""}>← Previous</button><button data-page-step="1" ${data.page >= state.totalPages ? "disabled" : ""}>Next →</button></div></section>`;
  $("#itemSearch").addEventListener("input", debounce(event => { state.query = event.target.value.trim(); state.pageNumber = 1; renderItems().catch(showPageError); }, 250));
  $("#categoryFilter")?.addEventListener("change", event => { state.categoryFilter = event.target.value; state.pageNumber = 1; renderItems().catch(showPageError); });
}
function itemsTable(items, admin) {
  if (!items.length) return '<div class="empty-state">No items match your search. Try a different name or SKU.</div>';
  return `<div class="table-wrap"><table><thead><tr><th>Item</th><th>Category</th><th>Unit price</th><th>In stock</th><th>Stock status</th>${admin ? "<th></th>" : ""}</tr></thead><tbody>${items.map(item => { const category = state.categories.find(c => c.id === item.category_id); const low = Number(item.quantity) <= Number(item.reorder_level); return `<tr><td><div class="item-cell"><span class="item-thumb">▤</span><span><strong>${esc(item.name)}</strong><span class="subtext">${esc(item.sku)}</span></span></div></td><td>${esc(category?.name || (item.category_id ? `#${item.category_id}` : "Uncategorized"))}</td><td>${money(item.unit_price)}</td><td><strong>${item.quantity}</strong><span class="subtext">reorder at ${item.reorder_level}</span></td><td><span class="badge ${low ? "low" : "good"}">${low ? "● Low stock" : "● In stock"}</span></td>${admin ? `<td><div class="action-group"><button class="table-action" data-action="edit-item" data-id="${item.id}" title="Edit">✎</button><button class="table-action danger" data-action="delete-item" data-id="${item.id}" title="Delete">⌫</button></div></td>` : ""}</tr>`; }).join("")}</tbody></table></div>`;
}
async function renderMovements() { const rows = await api("/stock/movements"); $("#pageContent").innerHTML = `${intro("INVENTORY ACTIVITY", "Stock movements", "A running record of every stock-in and stock-out.", recordButton())}<section class="panel"><div class="panel-heading"><div><h2>Movement history</h2><p>${rows.length} recorded ${rows.length === 1 ? "movement" : "movements"}</p></div></div>${movementTable(rows)}</section>`; }

async function renderCategories() { state.categories = await api("/categories"); $("#pageContent").innerHTML = `${intro("ORGANIZE YOUR CATALOG", "Categories", "Group items so your catalog stays easy to browse.", `<button class="button primary" data-action="new-category">＋ Add category</button>`)}<section class="panel"><div class="panel-heading"><div><h2>All categories</h2><p>${state.categories.length} categories</p></div></div>${state.categories.length ? `<div class="table-wrap"><table><thead><tr><th>Category</th><th>Items</th><th></th></tr></thead><tbody>${state.categories.map(c => `<tr><td><div class="item-cell"><span class="item-thumb">▦</span><strong>${esc(c.name)}</strong></div></td><td>${state.items.filter(i => i.category_id === c.id).length || "—"}</td><td><div class="action-group"><button class="table-action" data-action="edit-category" data-id="${c.id}">✎ Edit</button><button class="table-action danger" data-action="delete-category" data-id="${c.id}">⌫</button></div></td></tr>`).join("")}</tbody></table></div>` : '<div class="empty-state">No categories yet. Create one to organize your items.</div>'}</section>`; }
async function renderSuppliers() { state.suppliers = await api("/suppliers"); $("#pageContent").innerHTML = `${intro("PEOPLE BEHIND YOUR STOCK", "Suppliers", "Keep your supplier contacts close to your inventory.", `<button class="button primary" data-action="new-supplier">＋ Add supplier</button>`)}<section class="panel"><div class="panel-heading"><div><h2>Supplier directory</h2><p>${state.suppliers.length} suppliers</p></div></div>${state.suppliers.length ? `<div class="table-wrap"><table><thead><tr><th>Supplier</th><th>Phone</th><th>Email</th><th>Address</th><th></th></tr></thead><tbody>${state.suppliers.map(s => `<tr><td><div class="item-cell"><span class="item-thumb">⌂</span><strong>${esc(s.name)}</strong></div></td><td>${esc(s.phone || "—")}</td><td>${esc(s.email || "—")}</td><td>${esc(s.address || "—")}</td><td><div class="action-group"><button class="table-action" data-action="edit-supplier" data-id="${s.id}">✎ Edit</button><button class="table-action danger" data-action="delete-supplier" data-id="${s.id}">⌫</button></div></td></tr>`).join("")}</tbody></table></div>` : '<div class="empty-state">No suppliers yet. Add a supplier to keep its contact details handy.</div>'}</section>`; }
function renderUsers() { $("#pageContent").innerHTML = `${intro("YOUR STORE TEAM", "Team", "Create an admin or staff account for your inventory workspace.")}<section class="panel" style="max-width:620px"><div class="panel-heading"><div><h2>Register a team member</h2><p>Staff can view inventory and record stock movements. Admins can manage the catalog and team.</p></div></div><form id="registerForm" class="dialog-form"><label>Full name<input name="name" required maxlength="120" /></label><label>Email address<input name="email" type="email" required maxlength="254" /></label><label>Temporary password<input name="password" type="password" required minlength="8" maxlength="128" /></label><label>Role<select name="role"><option value="staff">Staff</option><option value="admin">Admin</option></select></label><div id="registerMessage" class="form-message"></div><button class="button primary" type="submit">Create account →</button></form></section>`; }
function showPageError(error) { $("#pageContent").innerHTML = `<div class="error-box">${esc(error.message || error)}</div>`; }
function debounce(fn, delay) { let timer; return (...args) => { clearTimeout(timer); timer = setTimeout(() => fn(...args), delay); }; }

async function openRecordDialog() { try { const data = await api("/items?page=1&limit=100"); state.items = data.items || []; $("#movementItem").innerHTML = state.items.map(i => `<option value="${i.id}">${esc(i.name)} · ${esc(i.sku)} · ${i.quantity} available</option>`).join(""); if (!state.items.length) throw new Error("Create an item before recording stock."); $("#recordForm").reset(); state.movementType = "IN"; updateMovementChoice(); showMessage("#recordMessage"); $("#recordDialog").showModal(); } catch (error) { notify(error.message); } }
function updateMovementChoice() { $$(".movement-choice").forEach(button => button.classList.toggle("selected", button.dataset.movement === state.movementType)); $("#recordTitle").textContent = state.movementType === "IN" ? "Record stock in" : "Record stock out"; }
async function openItemDialog(item = null) { try { await ensureLookups(); const form = $("#itemForm"); form.reset(); form.elements.id.value = item?.id || ""; form.elements.name.value = item?.name || ""; form.elements.sku.value = item?.sku || ""; form.elements.category_id.innerHTML = '<option value="">No category</option>' + state.categories.map(c => `<option value="${c.id}">${esc(c.name)}</option>`).join(""); form.elements.supplier_id.innerHTML = '<option value="">No supplier</option>' + state.suppliers.map(s => `<option value="${s.id}">${esc(s.name)}</option>`).join(""); form.elements.category_id.value = item?.category_id || ""; form.elements.supplier_id.value = item?.supplier_id || ""; form.elements.unit_price.value = item?.unit_price ?? ""; form.elements.reorder_level.value = item?.reorder_level ?? 0; form.elements.quantity.value = item?.quantity ?? 0; $("#itemDialogTitle").textContent = item ? "Edit item" : "Add item"; $("#initialQuantityField").classList.toggle("hidden", Boolean(item)); form.elements.quantity.disabled = Boolean(item); showMessage("#itemMessage"); $("#itemDialog").showModal(); } catch (error) { notify(error.message); } }
function openSimpleDialog(type, entity = null) { const form = $("#simpleForm"); form.reset(); form.elements.id.value = entity?.id || ""; const fields = { category: '<label>Category name<input name="name" required maxlength="120" placeholder="e.g. Electronics" /></label>', supplier: '<label>Supplier name<input name="name" required maxlength="160" placeholder="e.g. Northwind Supply" /></label><label>Phone<input name="phone" maxlength="40" placeholder="555-0101" /></label><label>Email<input name="email" type="email" maxlength="254" placeholder="orders@example.com" /></label><label>Address<textarea name="address" rows="2" maxlength="1000" placeholder="Street, city"></textarea></label>' }; $("#simpleEyebrow").textContent = type === "category" ? "ITEM ORGANIZATION" : "SUPPLIER DIRECTORY"; $("#simpleTitle").textContent = `${entity ? "Edit" : "Add"} ${type}`; $("#simpleFields").innerHTML = fields[type]; if (entity) for (const [key, value] of Object.entries(entity)) if (form.elements[key]) form.elements[key].value = value ?? ""; showMessage("#simpleMessage"); $("#simpleForm").dataset.type = type; $("#simpleDialog").showModal(); }

$("#loginForm").addEventListener("submit", async event => { event.preventDefault(); showMessage("#loginMessage"); const form = new FormData(event.currentTarget); try { const data = await api("/auth/login", { method: "POST", body: JSON.stringify(Object.fromEntries(form)) }); state.token = data.token; state.user = data; localStorage.setItem("inventory_token", data.token); setRoleUi(); showApp(); await loadPage("dashboard"); } catch (error) { showMessage("#loginMessage", error.message); } });
$("#logoutButton").addEventListener("click", () => logout());
$("#mainNav").addEventListener("click", event => { const button = event.target.closest("[data-page]"); if (button) loadPage(button.dataset.page).catch(showPageError); });
$("#topStockButton").addEventListener("click", openRecordDialog);
document.addEventListener("click", async event => {
  const close = event.target.closest("[data-close]"); if (close) { $("#" + close.dataset.close).close(); return; }
  const jump = event.target.closest("[data-goto]"); if (jump) { loadPage(jump.dataset.goto).catch(showPageError); return; }
  const step = event.target.closest("[data-page-step]"); if (step) { state.pageNumber += Number(step.dataset.pageStep); renderItems().catch(showPageError); return; }
  const movement = event.target.closest("[data-movement]"); if (movement) { state.movementType = movement.dataset.movement; updateMovementChoice(); return; }
  const action = event.target.closest("[data-action]"); if (!action) return;
  const id = Number(action.dataset.id);
  try {
    if (action.dataset.action === "record") await openRecordDialog();
    if (action.dataset.action === "new-item") await openItemDialog();
    if (action.dataset.action === "edit-item") { const item = await api(`/items/${id}`); await openItemDialog(item); }
    if (action.dataset.action === "delete-item" && confirm("Delete this item from the catalog?")) { await api(`/items/${id}`, { method: "DELETE" }); notify("Item deleted."); await loadPage("items"); }
    if (action.dataset.action === "new-category") openSimpleDialog("category");
    if (action.dataset.action === "edit-category") openSimpleDialog("category", state.categories.find(item => item.id === id));
    if (action.dataset.action === "delete-category" && confirm("Delete this category? Items that use it may prevent deletion.")) { await api(`/categories/${id}`, { method: "DELETE" }); state.categories = []; notify("Category deleted."); await loadPage("categories"); }
    if (action.dataset.action === "new-supplier") openSimpleDialog("supplier");
    if (action.dataset.action === "edit-supplier") openSimpleDialog("supplier", state.suppliers.find(item => item.id === id));
    if (action.dataset.action === "delete-supplier" && confirm("Delete this supplier? Items that use it may prevent deletion.")) { await api(`/suppliers/${id}`, { method: "DELETE" }); state.suppliers = []; notify("Supplier deleted."); await loadPage("suppliers"); }
  } catch (error) { notify(error.message); }
});
$("#recordForm").addEventListener("submit", async event => { event.preventDefault(); showMessage("#recordMessage"); const form = new FormData(event.currentTarget); const body = Object.fromEntries(form); body.item_id = Number(body.item_id); body.quantity = Number(body.quantity); try { await api(`/stock/${state.movementType === "IN" ? "in" : "out"}`, { method: "POST", body: JSON.stringify(body) }); $("#recordDialog").close(); notify(state.movementType === "IN" ? "Stock added successfully." : "Stock removed successfully."); await loadPage(state.page); } catch (error) { showMessage("#recordMessage", error.message); } });
$("#itemForm").addEventListener("submit", async event => { event.preventDefault(); showMessage("#itemMessage"); const form = new FormData(event.currentTarget); const body = Object.fromEntries(form); const id = body.id; delete body.id; body.category_id = body.category_id ? Number(body.category_id) : null; body.supplier_id = body.supplier_id ? Number(body.supplier_id) : null; body.unit_price = Number(body.unit_price); body.reorder_level = Number(body.reorder_level); if (id) delete body.quantity; else body.quantity = Number(body.quantity); try { await api(id ? `/items/${id}` : "/items", { method: id ? "PUT" : "POST", body: JSON.stringify(body) }); $("#itemDialog").close(); state.categories = []; notify(id ? "Item updated." : "Item added."); await loadPage("items"); } catch (error) { showMessage("#itemMessage", error.message); } });
$("#simpleForm").addEventListener("submit", async event => { event.preventDefault(); showMessage("#simpleMessage"); const form = new FormData(event.currentTarget); const body = Object.fromEntries(form); const id = body.id; delete body.id; const type = event.currentTarget.dataset.type; try { await api(`/${type === "category" ? "categories" : "suppliers"}${id ? `/${id}` : ""}`, { method: id ? "PUT" : "POST", body: JSON.stringify(body) }); $("#simpleDialog").close(); if (type === "category") state.categories = []; else state.suppliers = []; notify(`${type[0].toUpperCase() + type.slice(1)} saved.`); await loadPage(type === "category" ? "categories" : "suppliers"); } catch (error) { showMessage("#simpleMessage", error.message); } });
document.addEventListener("submit", async event => { if (event.target.id !== "registerForm") return; event.preventDefault(); showMessage("#registerMessage"); const body = Object.fromEntries(new FormData(event.target)); try { await api("/auth/register", { method: "POST", body: JSON.stringify(body) }); event.target.reset(); notify("Team member created."); } catch (error) { showMessage("#registerMessage", error.message); } });

boot();
