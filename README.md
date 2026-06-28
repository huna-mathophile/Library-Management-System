# Library Management System

A desktop library management application built in C with a raylib GUI.
First-semester project at FAST-NUCES.

## What it does

- **Two login roles** — admin and student, each with their own dashboard
- **5 search modes** — by ISBN, title, author, faculty+semester, or subject
- **Admin operations** — add, update quantity, remove books; view any student's
  full record; clear a student's outstanding fine
- **Student operations** — request (borrow) and return books; view personal
  profile with borrowing stats, current loans, and any outstanding fine
- **Batch-based borrowing limits** — older batches get higher caps (since they
  need books for more courses): batch 22 = 4 books, 23 = 3, 24 = 2, 25 = 1
- **14-day loan period with PKR 100/day late fine** — fines are calculated and
  locked in at return time
- **Borrow blocks** — a student cannot borrow another book while they have any
  overdue book OR an unpaid fine
- **File-based storage** — pipe-delimited `.txt` files act as the database

## Tech

- C99
- [raylib](https://www.raylib.com) for the GUI
- `<time.h>` for day-based fine calculation (days since 1970-01-01)
- Plain text files for persistence

## Setup

See [`HOW_TO_RUN.md`](HOW_TO_RUN.md) for full Windows / Linux / macOS setup
with VS Code. Short version:

```bash
# Linux
gcc main.c -o library_system -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./library_system

# Windows (MinGW / w64devkit)
gcc main.c -o library_system.exe -lraylib -lopengl32 -lgdi32 -lwinmm
.\library_system.exe

# macOS
gcc main.c -o library_system -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
./library_system
```

Or just `make`.

## Test credentials

- **Admin:** `11K00001` / `12345678`
- **Student (clean state):** `25k36005` / `zqp381lm`
- **Student with unpaid PKR 500 fine:** `24k36002` / `24681012` — try borrowing
  anything and watch it block
- **Student with overdue book:** `23k36003` / `pq51xl9r` — has book `662341`
  18 days overdue (4 days past loan period). Try returning it to see the
  PKR 400 fine fire. Or try borrowing first and watch the overdue block.
- **Student with multiple overdues:** `22k36004` / `un83ba11` — useful for
  testing the cascade when admin removes one of their books

## Architecture

Finite state machine. `AppState` enum controls which screen is drawn each
frame; the main loop branches on `currentState`.

File mutations use the standard read-original → write-to-temp → atomic-rename
pattern, which is the safe way to edit text files in C.

```
main loop
  ├─ STATE_LOGIN
  ├─ STATE_ADMIN_MENU ──► add / update / remove / view-student / search / profile
  ├─ STATE_STUDENT_MENU ► request / return / search / profile
  └─ ...
```

### Data file format

`books_Data.txt` — one book per line, fields pipe-delimited:

```
isbn|title|author|subject|faculty|semester|quantity|borrowed|available|borrower_list
```

`borrower_list` is comma-separated `studentId:dayNumber` pairs, where
`dayNumber` is days-since-1970 at borrow time. Example:
`23k36007:20624,23k36015:20620`.

`studLogin.txt` — one student per line:

```
username|password|name|semester|faculty|lifetime_borrowed|returned_on_time|returned_late|current_books|fine
```

`current_books` uses the same `isbn:dayNumber` format. `fine` is an integer
in PKR.

### Invariants

The data files maintain these properties:
- For every book: `available == quantity − count(borrower_list)`
- For every book: `borrowed == count(borrower_list)`
- For every student: `lifetime_borrowed == returned_on_time + returned_late + count(current_books)`
- Bidirectional consistency: a student's `current_books` always matches the
  borrowers listed on the corresponding book rows

### Fine calculation

When a book is returned:
- `days_held = today − borrow_day`
- If `days_held ≤ 14` → on-time counter +1, no fine
- If `days_held > 14` → late counter +1, fine added: `(days_held − 14) × 100`

The fine is locked in at return time. Browsing your profile with an overdue
book does *not* show a growing fine — it shows the book as "OVERDUE N days,"
and the fine number only appears once the book is actually returned.

## Known limitations

This is a learning project:
- Passwords stored in plaintext (no hashing)
- File-based "database" doesn't scale and isn't concurrent-safe
- Typing `|` or `,` into an input field would corrupt the row
- No "renew loan" feature
- No reservation queue when all copies are out

## License

MIT — feel free to fork and learn from it.
