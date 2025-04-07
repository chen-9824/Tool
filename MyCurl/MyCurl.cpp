#include "MyCurl.h"

#include <curl/curl.h>

#include <fstream>
#include <future>
#include <iostream>
#include <string>

size_t req_reply(void *ptr, size_t size, size_t nmemb, void *stream)
{
  // 在注释的里面可以打印请求流，cookie的信息
  // cout << "----->reply" << endl;
  std::string *str = (std::string *)stream;
  // cout << *str << endl;
  (*str).append((char *)ptr, size * nmemb);
  return size * nmemb;
}

void uploadImage(const std::string &url, const std::string &image_path)
{
  CURL *curl;
  CURLcode res;

  // 打开图片文件
  FILE *file = fopen(image_path.c_str(), "rb");
  if (!file)
  {
    std::cerr << "无法打开文件: " << image_path << std::endl;
    return;
  }

  // 初始化libcurl
  curl = curl_easy_init();

  if (!curl)
  {
    std::cerr << "无法初始化curl!" << std::endl;
    fclose(file);
    return;
  }

  std::string strData;

  // 设置目标URL
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  // 启用调试信息输出
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);  // 打印详细的curl请求信息
  // 忽略 SSL 证书验证
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 不验证证书
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // 不验证主机名

  // 配置POST请求
  curl_httppost *formpost = nullptr;
  curl_httppost *lastptr = nullptr;

  // 将图片文件添加到表单
  curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME,
               "image",                            // "file"是上传表单的字段名
               CURLFORM_FILE, image_path.c_str(),  // 图片文件路径
               CURLFORM_CONTENTTYPE, "image/jpeg", // 设置文件类型（例如JPEG）
               CURLFORM_END);

  // 设置POST数据
  curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

  // 设置回调函数
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
  // 设置写数据
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strData);

  // 执行请求
  res = curl_easy_perform(curl);

  if (res != CURLE_OK)
  {
    std::cerr << "图片上传失败: " << curl_easy_strerror(res) << " url:" << url
              << ", image_path:" << image_path << std::endl;
    curl_formfree(formpost);
    curl_easy_cleanup(curl);
    fclose(file);
    return;
  }

  std::cout << "图片已成功上传!" << " url:" << url
            << ", image_path:" << image_path << std::endl;
  std::cout << "response" << strData << std::endl;

  // 清理form
  curl_formfree(formpost);

  // 清理curl
  curl_easy_cleanup(curl);

  // 关闭文件
  fclose(file);

  return;
}

std::future<void> AsyncPostRequestWithImage(const std::string &url,
                                            const std::string &imagePath)
{
  return std::async(std::launch::async, uploadImage, url, imagePath);
}

MyCurl::MyCurl() { curl_global_init(CURL_GLOBAL_DEFAULT); }

MyCurl::~MyCurl() {}

void MyCurl::postWithImage(const std::string &url,
                           const std::string &imagePath)
{
  // uploadImage(url, imagePath);
  AsyncPostRequestWithImage(url, imagePath);
}
