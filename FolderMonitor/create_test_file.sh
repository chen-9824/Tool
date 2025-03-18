#!/bin/bash

cd test/1



for i in {1..50}; do
    dd if=/dev/urandom of="file_$i.bin" bs=1M count=10  # 生成 5MB 随机文件
    touch -t "$(date -d "$i days ago" +%Y%m%d%H%M)" "file_$i.bin"
done