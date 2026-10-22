# 手撕代码速查

[返回本章目录](README.md) · [返回手册首页](../README.md)

## 库函数

### 字符串与数字之间转换：string库

| 函数 | 类型 | 原有说明 |
| --- | --- | --- |
| `std::stoi` | `int` | String to Integer |
| `std::stol` | `long` | String to Long |
| `std::stoll` | `long long` | String to Long Long |
| `std::stof` | `float` | String to Float |
| `std::stod` | `double` | String to Double |
| `std::stold` | `long double` | String to Long Double |

反向转换使用 `std::to_string`：从数字到字符串。原速记中的 `tostring` 已按标准库函数名补全。
