#include <iostream>
#include "src/MySqlLite.h"

int main(int, char **)
{
    std::cout << "Hello, from MySqlTest!\n";

    MySqlLite m_sql;
    m_sql.open_history_db("../test.db", "test");
}
