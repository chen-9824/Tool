//
// Created by cfan on 2024/12/27.
//

#ifndef LVGL_APP_SRC_CUSTOMIZE_MYSQLLITE_MYSQLLITE_H_
#define LVGL_APP_SRC_CUSTOMIZE_MYSQLLITE_MYSQLLITE_H_

#include <SQLiteCpp/SQLiteCpp.h>
#include <vector>

class MySqlLite {
 public:
  enum CheckType {
    NextPage,
    LastPage
  };

 public:
  /*
 * 打开数据库并检测表是否存在
 */
  void open_history_db(const std::string &db_path, const std::string &table_name);

  long long get_table_data_num(const std::string &table_name);
  long long calculate_total_page_num(const std::string &table_name, int data_num_in_one_page);

  long long get_max_id(const std::string &table_name);

  std::string get_error_msg();

/*
 * 获取指定行数数据, 排序方式为降序, 从大到小排列
 * table_name:
 * data:获取数据, 排序方式为降序, 从大到小排列,范围为(compare_val, compare_val + line_num), (compare_val - line_num, compare_val)
 * line_num:指定行数,若为-1, 则返回所有数据
 * direction:向前或向后搜索, 即下一页或上一页
 * compare_key:
 * compare_val:比较值,
 */
  void get_multi_line_data(const std::string &table_name,
                           std::vector<std::vector<std::string>> &data,
                           int line_num,
                           CheckType direction,
                           std::string compare_key,
                           std::string compare_val);
/*
 * 搜索数据, 排序方式为降序, 从大到小排列
 * table_name:
 * data:获取数据, 排序方式为降序, 从大到小排列, 范围为(left_compare_val, right_compare_val)
 * compare_key:
 * left_compare_val:比较值
 * right_compare_val:比较值
 */
  void get_search_data(const std::string &table_name,
                       std::vector<std::vector<std::string>> &data,
                       std::string compare_key,
                       std::string left_compare_val,
                       std::string right_compare_val);
  /*
   * 返回对应[key, val]的一行数据
   * data{列名, 值}
   */
  void get_one_line_data(const std::string &table_name,
                         std::map<std::string, std::string> &data,
                         std::string key,
                         std::string val);

 private:
  SQLite::Database *_db = nullptr;

  std::string _sql_db_error_str = "unable to open database file";
  std::string _sql_table_error_str = "no such table";
  std::string _lvgl_table_error_str = "table not created";


};

#endif //LVGL_APP_SRC_CUSTOMIZE_MYSQLLITE_MYSQLLITE_H_
