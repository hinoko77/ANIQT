# ANIQT - Anime Quote Terminal

ANIQT (アニメQT) is a command-line tool written in **C** that displays inspirational, philosophical, and memorable **anime quotes**. It fetches anime character images using **AniList API** and supports different terminal image renderers. *please note that Aniqt is currently under developement, project is expected to be finished by April*

## ✨ Features
- 📜 Displays **random anime quotes** categorized by themes (shonen, isekai, philosophy, etc.).
- 🖼️ Fetches **author images** dynamically using the **AniList API**.
- 🎨 Supports terminal image renderers like **kitty icat, timg, catimg, and chafa**.
- 🔀 Randomized selection from curated **anime quote categories**.
- 🚀 Optimized with **multi-threading** for smooth rendering.

## 📥 Installation

### Prerequisites
Ensure you have the following installed:
- **GCC** (for compiling C code)
- **cURL** (for fetching author images)
- One of the following terminal image viewers:
  - `kitty icat`
  - `timg`
  - `catimg`
  - `chafa`

### Compile ANIQT
```sh
git clone https://github.com/yourusername/aniqt.git
cd aniqt
gcc -o aniqt aniqt.c -lcurl



