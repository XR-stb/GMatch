# 贡献指南

感谢您对GMatch项目的关注！我们欢迎各种形式的贡献，包括但不限于功能请求、错误报告、文档改进、代码优化等。此文档提供了参与贡献的指导方针。

## 如何贡献

### 报告问题

如果您发现了问题或有改进建议，请通过GitHub Issues提交，并包含以下信息：

1. 问题的清晰描述
2. 重现步骤（如适用）
3. 期望的行为
4. 实际观察到的行为
5. 截图（如有帮助）
6. 系统环境信息（操作系统、编译器版本等）

### 提交代码

1. Fork 本仓库
2. 创建您的特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交您的更改 (`git commit -m '添加了某某功能'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建一个Pull Request

### 代码风格

- 使用一致的代码风格，遵循现有代码的模式
- 缩进使用4个空格
- 类名使用大驼峰命名法（例如：`MatchMaker`）
- 函数和变量使用小驼峰命名法（例如：`createPlayer`）
- 常量使用全大写和下划线（例如：`MAX_PLAYERS`）
- 私有成员变量以下划线结尾（例如：`rating_`）
- 注释应当清晰、简洁，解释"为什么"，而不仅仅是"是什么"

### 提交信息指南

每个提交信息应当有一个简短的摘要，后跟更详细的描述（如需要）。摘要行应当：

- 限制在50个字符以内
- 以动词开头，使用现在时态（"添加功能"而非"添加了功能"）
- 不以句号结尾

例如：
```
添加玩家评分调整功能

- 实现基于比赛结果的评分调整算法
- 添加评分历史记录
- 优化评分更新性能
```

## 测试

请确保您的代码通过所有现有测试，并为新功能添加适当的测试。测试可以通过以下命令运行：

```bash
./scripts/build.sh --tests
cd build && ctest
```

## 文档

- 为所有公共API添加文档注释
- 更新README.md以反映重大变更
- 为复杂算法添加设计文档

## 许可证

通过贡献代码，您同意您的贡献将在MIT许可证下提供。

## 行为准则

- 尊重所有参与者
- 接受建设性批评
- 专注于最有利于社区的事情
- 以同理心对待其他社区成员

感谢您的贡献！ 