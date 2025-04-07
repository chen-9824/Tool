#ifndef MYCURL_H
#define MYCURL_H

#pragma once
#include <string>
class MyCurl {
 public:
  MyCurl();
  ~MyCurl();

  void postWithImage(const std::string& url, const std::string& imagePath);

 private:
};

#endif