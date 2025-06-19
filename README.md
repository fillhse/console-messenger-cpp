# Console Messenger C++

**Console-based chat application** with Telegram-backed authentication, history persistence, and peer-to-peer messaging.

---

## 🚀 Features

- **Server–Client Architecture** using BSD sockets and `select`-based multiplexing  
- **Telegram Authentication**: one-time codes delivered via Telegram Bot  
- **Peer-to-Peer Chat**: `/connect`, `/vote`, `/end` commands for turn-based conversations  
- **Message History**: stored on disk under `HISTORY/`  
- **Clean Shutdown**: `/shutdown` command in server console  
- **Configurable Client**: server IP and port persisted in `CLIENT_SETTING/ip_port.txt`  
- **Comprehensive Tests**: automated unit tests for each module  
- **Clang-Format**: one-liner to format all sources  
- **Doxygen Documentation**: HTML and optional PDF (LaTeX) outputs  

---

## 📁 Repository Layout

```
console-messenger-cpp/
├── CMakeLists.txt               # Top-level build script
├── .clang-format                # Clang-Format style configuration
├── Doxyfile.txt                 # Doxygen configuration
├── README.md                    # This file
├── client/
│   ├── main_client.cpp          # Client entry point
├── server/
│   ├── main_server.cpp          # Server entry point
│   ├── history.h/.cpp           # Chat history persistence
│   ├── telegram_auth.h/.cpp     # Telegram code send/verify
├── socket_utils.h               # Shared send/recv helpers
├── tests/
│   ├── test_history.cpp         # Unit tests for history
│   ├── test_main_client.cpp       # Unit tests for client
│   ├── test_main_server.cpp     # Unit tests for server
│   └── test_telegram_auth.cpp   # Unit tests for telegram_auth
└── docs/
    ├── html/                    # Generated HTML documentation
    └── latex/                   # refman.pdf
```

---

## ⚙️ Prerequisites

### Telegram Bot Setup

1. Create a new bot using the official Telegram bot @BotFather.
2. Copy the generated bot token and save it in the file `BUILD/SERVER_SETTINGS/BOT_TOKEN.txt`
   (first line only).

On startup, the server will read this token and use it to send one-time codes.

3. Users should start the bot and enter their Telegram user ID when prompted.
   The verification code will be sent to that Telegram account via your bot.


> **Supported platforms:** Ubuntu / Debian / WSL

Before building, install the libcurl development package (required by `find_package(CURL)` in CMake):

```bash
git clone https://github.com/fillhse/console-messenger-cpp.git
cd console-messenger-cpp
git submodule update --init --recursive      # cpr
sudo apt-get update
sudo apt-get install libssl-dev
sudo apt-get install libcurl4-openssl-dev
```

- **C++17** compiler (GCC, Clang, or MSVC)  
- **CMake** ≥ 3.10  
- **Linux/macOS** or **WSL** (for BSD sockets)  
- **Telegram Bot Token**: place in `SERVER_SETTINGS/BOT_TOKEN.txt`  
- **clang-format** for code formatting  
- **doxygen** (+ **graphviz**) for documentation generation  
- **(Optional)** TeX Live (`pdflatex` or `xelatex`) for PDF output  

---

## 🛠️ Building the Application

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Client Settings

On first run, the client prompts for server IP and port, which are saved to:

```
CLIENT_SETTING/ip_port.txt
```

---

## 🎯 Running the Application

### Start Server

```bash
./console_server in build folder
```
- By default, the server runs on port 9090
- In the server console enter `/shutdown` to notify clients and exit cleanly.

### Start Client

```bash
./console_client in build folder
```
- Follow prompts to authenticate via Telegram.  
- Available client commands:

  ```
  /connect <ID>  - request chat
  /vote          - pass speaking turn
  /end           - end conversation
  /exit          - disconnect client
  /help          - show commands
  ```

---

## ✅ Testing

The `tests/` directory contains unit tests for each module.

### Build & Run Tests

```bash
cd tests
mkdir build && cd build
cmake ..
cmake --build .
ctest -V
```

---

## 🎨 Code Formatting

Apply `.clang-format` to all sources in **Windows PowerShell**:

```powershell
Get-ChildItem -Recurse -Include *.cpp,*.h | ForEach-Object { clang-format -i $_.FullName }
```

Or in **cmd.exe**:

```cmd
for /R %f in (*.cpp) do clang-format -i "%f" & for /R %f in (*.h) do clang-format -i "%f"
```

---

## 📖 Documentation (Doxygen)

### Generate HTML Documentation

Ensure `Doxyfile.txt` is in the project root. Then run:

```bash
doxygen Doxyfile.txt
```

- **Output:** `docs/html/index.html` and related pages  
- Open in browser: `docs/html/index.html`

### Generate LaTeX & PDF Documentation

If you need PDF output:

1. Install TeX Live with Unicode support:
   ```bash
   sudo apt-get install texlive-latex-recommended texlive-lang-cyrillic
   # or for XeLaTeX:
   sudo apt-get install texlive-xetex
   ```

2. In the project root, set in `Doxyfile.txt`:
   ```ini
   GENERATE_LATEX = YES
   ```

3. Generate Doxygen sources:
   ```bash
   doxygen Doxyfile.txt
   ```

4. Build PDF:
   ```bash
   cd docs/latex
   make           # or use xelatex: xelatex refman.tex
   ```

- **Output PDF:** `docs/latex/refman.pdf`

---

## 📂 Where Documentation Is Stored

- **HTML site:** `docs/html/`  
- **LaTeX sources & PDF:** `docs/latex/`  

---

📦 Dependencies
This project uses the external library cpr for handling HTTP requests (used, for example, for Telegram Bot API communication).

It is included as a Git submodule in the extern/cpr directory.

To initialize the submodule, run:

  ```bash
git submodule update --init --recursive
Make sure libcurl and OpenSSL are installed on your system.
   ```

📄 License Note
The cpr library is open-source and distributed under the MIT License.
Please make sure to comply with the terms of its license if you redistribute or publish your project.


*Licensed under MIT.*
