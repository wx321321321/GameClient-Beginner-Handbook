# 教材编号与电子书导出工具

[`course-map.json`](course-map.json) 定义五章共 42 篇正文的唯一阅读顺序。[`number_course.py`](number_course.py) 依据它维护标题编号和本地小节链接；[`build_apple_books_epubs.py`](build_apple_books_epubs.py) 按相同顺序导出五个分册与一本完整合订本。工具只做后台文件处理，不启动阅读器或桌面应用。

## 环境与依赖

使用 Python 3.10 或更高版本；当前依赖版本在 Python 3.12 环境中验证。依赖清单见 [`requirements.txt`](requirements.txt)，包括 `lxml`、`markdown-it-py`、`mdit-py-plugins` 和 `Pillow`，不需要 Pandoc 或 Calibre。

从仓库根目录执行以下命令，可将依赖安装到独立环境：

```powershell
python -m venv .venv
.venv\Scripts\python.exe -m pip install -r tools/requirements.txt
.venv\Scripts\python.exe tools/build_apple_books_epubs.py
```

如果当前 Python 环境已具备这些依赖，可以直接执行下方命令。封面优先使用 Windows 自带的微软雅黑或黑体；其他系统需自行调整脚本中的 `font_candidates`，以保证中文字体可用。

## 统一编号与链接

在所有正文编辑结束、已有备份后，从仓库根目录运行：

```powershell
# 只预览修改范围，不写文件
python tools/number_course.py --navigation

# 应用层级编号，并加入上一节/下一节导航
python tools/number_course.py --write --navigation

# 再检查一次；无需修改时退出码为 0
python tools/number_course.py --check --navigation
```

每篇 H1 采用章号和篇号，例如 `1.4 指针与引用`；H2、H3、H4 依次使用 `1.4.1`、`1.4.1.1`、`1.4.1.1.1`。工具先去掉已有的标题编号，再重新生成，因此重复执行不会叠加编号。`64 位地址`、`3D 旋转` 等本身有意义的数字不当作旧编号删除。

编号依据 Markdown 解析结果，代码围栏、缩进代码和代码中的 `#include` 不参与编号。每篇应有一个 H1，标题层级不能跳级；配置缺文件或结构不符合时先报错，不写入部分文件。`--root` 可指定仓库，`--map` 可指定另一份相同格式的课程配置。

工具建立旧标题锚点到新标题锚点的映射，同步修改全仓 Markdown 的本地小节链接，包含同篇、跨篇、引用式链接和 URL 编码的中文锚点。外部 URL、代码块与行内代码内的链接字面量不改。原无编号锚点也作为兼容别名参与迁移；不存在的旧锚点不会凭空猜测，应继续通过链接检查定位。重复标题按 GitHub 风格增加后缀。

扫描时会在进入目录前排除 `.local-archive`、其他隐藏目录、`output`、`build`、`dist`、虚拟环境和依赖目录，归档不会被读取或改写。指向仓库外部的文件链接也不会作为待改写 Markdown 读取。

`--navigation` 仅维护自身标记块中的相邻篇导航，目录 README 仍由作者维护。省略该选项不会新增导航。编号工具会写回发生变化的 Markdown；EPUB 导出器只读源文件。

## EPUB 用法

```powershell
# 生成五个分册和一本合订本
python tools/build_apple_books_epubs.py

# 仅生成 C++ 与计算机基础分册
python tools/build_apple_books_epubs.py --only cpp

# 仅生成项目实践或学习计划分册
python tools/build_apple_books_epubs.py --only projects
python tools/build_apple_books_epubs.py --only study

# 仅生成合订本，并指定输出目录
python tools/build_apple_books_epubs.py --only combined --output "output/custom-epub"

# 查看全部参数
python tools/build_apple_books_epubs.py --help
```

| 参数 | 作用 | 默认值 |
| --- | --- | --- |
| `--root` | 指定包含五个课程目录的仓库根目录 | 脚本所在仓库的根目录 |
| `--output` | 指定输出目录；相对路径以执行命令时的当前目录为准 | `<root>/output/AppleBooks_EPUB` |
| `--only` | 选择 `cpp`、`graphics`、`ue`、`projects`、`study`、`combined` 或 `all` | `all` |

默认输出目录已被 Git 忽略。重复运行会覆盖同名导出文件；需要保留旧版时，请先备份导出文件或使用新的 `--output` 路径。

## 内容与顺序

脚本读取 `course-map.json` 中的五章，篇数依次为 14、2、4、17、5；合订本包含全部 42 篇。各目录的 `README.md` 只用于仓库导航，不作为正文导出。正文缺失或出现尚未配置的新文章时，工具报错，避免悄悄漏掉或把文章追加到错误位置。

正文中的旧式整行仓库导航和编号工具生成的导航标记块在导出时省略，使用 EPUB 自带目录；代码围栏内的相同文字仍保留。书内正文保留教材标题号，不再为每篇额外添加“第 N 章”，上方提示只标识所属的第 1～5 章。

增加、移动或重命名文章时，同步更新课程配置和分章 README，再运行编号工具。图片使用相对路径引用时会嵌入 EPUB；外部图片会保留为链接，缺失图片会在构建报告中列出警告。

Markdown 表格按表格渲染，已标记的代码不会被旧的 ASCII 图识别逻辑改写；字面量 `\n` 原样保留，不自动拆成换行。`new[](size_t)` 等代码应按正常 Markdown 写进反引号或代码围栏，导出器不再为少数未标记代码关闭所有链接。

正文来源链接和交叉引用按以下规则导出：

- HTTP(S) 等外部链接保留，可在阅读器支持时打开。
- 本册已收录的 Markdown 章节转为书内 XHTML 链接；先索引所有章节标题，再把源文件的 GitHub 风格小节锚点映射到 EPUB 锚点，支持前向引用和重复标题。
- 只在其他分册或仓库中存在的文件，保留引用文字并注明“本册未收录”和仓库相对路径，不生成无法打开的本地链接。合订本已收录的跨编链接仍在书内跳转。
- 原小节锚点未匹配时，退回目标章开头并报告警告；文件缺失时显示缺失提示及路径。构建后应检查这些警告，不能把退回章首当成小节链接已正确匹配。

源码实验、PDF 资源等不作为正文章节打包，引用保留为可读的仓库路径说明。原文件的 GitHub 风格标题链接是映射依据，任意 HTML 自定义锚点不在当前 Markdown 输入支持范围内。

## 构建结果与检查

执行成功后，终端会输出 JSON 报告，包括每本书的文件位置、章节数、图片数、警告和检查结果。脚本检查 ZIP 完整性、EPUB 必需文件、XML、资源引用和目录锚点；发现错误时以非零状态退出。

这些是脚本内置的结构检查，不包含 EPUBCheck 或 Apple Books 实机阅读测试，也不验证外部网站的联网可用性。阅读效果以实际阅读器为准；Markdown 源文件仍是维护入口，导出文件可以随时重新生成。
