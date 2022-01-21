#!/bin/sh
echo "帮助您自动生成可发布的源码包..."

echo "1、调用automake和autoconf等标准工具"
autoreconf --install
if test $? != 0 ; then
echo "缺少标准开发工具automake autoconf，请先安装开发工具"
exit 1
fi

echo "2、调用./configure脚本自动完成各项检查"
./configure
if test $? != 0 ; then
echo "缺少开发库，请自行安装所需开发工具（祝你好运）"
exit 2
fi

echo "3、调用make工具生成tar.gz源代码压缩包"
make dist --quiet 2>&1 > /dev/null
ls -hs | grep .tar.

