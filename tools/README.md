# 电子书导出工具

`build_apple_books_epubs.py` 把三个学习目录中的 Markdown 正文转换为可重排的 EPUB 3，生成适合 Apple Books 阅读的封面、章节目录和合订本。脚本仅通过文件读写工作，不启动阅读器或桌面应用，也不修改原始笔记。

## 环境与依赖

使用 Python 3.10 或更高版本；当前依赖版本在 Python 3.12 环境中验证。依赖清单见 [`requirements.txt`](requirements.txt)，包括 `lxml`、`markdown-it-py`、`mdit-py-plugins` 和 `Pillow`，不需要 Pandoc 或 Calibre。

从仓库根目录执行以下命令，可将依赖安装到独立环境：

```powershell
python -m venv .venv
.venv\Scripts\python.exe -m pip install -r tools/requirements.txt
.venv\Scripts\python.exe tools/build_apple_books_epubs.py
```

如果当前 Python 环境已具备这些依赖，可以直接执行下方命令。封面优先使用 Windows 自带的微软雅黑或黑体；其他系统需自行调整脚本中的 `font_candidates`，以保证中文字体可用。

## 用法

```powershell
# 生成三个分册和一本合订本
python tools/build_apple_books_epubs.py

# 仅生成 C++ 与计算机基础分册
python tools/build_apple_books_epubs.py --only cpp

# 仅生成合订本，并指定输出目录
python tools/build_apple_books_epubs.py --only combined --output "output/custom-epub"

# 查看全部参数
python tools/build_apple_books_epubs.py --help
```

| 参数 | 作用 | 默认值 |
| --- | --- | --- |
| `--root` | 指定包含三个学习目录的仓库根目录 | 脚本所在仓库的根目录 |
| `--output` | 指定输出目录；相对路径以执行命令时的当前目录为准 | `<root>/output/AppleBooks_EPUB` |
| `--only` | 选择 `cpp`、`graphics`、`ue`、`combined` 或 `all` | `all` |

默认输出目录已被 Git 忽略。重复运行会覆盖同名导出文件；需要保留旧版时，请先备份导出文件或使用新的 `--output` 路径。

## 内容与顺序

脚本分别读取 `01_CPP&Computer_Basics`、`02_Graphics` 和 `03_UE`，按脚本顶部 `CPP`、`GRAPHICS`、`UE` 的 `order` 列表排列章节。各目录的 `README.md` 只用于仓库导航，不会作为正文导出；大小写不同的 README 文件也会排除。未列入 `order` 的 Markdown 正文按相对路径排序，追加在对应分册末尾。

正文中的整行仓库导航 `[返回本章目录](README.md) · [返回手册首页](../README.md)` 在导出时省略，使用 EPUB 自带的“返回目录”；代码围栏内的相同文字仍会保留。

增加或重命名章节时，需要同步更新分册 README 和脚本中的 `order`。图片使用相对路径引用时会嵌入 EPUB；外部图片会保留为链接，缺失图片会在构建报告中列出警告。

Markdown 表格按表格渲染，已标记的代码不会被旧的 ASCII 图识别逻辑改写；字面量 `\n` 原样保留，不自动拆成换行。`new[](size_t)` 等代码应按正常 Markdown 写进反引号或代码围栏，导出器不再为少数未标记代码关闭所有链接。

正文来源链接和交叉引用按以下规则导出：

- HTTP(S) 等外部链接保留，可在阅读器支持时打开。
- 本册已收录的 Markdown 章节转为书内 XHTML 链接；先索引所有章节标题，再把源文件的 GitHub 风格小节锚点映射到 EPUB 锚点，支持前向引用和重复标题。
- 只在其他分册或仓库中存在的文件，保留引用文字并注明“本册未收录”和仓库相对路径，不生成无法打开的本地链接。合订本已收录的跨编链接仍在书内跳转。
- 原小节锚点未匹配时，退回目标章开头并报告警告；文件缺失时显示缺失提示及路径。构建后应检查这些警告，不能把退回章首当成小节链接已正确匹配。

“合订本”范围仍是上述三个学习目录，不包含项目实践和学习计划目录。原文件的 GitHub 风格标题链接是映射依据，任意 HTML 自定义锚点不在当前 Markdown 输入支持范围内。

## 构建结果与检查

执行成功后，终端会输出 JSON 报告，包括每本书的文件位置、章节数、图片数、警告和检查结果。脚本检查 ZIP 完整性、EPUB 必需文件、XML、资源引用和目录锚点；发现错误时以非零状态退出。

这些是脚本内置的结构检查，不包含 EPUBCheck 或 Apple Books 实机阅读测试，也不验证外部网站的联网可用性。阅读效果以实际阅读器为准；Markdown 源文件仍是维护入口，导出文件可以随时重新生成。
