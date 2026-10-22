# 手撕代码速查：字符串与数字转换

这一页对应原来的 `string` 库转换速记：先按结果类型选函数，再区分“转换出一个前缀”与“整个字符串都合法”。这些函数声明在 `<string>` 中。

## 1. 字符串转数字

| 函数 | 返回类型 | 名称记忆 |
| --- | --- | --- |
| `std::stoi` | `int` | string to integer |
| `std::stol` | `long` | string to long |
| `std::stoll` | `long long` | string to long long |
| `std::stof` | `float` | string to float |
| `std::stod` | `double` | string to double |
| `std::stold` | `long double` | string to long double |

整数转换可指定进制，第二个参数可记录第一个未转换字符的位置。返回类型应按需要的范围选择，不能把所有整数都先转成 `int` 再想办法补救溢出。

```cpp
#include <cassert>
#include <string>

int main()
{
    std::size_t pos = 0;
    int value = std::stoi("123abc", &pos);
    assert(value == 123);
    assert(pos == 3);  // 转换了数字前缀，不代表整个输入合法。

    int hex = std::stoi("ff", nullptr, 16);
    assert(hex == 255);
}
```

如果题目要求整个字符串都是数字，还要检查 `pos` 是否到末尾，并按题意处理空白、正负号等规则。没有可转换内容时可能抛出 `std::invalid_argument`，超出目标类型范围时可能抛出 `std::out_of_range`。不能把“转换失败”与合法数字 0 混为一谈。规则可查 [C++ 标准草案：字符串数值转换](https://eel.is/c++draft/string.conversions)。

## 2. 数字转字符串

反方向使用 `std::to_string`，不是原速记里的 `tostring`：

```cpp
std::string countText = std::to_string(42);  // "42"
```

它适合一般数字文本转换；如果题目要求特定位数、小数精度或格式，不要依赖未说明的浮点输出形式，应明确选择格式化方式。
