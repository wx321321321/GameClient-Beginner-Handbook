# UE 运行时类型检查

[返回本章目录](README.md) · [返回手册首页](../README.md)

## type_info:

原理：每个类中会存一个StaticClass()，根据这个来返回

## isA：

沿着父链查找，类似于dfs，可能使用预计算的继承链数组、索引和 ClassCastFlags 加速

## Cast：

用isA判断，如果是，用static_cast
