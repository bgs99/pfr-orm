#pragma once

#include <podrm/detail/span.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <variant>

struct sqlite3;
struct sqlite3_stmt;
struct sqlite3_value;

namespace podrm::sqlite {

class Entry {
public:
  /// @throws InvalidRowError if the column is outside of the range
  [[nodiscard]] std::string_view text() const;

  /// @throws InvalidRowError if the column is outside of the range
  [[nodiscard]] std::int64_t bigint() const;

  /// @throws InvalidRowError if the column is outside of the range
  [[nodiscard]] bool boolean() const;

private:
  sqlite3_stmt *statement;

  int column;

  friend class Row;

  explicit Entry(sqlite3_stmt *statement, int column);
};

class Row {
public:
  class InvalidRowError : public std::out_of_range {
  public:
    using std::out_of_range::out_of_range;
  };

  [[nodiscard]] int getColumnCount() const { return this->columnCount; }

  /// @throws InvalidRowError if the column is outside of the range
  [[nodiscard]] Entry get(int column) const;

private:
  sqlite3_stmt *statement;

  int columnCount;

  friend class Result;

  explicit Row(sqlite3_stmt *const statement, const int columnCount)
      : statement(statement), columnCount(columnCount) {}
};

class Result {
public:
  [[nodiscard]] std::optional<Row> getRow() const {
    if (!this->statement.has_value()) {
      return std::nullopt;
    }
    return Row{statement->get(), this->columnCount};
  }

  bool nextRow();

  [[nodiscard]] int getColumnCount() const { return this->columnCount; }

private:
  using Statement = std::unique_ptr<sqlite3_stmt, int (*)(sqlite3_stmt *)>;

  std::optional<Statement> statement;

  int columnCount = 0;

  friend class Connection;

  explicit Result(Statement statement);
};

using Value = std::variant<detail::span<std::byte>, double, std::int64_t,
                           std::string_view>;

class Connection {
public:
  static Connection fromRaw(sqlite3 &connection);

  static Connection inMemory(const char *name);

  static Connection inFile(const std::filesystem::path &path);

  void execute(std::string_view statement, detail::span<const Value> args = {});

  Result query(std::string_view statement, detail::span<const Value> args = {});

private:
  std::unique_ptr<sqlite3, int (*)(sqlite3 *)> connection;

  explicit Connection(sqlite3 &connection);
};

} // namespace podrm::sqlite
