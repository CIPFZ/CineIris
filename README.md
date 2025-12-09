# 🎬 CineIris Studio

**CineIris Studio** is a high-performance desktop application that transforms video files into artistic "Movie Barcodes" and "Iris" visualizations. It analyzes the color spectrum of a movie over time and condenses it into a single, beautiful image.

**CineIris Studio** 是一个高性能桌面应用程序，能够将视频文件转化为极具艺术感的“电影条形码”或“色彩虹膜图”。它通过分析电影随时间变化的色彩光谱，将其浓缩为一张精美的视觉艺术作品。

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Qt](https://img.shields.io/badge/Qt-5.14.2-green.svg)
![FFmpeg](https://img.shields.io/badge/FFmpeg-Latest-red.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)

---

## ✨ Features (功能特性)

* **🎨 Dual Artistic Modes**: Generate linear **Barcodes** or circular **Iris** (Ring) visualizations.
    * **双重艺术模式**：支持生成线性的**电影条形码**或圆环状的**色彩虹膜图**。
* **🚀 High Performance**: Powered by **FFmpeg**, utilizing optimized seeking algorithms to process 4K movies in seconds.
    * **极速性能**：基于 **FFmpeg** 内核，利用优化的空降读取算法，数秒内即可处理 4K 电影。
* **🧠 Smart Analysis**: Automatically detects video metadata (resolution, duration) and calculates the optimal sampling rate for the best result.
    * **智能分析**：自动读取视频元数据（分辨率、时长），并智能计算最合适的抽帧密度。
* **📂 Batch Processing**: Professional task queue system supporting drag-and-drop, multi-selection, pause, and resume.
    * **批量处理**：专业的任务队列系统，支持拖拽添加、批量勾选、暂停和恢复任务。
* **💾 Session Persistence**: Automatically saves your task list and progress. You can close the app and resume work later.
    * **进度持久化**：自动保存任务列表和处理进度。即使关闭软件，下次打开也能无缝继续。
* **🖥️ Modern UI**: Clean interface with separate logic for viewing details (Click) and batch operations (Checkbox).
    * **现代交互**：清爽的界面设计，实现了“点击查看详情”与“勾选批量操作”的逻辑分离。

---

## 📥 Download & Install (下载与安装)

### Windows
This is a **portable** application (Green Software). No installation required.
这是一个**绿色免安装**软件。

1.  Go to the [Releases](../../releases) page.
2.  Download `CineIris_App.zip`.
3.  Unzip the file.
4.  Run `CineIris.exe`.

1.  前往 [Releases](../../releases) 页面。
2.  下载 `CineIris_App.zip` 压缩包。
3.  解压文件。
4.  直接运行 `CineIris.exe` 即可。

---

## 📖 Usage (使用指南)

1.  **Add Videos**: Drag and drop video files (`.mp4`, `.mkv`, `.avi`, `.mov`) into the window, or click the "Add" button.
    * **添加视频**：将视频文件直接拖入窗口，或点击“添加”按钮。
2.  **Configure**: Click on a task name to view details. Adjust **Resolution** (Output Size) and **Sample Count** (Density) in the right panel.
    * **参数配置**：点击左侧任务名称查看详情。在右侧面板调整**输出分辨率**和**抽帧密度**。
3.  **Run**: Click "Add to Queue" to start processing.
    * **开始生成**：点击“加入生成队列”开始处理。
4.  **Batch Control**: Check the boxes on the left list to Select multiple tasks, then use the toolbar buttons to **Start**, **Pause**, or **Delete** them in batch.
    * **批量控制**：勾选左侧列表的复选框，利用顶部工具栏按钮进行**批量启动**、**暂停**或**删除**。
5.  **Save**: Once finished, view the result in the gallery and click "Save Image" to export as PNG.
    * **保存图片**：生成完成后，预览大图并点击“保存图片”导出 PNG 文件。

---

## 🛠️ Build from Source (源码构建)

If you are a developer and want to build it yourself:
如果你是开发者并希望自行编译：

### Requirements
* **Qt 5.14.2** (MinGW 64-bit recommended)
* **FFmpeg Dev Libraries** (Shared DLLs & Dev files)

### Steps
1.  Clone this repository.
2.  Place the `ffmpeg` folder (containing `bin`, `include`, `lib`) into the project root directory.
    * *Structure should be: `CineIris/ffmpeg/include/...`*
3.  Open `CineIris.pro` in Qt Creator.
4.  Build and Run (Release mode recommended).
5.  Copy FFmpeg `.dll` files to the build output directory to run the executable.

---

## 🤝 Contributing (贡献)

Pull requests are welcome! If you have ideas for new features (e.g., new color algorithms, dark mode), feel free to open an issue.

欢迎提交 Pull Request！如果你有新的想法（例如新的色彩算法、暗黑模式支持），欢迎提交 Issue 讨论。

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
