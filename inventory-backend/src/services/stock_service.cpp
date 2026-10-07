#include "stock_service.hpp"
#include "../database/sqlite.hpp"

#include <mutex>
#include <stdexcept>

namespace inventory {
StockResult StockService::move(int itemId, int amount, const std::string &type,
                               const std::string &note, int userId) {
    if (itemId <= 0 || amount <= 0 || userId <= 0 || (type != "IN" && type != "OUT"))
        return {false, "INVALID_INPUT", 0};
    try {
        std::lock_guard<std::mutex> lock(database_.mutex());
        sql::Transaction transaction(database_.get());
        auto lookup = sql::prepare(database_.get(), "SELECT quantity FROM items WHERE id=?");
        sql::bindInt(lookup.get(), 1, itemId);
        if (sqlite3_step(lookup.get()) != SQLITE_ROW) return {false, "ITEM_NOT_FOUND", 0};
        const int before = sqlite3_column_int(lookup.get(), 0);
        if (type == "OUT" && before < amount) return {false, "INSUFFICIENT_STOCK", before};

        auto update = sql::prepare(database_.get(), type == "IN"
            ? "UPDATE items SET quantity=quantity+? WHERE id=?"
            : "UPDATE items SET quantity=quantity-? WHERE id=? AND quantity>=?");
        sql::bindInt(update.get(), 1, amount);
        sql::bindInt(update.get(), 2, itemId);
        if (type == "OUT") sql::bindInt(update.get(), 3, amount);
        if (!sql::done(update.get()) || sqlite3_changes(database_.get()) != 1)
            return {false, "INSUFFICIENT_STOCK", before};
        const int after = type == "IN" ? before + amount : before - amount;

        auto movement = sql::prepare(database_.get(),
            "INSERT INTO stock_movements(item_id,type,quantity,note,user_id) VALUES(?,?,?,?,?)");
        sql::bindInt(movement.get(), 1, itemId);
        sql::bindText(movement.get(), 2, type);
        sql::bindInt(movement.get(), 3, amount);
        sql::bindText(movement.get(), 4, note);
        sql::bindInt(movement.get(), 5, userId);
        if (!sql::done(movement.get())) return {false, "DATABASE_ERROR", before};
        transaction.commit();
        return {true, "", after};
    } catch (const std::exception &) {
        return {false, "DATABASE_ERROR", 0};
    }
}

nlohmann::json StockService::movements(std::optional<int> itemId) {
    std::lock_guard<std::mutex> lock(database_.mutex());
    auto statement = sql::prepare(database_.get(), itemId
        ? "SELECT m.id,m.item_id,i.sku,m.type,m.quantity,m.note,m.user_id,m.created_at FROM stock_movements m JOIN items i ON i.id=m.item_id WHERE m.item_id=? ORDER BY m.id DESC"
        : "SELECT m.id,m.item_id,i.sku,m.type,m.quantity,m.note,m.user_id,m.created_at FROM stock_movements m JOIN items i ON i.id=m.item_id ORDER BY m.id DESC");
    if (itemId) sql::bindInt(statement.get(), 1, *itemId);
    nlohmann::json rows = nlohmann::json::array();
    while (sqlite3_step(statement.get()) == SQLITE_ROW) {
        nlohmann::json row = {{"id", sqlite3_column_int(statement.get(), 0)},
            {"item_id", sqlite3_column_int(statement.get(), 1)}, {"sku", sql::text(statement.get(), 2)},
            {"type", sql::text(statement.get(), 3)}, {"quantity", sqlite3_column_int(statement.get(), 4)},
            {"note", sql::text(statement.get(), 5)}, {"created_at", sql::text(statement.get(), 7)}};
        row["user_id"] = sqlite3_column_type(statement.get(), 6) == SQLITE_NULL
            ? nlohmann::json(nullptr) : nlohmann::json(sqlite3_column_int(statement.get(), 6));
        rows.push_back(std::move(row));
    }
    return rows;
}

nlohmann::json StockService::lowStockItems() {
    std::lock_guard<std::mutex> lock(database_.mutex());
    auto statement = sql::prepare(database_.get(),
        "SELECT id,sku,name,category_id,supplier_id,unit_price,quantity,reorder_level,created_at FROM items WHERE quantity<=reorder_level ORDER BY quantity ASC,name ASC");
    nlohmann::json rows = nlohmann::json::array();
    while (sqlite3_step(statement.get()) == SQLITE_ROW) {
        nlohmann::json row = {{"id", sqlite3_column_int(statement.get(), 0)}, {"sku", sql::text(statement.get(), 1)},
            {"name", sql::text(statement.get(), 2)}, {"unit_price", sqlite3_column_double(statement.get(), 5)},
            {"quantity", sqlite3_column_int(statement.get(), 6)}, {"reorder_level", sqlite3_column_int(statement.get(), 7)},
            {"created_at", sql::text(statement.get(), 8)}};
        row["category_id"] = sqlite3_column_type(statement.get(), 3) == SQLITE_NULL ? nlohmann::json(nullptr) : nlohmann::json(sqlite3_column_int(statement.get(), 3));
        row["supplier_id"] = sqlite3_column_type(statement.get(), 4) == SQLITE_NULL ? nlohmann::json(nullptr) : nlohmann::json(sqlite3_column_int(statement.get(), 4));
        rows.push_back(std::move(row));
    }
    return rows;
}
} // namespace inventory
