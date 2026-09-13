<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

### AI Passport 个人背景

- `images/dalei-ai-passport-sky-v2.png`：保留现有黏土头像，把红色背景替换为
  浅蓝像素天空。使用内置 OpenAI 图像工具基于项目所有者已有的公开头像生成，
  已获准用于本项目。
- `main/profile_images.c`：由 `tools/generate_profile_images.py` 生成的 RGB565
  衍生资源，仅包含 72 × 96 完整人物头像，供 240 × 320 屏幕使用。

### 真机照片

- `screenshots/ai-passport-home-closeup.jpg`：README 主图，展示真机上的个人
  Passport 首页和 AB-731 应用入口。
- `screenshots/ai-passport-home-handheld.jpg`：README 真机展示中的手持近景。
- `screenshots/ai-passport-wearable-view.jpg`：README 图集和 GitHub 个人首页使用的
  完整穿戴设备照片。
- `screenshots/ab731-home-v18-legacy.jpg`：清楚显示 18 题的 AB-731 早期真机首页，
  仅作为迭代记录保留。
- `screenshots/ab731-question-v18-legacy.jpg`：清楚显示 `Q 1/18` 的早期四选一界面，
  不能作为 100 题版本的真机证明。
- 五张照片均由项目所有者提供并授权公开用于本项目；发布前已缩放为
  1350 × 1800，并清除 EXIF、设备型号和 GPS 位置信息。

### 发布插画

- `publish/ab731-cover-illustration.png`：项目所有者确认可使用的 AB-731 学习流程插画。
  可用于 README、发布资料和视频 B-roll，但必须作为玩法示意，不冒充真机截图。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。
