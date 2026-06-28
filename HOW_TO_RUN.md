# How to compile & run this project in VS Code

The fastest path on Windows is below. Linux/macOS sections follow.

---

## 🪟 Windows (the path most FAST students use)

### Step 1 — Install w64devkit (one-time, ~5 minutes)

w64devkit ships GCC + raylib in one zip. You don't have to mess with MSYS2,
pacman, or chasing DLLs.

1. Open https://github.com/skeeto/w64devkit/releases
2. Download `w64devkit-x.y.z.zip` (latest, ~80 MB)
3. Extract to `C:\w64devkit`
4. Add `C:\w64devkit\bin` to your PATH:
   - Press `Win` → type `environment variables` → open it
   - Click **Environment Variables** → under **User variables**, select `Path` → **Edit**
   - **New** → paste `C:\w64devkit\bin` → **OK** out
5. Open a **new** PowerShell or Command Prompt:
   ```
   gcc --version
   ```
   You should see GCC info ✅

### Step 2 — Install raylib

1. Go to https://github.com/raysan5/raylib/releases
2. Download `raylib-X.X_win64_mingw-w64.zip`
3. Extract it
4. Copy:
   - `include\raylib.h` → `C:\w64devkit\include\`
   - `lib\libraylib.a` → `C:\w64devkit\lib\`

### Step 3 — Open in VS Code

1. Install VS Code: https://code.visualstudio.com
2. Install the **C/C++** extension (by Microsoft)
3. Open the project folder: `File → Open Folder → library-management-system`

### Step 4 — Build & Run

**Option A — One keypress:**
- Press `Ctrl + Shift + B` → pick `Build Library System (Windows)`
- Then in VS Code terminal: `.\library_system.exe`

**Option B — Terminal:**
```
gcc main.c -o library_system.exe -lraylib -lopengl32 -lgdi32 -lwinmm
.\library_system.exe
```

---

## 🐧 Linux

```bash
sudo apt update
sudo apt install build-essential libraylib-dev
gcc main.c -o library_system -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./library_system
```

In VS Code: `Ctrl + Shift + B` → `Build Library System (Linux)`

---

## 🍎 macOS

```bash
brew install raylib
gcc main.c -o library_system -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
./library_system
```

In VS Code: `Ctrl + Shift + B` → `Build Library System (macOS)`

---

## 🔑 Demo scenarios

Username / password is case-insensitive — `25K36005` and `25k36005` both work.

### Clean students (can borrow normally)
| Login | Password |
|---|---|
| `25k36001` | `12345678` |
| `25k36005` | `zqp381lm` |
| `22k36008` | `hk19vbe2` |
| `25k36025` | `r8xp3mz5` |

### Students with **unpaid fines** (blocked from borrowing)
| Login | Password | Fine |
|---|---|---|
| `24k36002` | `24681012` | PKR 500 |
| `24k36006` | `t8lqp3wk` | PKR 300 |
| `25k36013` | `f3pz7aq6` | PKR 800 |

Try logging in as any of these → click Request Book → enter any valid ISBN →
watch it block. Then log in as admin → View Student → enter their ID → Clear Fine.

### Students with **overdue books** right now (block applies too, plus the return triggers a fine)
| Login | Password | What |
|---|---|---|
| `23k36003` | `pq51xl9r` | book `662341` held 18 days → 4 days overdue |
| `22k36004` | `un83ba11` | book `442198` held 25 days → 11 days overdue |

Log in as `23k36003` → Return Book → enter `662341` → see the PKR 400 fine
calculated and added. Their on-time/late counters update too.

### Students with **late-return history** (visible in profile)
| Login | Password |
|---|---|
| `23k36007` | `md75rx0v` |
| `22k36020` | `w2tp9fc7` |
| `23k36011` | `s1vr8kp3` |

Their profile shows `Returned late: 1` even though they have no current fine.

### Admin
| Login | Password |
|---|---|
| `11K00001` | `12345678` |
| `11K00002` | `87654321` |

---

## 🧭 Quick tour

1. **Login** as `25k36005` → student menu opens
2. **Search Books** → By Author → enter `khan` → see books written by anyone whose author name contains "khan"
3. **Profile** → see your record. If you have an overdue book it'll appear in red.
4. **Logout**, log in as admin `11K00001` / `12345678`
5. **View Student** → enter `25k36013` → see their PKR 800 fine highlighted in red
6. **Clear Fine** button appears in the bottom-right → click it → fine drops to 0

---

## ⚠️ Common errors

### `'raylib.h' file not found`
- **Windows:** check `raylib.h` is in `C:\w64devkit\include\`
- **Linux:** `sudo apt install libraylib-dev`
- **macOS:** `brew install raylib`

### `undefined reference to InitWindow`
- **Windows:** check `libraylib.a` is in `C:\w64devkit\lib\`
- Make sure your compile command includes `-lraylib`

### `gcc: command not found` (Windows)
PATH isn't set. Close VS Code, open a fresh PowerShell, type `gcc --version`.

### Login fails
The `.txt` files must be in the **same folder** as the executable.

### Dates / fines seem wrong
The app uses your system clock via `time(NULL)`. If your computer's date is
wrong, fines will be wrong. The seed data is timed for Jun 24, 2026; running
much later means the demo books look more overdue than originally written —
that's fine, the math still works.

---

## 📁 Project structure

```
library-management-system/
├── main.c
├── adminLogin.txt
├── studLogin.txt
├── books_Data.txt
├── Makefile
├── README.md
├── HOW_TO_RUN.md
├── .gitignore
└── .vscode/
    ├── tasks.json
    └── launch.json
```

The app creates `temp.txt` and `studTemp.txt` briefly during updates — these
are deleted automatically. If you see them lying around, the program crashed
mid-update. Safe to delete.
