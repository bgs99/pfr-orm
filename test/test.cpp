#include "field.hpp"

#include <podrm/api.hpp>
#include <podrm/reflection.hpp>
#include <podrm/relations.hpp>
#include <podrm/span.hpp>
#include <podrm/sqlite/operations.hpp>
#include <podrm/sqlite/utils.hpp>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>

namespace orm = podrm::sqlite;

namespace {

struct Address {
  std::int64_t id;

  std::string postalCode;

  friend constexpr bool operator==(const Address &,
                                   const Address &) noexcept = default;
};

} // namespace

template <>
constexpr auto podrm::EntityRegistration<Address> =
    podrm::EntityRegistrationData<Address>{
        .id = test::Field<Address, &Address::id>,
        .idMode = IdMode::Auto,
    };

namespace {

struct Person {
  std::int64_t id;

  std::string name;

  podrm::ForeignKey<Address> address;

  friend constexpr bool operator==(const Person &,
                                   const Person &) noexcept = default;
};

} // namespace

template <>
constexpr auto podrm::EntityRegistration<Person> =
    podrm::EntityRegistrationData<Person>{
        .id = test::Field<Person, &Person::id>,
        .idMode = IdMode::Auto,
    };

static_assert(podrm::DatabaseEntity<Person>);

TEST_CASE("SQLite works", "[sqlite]") {
  orm::Connection conn = orm::Connection::inMemory("test");

  REQUIRE_NOTHROW(orm::createTable<Address>(conn));
  REQUIRE_NOTHROW(orm::createTable<Person>(conn));

  REQUIRE_FALSE(orm::exists<Address>(conn));
  REQUIRE_FALSE(orm::exists<Person>(conn));

  SECTION("Foreign key constraints are enforced") {
    Person person{
        .id = 0,
        .name = "Alex",
        .address{.key = 42},
    };

    CHECK_THROWS(orm::persist(conn, person));
  }

  Address address{
      .id = 0,
      .postalCode = "abc",
  };

  REQUIRE_NOTHROW(orm::persist(conn, address));

  Person person{
      .id = 0,
      .name = "Alex",
      .address{.key = address.id},
  };

  REQUIRE_NOTHROW(orm::persist(conn, person));

  SECTION("find on non-existent id returns nullopt") {
    const std::optional<Person> person = orm::find<Person>(conn, 42);
    CHECK_FALSE(person.has_value());
  }

  SECTION("erase on non-existent id throws") {
    CHECK_THROWS(orm::erase<Person>(conn, 42));
  }

  SECTION("update on non-existent id throws") {
    Person newPerson{
        .id = 42,
        .name = "Anne",
        .address{.key = address.id},
    };

    CHECK_THROWS(orm::update<Person>(conn, newPerson));
  }

  SECTION("erase of referenced entity throws") {
    REQUIRE_THROWS(orm::erase<Address>(conn, address.id));
  }

  SECTION("find on existing id returns existing value") {
    const std::optional<Person> personFound =
        orm::find<Person>(conn, person.id);
    REQUIRE(personFound.has_value());
    CHECK(person == *personFound);
  }

  SECTION("erase on existing id erases existing value") {
    REQUIRE_NOTHROW(orm::erase<Person>(conn, person.id));

    const std::optional<Person> personFound =
        orm::find<Person>(conn, person.id);
    CHECK_FALSE(personFound.has_value());
  }

  SECTION("update on existing id updates existing value") {
    person.name = "Anne";
    REQUIRE_NOTHROW(orm::update(conn, person));

    const std::optional<Person> personFound =
        orm::find<Person>(conn, person.id);
    REQUIRE(personFound.has_value());
    CHECK(personFound->name == "Anne");
  }
}
