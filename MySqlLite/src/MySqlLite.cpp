//
// Created by cfan on 2024/12/27.
//

#include "MySqlLite.h"

#include <iostream>

using namespace std;

void MySqlLite::open_history_db(const std::string &db_path, const std::string &table_name) {

    //std::cout << "SQlite3 version " << SQLite::VERSION << " (" << SQLite::getLibVersion() << ")" << std::endl;
    //std::cout << "SQliteC++ version " << SQLITECPP_VERSION << std::endl;

    try {
        // Open a database file in create/write mode
        _db = new SQLite::Database(db_path, SQLite::OPEN_READWRITE);
        std::cout << "SQLite database file '" << _db->getFilename().c_str() << "' opened successfully\n";

        // 检查表是否存在
        string check_str = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + table_name + "';";
        SQLite::Statement checkTable(*_db, check_str);

        if (checkTable.executeStep()) {
            std::cout << "Table " << table_name << " already exists." << std::endl;
        } else {
            //std::cout << "Table " << table_name << " does not exist. Creating it now." << std::endl;
            std::cout << "Table " << table_name << " does not exist." << std::endl;
        }

#if 0
        //若不存在则创建
        // Open a database file in create/write mode
        _db = new SQLite::Database(db_path, SQLite::OPEN_READWRITE);
        std::cout << "SQLite database file '" << _db->getFilename().c_str() << "' opened successfully\n";

        // 检查表是否存在
        string check_str = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + table_name +"';";
        SQLite::Statement checkTable(*_db, check_str);
        //
        if (checkTable.executeStep()) {
            std::cout << "Table " << table_name << " already exists." << std::endl;
        } else {
            std::cout << "Table " << table_name << " does not exist. Creating it now." << std::endl;
            // 如果表不存在，创建新表
            string creat_str = R"(
            CREATE TABLE ")" + table_name +R"(" (
                "id" INTEGER PRIMARY KEY AUTOINCREMENT,
                "报警设备" TEXT NOT NULL,
                "报警区域" TEXT NOT NULL,
                "报警原因" TEXT NOT NULL,
                "报警时间" INTEGER NOT NULL
            );
        )";
            *_db->exec(creat_str);
            std::cout << "Table 'history' created successfully." << std::endl;
        }
#endif

#if 0
        // Check the results : expect two row of result
        std::string s = "SELECT * FROM " + table_name;
        SQLite::Statement   query(*_db, s);
        std::cout << s << endl;
        while (query.executeStep())
        {
            std::cout << "row (" << query.getColumn(0) << ", \"" << query.getColumn(1) << "\")\n";
        }
#endif
    }
    catch (std::exception &e) {
        std::cerr << __func__ << " exception: " << e.what() << std::endl;
        throw;
    }
}
long long MySqlLite::get_table_data_num(const std::string &table_name) {
    if (_db == nullptr)
        throw std::runtime_error(get_error_msg());
    long long data_num = 0;
    try {
        std::string get_str = "SELECT COUNT(*) FROM " + table_name + ";";
        SQLite::Statement query(*_db, get_str);

        if (query.executeStep()) {
            // 获取查询结果，即表中记录的总数
            data_num = query.getColumn(0).getInt();
            //std::cout << "Total rows in 'history' table: " << rowCount << std::endl;
        }
    } catch (std::exception &e) {
        std::cerr << __func__ << " exception: " << e.what() << std::endl;
        throw;
    }
    return data_num;
}

long long MySqlLite::calculate_total_page_num(const std::string &table_name, int data_num_in_one_page) {
    long long num = -1;
    try {
        num = (get_table_data_num(table_name) + data_num_in_one_page - 1) / data_num_in_one_page;
    }
    catch (std::exception &e) {
        std::cerr << __func__ << " exception: " << e.what() << std::endl;
        throw;
    }
    return num;
}

long long MySqlLite::get_max_id(const std::string &table_name) {
    try {
        // 构造查询语句
        std::string query_str = "SELECT MAX(id) FROM " + table_name + ";";
        SQLite::Statement query(*_db, query_str);

        // 执行查询
        if (query.executeStep()) {
            // 返回查询结果（若为空则返回0）
            return query.getColumn(0).isNull() ? 0 : query.getColumn(0).getInt();
        }
    } catch (const std::exception &e) {
        std::cerr << __func__ << " exception: " << e.what() << std::endl;
        throw;
    }
    return 0;
}

void MySqlLite::get_multi_line_data(const std::string &table_name, std::vector<std::vector<std::string>> &data,
                                    int line_num,
                                    CheckType direction, std::string compare_key,
                                    std::string compare_val) {
    if (_db == nullptr)
        throw std::runtime_error(get_error_msg());

    std::string get_str;
    if (direction == NextPage) {
        if (line_num != -1) {
            get_str =
                "SELECT * FROM " + table_name + " WHERE " + compare_key + " < " + compare_val
                    + " ORDER BY id DESC LIMIT "
                    + to_string(line_num) + ";";
        } else {
            get_str =
                "SELECT * FROM " + table_name + " WHERE " + compare_key + " < " + compare_val
                    + " ORDER BY id DESC" + ";";
        }

    } else if (direction == LastPage) {
        if (line_num != -1) {
            get_str =
                "SELECT * FROM " + table_name + " WHERE " + compare_key + " > " + compare_val
                    + " ORDER BY id ASC LIMIT "
                    + to_string(line_num) + ";";
        } else {
            get_str =
                "SELECT * FROM " + table_name + " WHERE " + compare_key + " > " + compare_val
                    + " ORDER BY id ASC" + ";";
        }

    }

    std::cout << __func__ << ": " << get_str << endl;

    try {

        std::cout << "CheckType:" << direction << ", compare_key:" << compare_key << ", compare_val:" << compare_val
                  << endl;

        SQLite::Statement query(*_db, get_str);

        std::vector<std::vector<std::string>> temp_data;

        while (query.executeStep()) {
            std::vector<std::string> d;
            std::cout << "row (" << query.getColumn(0) << ", \"" << query.getColumn(1) << "\")\n";
            for (int col = 0; col < query.getColumnCount(); ++col) {
                d.push_back(query.getColumn(col).getString());
            }
            temp_data.push_back(d);

            //不继续查询，避免抛出异常
            /*if (_current_page_num == _total_page_num) {
                if (query.getColumn(0).getInt() == 1)
                    break;
            }*/

        }

        if (direction == NextPage) {
            data = temp_data;
        } else if (direction == LastPage) {
            data.assign(temp_data.rbegin(), temp_data.rend());
        }

        //query.reset();
        std::cout << "=================================" << endl;
    } catch (std::exception &e) {
        std::cerr << __func__ << " exception: " << e.what() << std::endl;
        throw;
    }

}
void MySqlLite::get_search_data(const std::string &table_name,
                                std::vector<std::vector<std::string>> &data,
                                std::string compare_key,
                                std::string left_compare_val,
                                std::string right_compare_val) {
    if (_db == nullptr)
        throw std::runtime_error(get_error_msg());

    std::string get_str = "SELECT * FROM " + table_name +
        " WHERE " + compare_key + " >= " + left_compare_val +
        " AND " + compare_key + " <= " + right_compare_val +
        " ORDER BY " + compare_key + " DESC;";

    std::cout << __func__ << ": " << get_str << endl;

    try {

        std::cout << "compare_key:" << compare_key << ", (" << left_compare_val << ", " << right_compare_val << ")"
                  << endl;

        SQLite::Statement query(*_db, get_str);

        while (query.executeStep()) {
            std::vector<std::string> d;
            std::cout << "row (" << query.getColumn(0) << ", \"" << query.getColumn(1) << "\")\n";
            for (int col = 0; col < query.getColumnCount(); ++col) {
                d.push_back(query.getColumn(col).getString());
            }
            data.push_back(d);
        }

        //query.reset();
        std::cout << "=================================" << endl;
    } catch (std::exception &e) {
        std::cerr << __func__ << " exception: " << e.what() << std::endl;
        throw;
    }
}
void MySqlLite::get_one_line_data(const std::string &table_name,
                                  std::map<std::string, std::string> &data,
                                  std::string key,
                                  std::string val) {

    if (_db == nullptr)
        throw std::runtime_error(get_error_msg());

    std::string get_str = "SELECT * FROM " + table_name +
        " WHERE " + key + " == " + val + ";";

    std::cout << __func__ << ": " << get_str << endl;

    try {

        SQLite::Statement query(*_db, get_str);

        if (query.executeStep()) {
            std::cout << "row (" << query.getColumn(0) << ", \"" << query.getColumn(1) << "\")\n";
            for (int col = 0; col < query.getColumnCount(); ++col) {
                data.insert(make_pair(query.getColumnName(col), query.getColumn(col).getString()));
            }
        }

        //query.reset();
        std::cout << "=================================" << endl;
    } catch (std::exception &e) {
        std::cerr << __func__ << " exception: " << e.what() << std::endl;
        throw;
    }

}
std::string MySqlLite::get_error_msg() {
    if (_db == nullptr)
        throw std::runtime_error(_sql_db_error_str);
    return _db->getErrorMsg();
}
