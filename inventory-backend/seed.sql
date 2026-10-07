-- Local-only starter account. Password is admin12345; change it after first login.
BEGIN;
INSERT OR IGNORE INTO users(name,email,password_hash,role)
VALUES('Inventory Admin','admin@example.com',
       'inventory-admin-salt:50bfe92868eed81a95662b0a1eaa724e0c95ee3daff4c8d594aaaa83324900c7','admin');

INSERT OR IGNORE INTO categories(name) VALUES('Electronics');
INSERT OR IGNORE INTO categories(name) VALUES('Office Supplies');
INSERT OR IGNORE INTO categories(name) VALUES('Accessories');

INSERT INTO suppliers(name,phone,email,address)
SELECT 'Northwind Supply','555-0101','orders@northwind.example','100 Market Street'
WHERE NOT EXISTS (SELECT 1 FROM suppliers WHERE name='Northwind Supply');

INSERT OR IGNORE INTO items(sku,name,category_id,supplier_id,unit_price,quantity,reorder_level)
SELECT 'KB-001','USB Keyboard',(SELECT id FROM categories WHERE name='Electronics'),
       (SELECT id FROM suppliers WHERE name='Northwind Supply'),49.99,25,5;
INSERT OR IGNORE INTO items(sku,name,category_id,supplier_id,unit_price,quantity,reorder_level)
SELECT 'MS-001','Wireless Mouse',(SELECT id FROM categories WHERE name='Accessories'),
       (SELECT id FROM suppliers WHERE name='Northwind Supply'),24.50,8,10;
INSERT OR IGNORE INTO items(sku,name,category_id,supplier_id,unit_price,quantity,reorder_level)
SELECT 'NB-001','Notebook Pack',(SELECT id FROM categories WHERE name='Office Supplies'),
       (SELECT id FROM suppliers WHERE name='Northwind Supply'),6.25,3,5;

INSERT INTO stock_movements(item_id,type,quantity,note,user_id)
SELECT i.id,'IN',i.quantity,'Seed opening stock',(SELECT id FROM users WHERE email='admin@example.com')
FROM items i
WHERE i.sku IN ('KB-001','MS-001','NB-001') AND i.quantity>0
  AND NOT EXISTS (SELECT 1 FROM stock_movements m WHERE m.item_id=i.id);
COMMIT;
