#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================
//  CONSTANTS
// ============================================================
#define MAX_INPUT_CHARS    50
#define SCREEN_WIDTH       1000
#define SCREEN_HEIGHT      800     // bumped to give the profile screen room
#define LOAN_PERIOD_DAYS   14
#define FINE_PER_DAY       100
#define SECONDS_PER_DAY    86400

// Button sizing
#define BTN_H              54
#define BTN_W_MENU         300
#define BTN_W_ACTION       200
#define BTN_W_SMALL        130
#define BTN_GAP            16

// Layout
#define HEADER_H           72
#define FOOTER_RESERVE     90

// ============================================================
//  PALETTE  (dark theme)
// ============================================================
#define COLOR_BG           (Color){  15,  17,  26, 255 }
#define COLOR_HEADER       (Color){  18,  20,  32, 255 }
#define COLOR_HEADER_TEXT  (Color){ 255, 255, 255, 255 }
#define COLOR_PRIMARY      (Color){  99,  91, 255, 255 }
#define COLOR_PRIMARY_HOV  (Color){ 122, 115, 255, 255 }
#define COLOR_SUCCESS      (Color){  34, 211, 138, 255 }
#define COLOR_SUCCESS_HOV  (Color){  52, 231, 158, 255 }
#define COLOR_DANGER       (Color){ 255,  82,  82, 255 }
#define COLOR_DANGER_HOV   (Color){ 255, 110, 110, 255 }
#define COLOR_WARNING      (Color){ 251, 191,  36, 255 }
#define COLOR_TEXT         (Color){ 255, 255, 255, 255 }
#define COLOR_MUTED        (Color){ 190, 195, 220, 255 }
#define COLOR_BORDER       (Color){  40,  44,  66, 255 }
#define COLOR_SUBTLE       (Color){  22,  25,  40, 255 }
#define COLOR_CARD         (Color){  22,  25,  40, 255 }

// Button style enum
typedef enum {
    BTN_PRIMARY,
    BTN_SECONDARY,
    BTN_DANGER,
    BTN_SUCCESS
} ButtonStyle;

// ============================================================
//  STATES
// ============================================================
typedef enum {
    STATE_LOGIN,
    STATE_ADMIN_MENU,
    STATE_STUDENT_MENU,
    STATE_SEARCH_MENU,
    STATE_SEARCH_INPUT,
    STATE_SEARCH_RESULT,
    STATE_ADD_BOOK,
    STATE_REMOVE_BOOK,
    STATE_UPDATE_BOOK,
    STATE_PROFILE,
    STATE_REQUEST_BOOK,
    STATE_RETURN_BOOK,
    STATE_VIEW_STUDENT_INPUT,
    STATE_VIEW_STUDENT_RESULT
} AppState;

// ============================================================
//  GLOBALS
// ============================================================
char usernameInput[MAX_INPUT_CHARS] = {0};
char passwordInput[MAX_INPUT_CHARS] = {0};
char searchInput[MAX_INPUT_CHARS]   = {0};
char searchInput2[MAX_INPUT_CHARS]  = {0};
char displayBuffer[4096]            = {0};
char currentLoggedInUser[400]       = {0};
char viewedStudent[400]             = {0};
char inIsbn[50] = {0}, inTitle[50] = {0}, inAuth[50] = {0};
char inSub[50]  = {0}, inFac[50]   = {0}, inSem[50]  = {0}, inQty[50] = {0};
char inStudId[50] = {0};

int  currentSearchMode = 0;
int  activeBox         = 0;
int  isAdmin           = 0;
AppState currentState  = STATE_LOGIN;

// Global font, loaded at startup. Falls back to raylib's default if no
// system font can be opened (rare, but we handle it).
Font appFont;
int  appFontLoaded = 0;

// Try several common system font paths until one loads. Order matters —
// the first match wins, so Windows/macOS/Linux fonts are listed top-down.
Font LoadAppFont(void) {
    const char *candidates[] = {
        // Windows
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        // macOS
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        // Linux (common Ubuntu/Debian paths)
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        NULL
    };
    for (int i = 0; candidates[i] != NULL; i++) {
        if (FileExists(candidates[i])) {
            Font f = LoadFontEx(candidates[i], 32, NULL, 0);
            if (f.texture.id != 0) {
                appFontLoaded = 1;
                return f;
            }
        }
    }
    appFontLoaded = 0;
    return GetFontDefault();
}

// Drop-in DrawText replacement that uses the loaded TTF font.
void DT(const char *text, int x, int y, int size, Color color) {
    Vector2 p = { (float)x, (float)y };
    DrawTextEx(appFont, text, p, (float)size, 1.0f, color);
}

// Drop-in MeasureText replacement.
int MT(const char *text, int size) {
    Vector2 v = MeasureTextEx(appFont, text, (float)size, 1.0f);
    return (int)v.x;
}


// ============================================================
//  GENERAL HELPERS
// ============================================================

int strToNum(const char *str) {
    int r = 0;
    for (int i = 0; str[i]; i++)
        if (str[i] >= '0' && str[i] <= '9') r = r * 10 + str[i] - '0';
    return r;
}

void strToLower(char *str) {
    for (int i = 0; str[i]; i++)
        if (str[i] >= 'A' && str[i] <= 'Z') str[i] += 'a' - 'A';
}

// Case-insensitive string equality. Used for usernames and ISBNs so the
// user can type "25K36005" or "25k36005" and either works.
int streqi(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == *b;
}

void stripNewline(char *line) { line[strcspn(line, "\r\n")] = '\0'; }

int getTodayDayNum(void) { return (int)(time(NULL) / SECONDS_PER_DAY); }

void getField(const char *line, int targetIndex, char *dest, int destSize) {
    int currentField = 0, i = 0, k = 0;
    while (line[i] != '\0' && currentField < targetIndex) {
        if (line[i] == '|') currentField++;
        i++;
    }
    while (line[i] != '\0' && line[i] != '|' && k < destSize - 1)
        dest[k++] = line[i++];
    dest[k] = '\0';
}


// ============================================================
//  COMMA-LIST HELPERS  (dated entries: "id:daynum")
// ============================================================

int splitIdDay(const char *token, char *idOut, int idSize) {
    int i = 0, k = 0;
    while (token[i] != '\0' && token[i] != ':' && k < idSize - 1)
        idOut[k++] = token[i++];
    idOut[k] = '\0';
    if (token[i] == ':') return strToNum(token + i + 1);
    return 0;
}

int idInDatedList(const char *list, const char *target) {
    int i = 0;
    while (list[i] != '\0') {
        char tok[80]; int t = 0;
        while (list[i] != '\0' && list[i] != ',' && t < 79) tok[t++] = list[i++];
        tok[t] = '\0';
        if (list[i] == ',') i++;
        if (t == 0) continue;
        char id[50]; splitIdDay(tok, id, sizeof(id));
        if (streqi(id, target)) return 1;
    }
    return 0;
}

int getDayForId(const char *list, const char *target) {
    int i = 0;
    while (list[i] != '\0') {
        char tok[80]; int t = 0;
        while (list[i] != '\0' && list[i] != ',' && t < 79) tok[t++] = list[i++];
        tok[t] = '\0';
        if (list[i] == ',') i++;
        if (t == 0) continue;
        char id[50];
        int day = splitIdDay(tok, id, sizeof(id));
        if (streqi(id, target)) return day;
    }
    return -1;
}

void removeFromDatedList(const char *list, const char *target,
                         char *dest, int destSize) {
    int k = 0, i = 0;
    while (list[i] != '\0' && k < destSize - 1) {
        char tok[80]; int t = 0;
        while (list[i] != '\0' && list[i] != ',' && t < 79) tok[t++] = list[i++];
        tok[t] = '\0';
        if (list[i] == ',') i++;
        if (t == 0) continue;
        char id[50]; splitIdDay(tok, id, sizeof(id));
        if (!streqi(id, target)) {
            if (k > 0 && k < destSize - 1) dest[k++] = ',';
            for (int x = 0; x < t && k < destSize - 1; x++) dest[k++] = tok[x];
        }
    }
    dest[k] = '\0';
}

void appendDated(char *list, int listSize, const char *id, int day) {
    int len = (int)strlen(list);
    char add[80];
    snprintf(add, sizeof(add), "%s:%d", id, day);
    if (len == 0) snprintf(list, listSize, "%s", add);
    else if (len + 1 + (int)strlen(add) < listSize)
        snprintf(list + len, listSize - len, ",%s", add);
}

int countTokens(const char *list) {
    if (list[0] == '\0') return 0;
    int n = 1;
    for (int i = 0; list[i]; i++) if (list[i] == ',') n++;
    return n;
}


// ============================================================
//  UI WIDGETS
// ============================================================

static Color buttonFill(ButtonStyle s, bool hover) {
    switch (s) {
        case BTN_PRIMARY:   return hover ? COLOR_PRIMARY_HOV : COLOR_PRIMARY;
        case BTN_DANGER:    return hover ? COLOR_DANGER_HOV  : COLOR_DANGER;
        case BTN_SUCCESS:   return hover ? COLOR_SUCCESS_HOV : COLOR_SUCCESS;
        case BTN_SECONDARY: return hover ? COLOR_SUBTLE      : COLOR_CARD;
    }
    return COLOR_CARD;
}

static Color buttonText(ButtonStyle s) {
    return (s == BTN_SECONDARY) ? COLOR_PRIMARY : COLOR_CARD;
}

// Styled button. Rounded, with proper internal padding (text centred inside).
bool GuiBtn(Rectangle bounds, const char *text, ButtonStyle style) {
    Vector2 mp = GetMousePosition();
    bool hover  = CheckCollisionPointRec(mp, bounds);
    bool clicked = (hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON));

    Color fill = buttonFill(style, hover);
    DrawRectangleRounded(bounds, 0.30f, 8, fill);
    if (style == BTN_SECONDARY)
        DrawRectangleRoundedLinesEx(bounds, 0.30f, 8, 2.0f, COLOR_PRIMARY);

    int fontSize = 28;
    int tw = MT(text, fontSize);
    DT(text,
             (int)(bounds.x + (bounds.width  - tw)       / 2),
             (int)(bounds.y + (bounds.height - fontSize) / 2),
             fontSize, buttonText(style));
    return clicked;
}

// Modern rounded input box.
bool GuiInput(Rectangle bounds, char *buffer, int bufferSize, bool isActive) {
    DrawRectangleRounded(bounds, 0.25f, 8, COLOR_CARD);
    DrawRectangleRoundedLinesEx(bounds, 0.25f, 8, 2.0f,
                                isActive ? COLOR_PRIMARY : COLOR_BORDER);

    int fontSize = 28;
    DT(buffer, (int)(bounds.x + 14),
             (int)(bounds.y + (bounds.height - fontSize) / 2),
             fontSize, COLOR_TEXT);

    if (isActive) {
        if ((GetTime() * 2.0) - (int)(GetTime() * 2.0) < 0.5)
            DT("_",
                     (int)(bounds.x + 16 + MT(buffer, fontSize)),
                     (int)(bounds.y + (bounds.height - fontSize) / 2 + 2),
                     fontSize, COLOR_PRIMARY);

        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125 &&
                (int)strlen(buffer) < bufferSize - 1) {
                int len = strlen(buffer);
                buffer[len]     = (char)key;
                buffer[len + 1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = strlen(buffer);
            if (len > 0) buffer[len - 1] = '\0';
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return CheckCollisionPointRec(GetMousePosition(), bounds);
    return isActive;
}

// Draw a card-style panel that hosts a screen's content.
void DrawCard(Rectangle bounds) {
    DrawRectangleRounded(bounds, 0.04f, 6, COLOR_CARD);
    DrawRectangleRoundedLinesEx(bounds, 0.04f, 6, 1.0f, COLOR_BORDER);
}

// Section header text.
void DrawHeading(const char *text, int x, int y) {
    DT(text, x, y, 42, COLOR_TEXT);
}

void DrawLabel(const char *text, int x, int y) {
    DT(text, x, y, 26, COLOR_MUTED);
}


// ============================================================
//  AUTHENTICATION
// ============================================================

int VerifyLogin(const char *user, const char *pass, int asAdmin) {
    FILE *ptr = fopen(asAdmin ? "adminLogin.txt" : "studLogin.txt", "r");
    if (!ptr) return 0;
    char line[400];
    int firstLine = 1;
    while (fgets(line, sizeof(line), ptr)) {
        stripNewline(line);
        if (firstLine) { firstLine = 0; continue; }
        if (line[0] == '\0') continue;
        char dbUser[50], dbPass[50];
        getField(line, 0, dbUser, sizeof(dbUser));
        getField(line, 1, dbPass, sizeof(dbPass));
        if (streqi(user, dbUser) && strcmp(pass, dbPass) == 0) {
            strncpy(currentLoggedInUser, line, sizeof(currentLoggedInUser) - 1);
            currentLoggedInUser[sizeof(currentLoggedInUser) - 1] = '\0';
            fclose(ptr);
            return 1;
        }
    }
    fclose(ptr);
    return 0;
}

int LoadStudentRecord(const char *studId) {
    FILE *ptr = fopen("studLogin.txt", "r");
    if (!ptr) return 0;
    char line[400];
    int firstLine = 1;
    while (fgets(line, sizeof(line), ptr)) {
        stripNewline(line);
        if (firstLine) { firstLine = 0; continue; }
        if (line[0] == '\0') continue;
        char dbId[50];
        getField(line, 0, dbId, sizeof(dbId));
        if (streqi(dbId, studId)) {
            strncpy(viewedStudent, line, sizeof(viewedStudent) - 1);
            viewedStudent[sizeof(viewedStudent) - 1] = '\0';
            fclose(ptr);
            return 1;
        }
    }
    fclose(ptr);
    return 0;
}


// ============================================================
//  SEARCH
// ============================================================

void PerformSearch(int mode, const char *q1, const char *q2, int showBorrowers) {
    FILE *ptr = fopen("books_Data.txt", "r");
    if (!ptr) { strcpy(displayBuffer, "Database not found."); return; }

    displayBuffer[0] = '\0';
    int foundCount = 0;

    char q1Lower[50];
    strncpy(q1Lower, q1, sizeof(q1Lower) - 1);
    q1Lower[sizeof(q1Lower) - 1] = '\0';
    strToLower(q1Lower);

    char line[400];
    int firstLine = 1;
    while (fgets(line, sizeof(line), ptr)) {
        stripNewline(line);
        if (firstLine) { firstLine = 0; continue; }
        if (line[0] == '\0') continue;

        char isbn[50], title[50], author[50], sub[50], fac[50];
        char sem[50], qty[50], avail[50], stud[300];

        getField(line, 0, isbn,   sizeof(isbn));
        getField(line, 1, title,  sizeof(title));
        getField(line, 2, author, sizeof(author));
        getField(line, 3, sub,    sizeof(sub));
        getField(line, 4, fac,    sizeof(fac));
        getField(line, 5, sem,    sizeof(sem));
        getField(line, 6, qty,    sizeof(qty));
        getField(line, 8, avail,  sizeof(avail));
        getField(line, 9, stud,   sizeof(stud));

        char tTit[50], tAuth[50], tFac[50], tSub[50];
        strcpy(tTit,  title);  strToLower(tTit);
        strcpy(tAuth, author); strToLower(tAuth);
        strcpy(tFac,  fac);    strToLower(tFac);
        strcpy(tSub,  sub);    strToLower(tSub);

        int match = 0;
        if      (mode == 1 && streqi(isbn, q1))                    match = 1;
        else if (mode == 2 && strstr(tTit,  q1Lower))              match = 1;
        else if (mode == 3 && strstr(tAuth, q1Lower))              match = 1;
        else if (mode == 4 && strstr(tFac,  q1Lower)
                           && strcmp(sem, q2) == 0)                match = 1;
        else if (mode == 5 && strstr(tSub,  q1Lower))              match = 1;

        if (match) {
            char entry[700];
            snprintf(entry, sizeof(entry),
                     "ISBN: %s | %s by %s\nSub: %s | Fac: %s | Sem: %s\nAvail: %s/%s\n",
                     isbn, title, author, sub, fac, sem, avail, qty);

            if (showBorrowers) {
                char borrowerIds[300] = "";
                int i = 0, first = 1;
                while (stud[i] != '\0') {
                    char tok[80]; int t = 0;
                    while (stud[i] != '\0' && stud[i] != ',' && t < 79) tok[t++] = stud[i++];
                    tok[t] = '\0';
                    if (stud[i] == ',') i++;
                    if (t == 0) continue;
                    char id[50]; splitIdDay(tok, id, sizeof(id));
                    if (!first) strncat(borrowerIds, ", ",
                                        sizeof(borrowerIds) - strlen(borrowerIds) - 1);
                    strncat(borrowerIds, id,
                            sizeof(borrowerIds) - strlen(borrowerIds) - 1);
                    first = 0;
                }
                size_t len = strlen(entry);
                snprintf(entry + len, sizeof(entry) - len,
                         "Borrowed By: %s\n", borrowerIds);
            }
            strncat(entry, "------------------------------------------------\n",
                    sizeof(entry) - strlen(entry) - 1);

            if (strlen(displayBuffer) + strlen(entry) < sizeof(displayBuffer) - 1)
                strcat(displayBuffer, entry);
            foundCount++;
        }
    }
    fclose(ptr);
    if (foundCount == 0) strcpy(displayBuffer, "No records found.");
}


// ============================================================
//  ADD BOOK
// ============================================================

void PerformAddBook(void) {
    if (strlen(inIsbn) == 0 || strlen(inTitle) == 0 || strlen(inQty) == 0) {
        strcpy(displayBuffer, "Error: ISBN, Title, and Qty are required.");
        return;
    }
    FILE *check = fopen("books_Data.txt", "r");
    if (check) {
        char line[400], curIsbn[50];
        int firstLine = 1;
        while (fgets(line, sizeof(line), check)) {
            stripNewline(line);
            if (firstLine) { firstLine = 0; continue; }
            if (line[0] == '\0') continue;
            getField(line, 0, curIsbn, sizeof(curIsbn));
            if (streqi(curIsbn, inIsbn)) {
                strcpy(displayBuffer, "Error: ISBN already exists.");
                fclose(check);
                return;
            }
        }
        fclose(check);
    }
    // Normalize new entries to lowercase so the corpus stays uniform.
    strToLower(inIsbn); strToLower(inTitle); strToLower(inAuth);
    strToLower(inSub);  strToLower(inFac);

    FILE *ptr = fopen("books_Data.txt", "a");
    if (!ptr) { strcpy(displayBuffer, "File Error."); return; }
    fprintf(ptr, "%s|%s|%s|%s|%s|%s|%s|0|%s|\n",
            inIsbn, inTitle, inAuth, inSub, inFac, inSem, inQty, inQty);
    fclose(ptr);

    strcpy(displayBuffer, "Book added successfully.");
    memset(inIsbn, 0, sizeof(inIsbn));
    memset(inTitle, 0, sizeof(inTitle));
    memset(inAuth, 0, sizeof(inAuth));
    memset(inSub, 0, sizeof(inSub));
    memset(inFac, 0, sizeof(inFac));
    memset(inSem, 0, sizeof(inSem));
    memset(inQty, 0, sizeof(inQty));
}


// ============================================================
//  UPDATE BOOK QUANTITY
// ============================================================

void PerformUpdateBook(void) {
    if (strlen(inIsbn) == 0 || strlen(inQty) == 0) {
        strcpy(displayBuffer, "Error: ISBN and Qty are required.");
        return;
    }
    FILE *ptr  = fopen("books_Data.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if (!ptr || !temp) {
        strcpy(displayBuffer, "File Error.");
        if (ptr)  fclose(ptr);
        if (temp) fclose(temp);
        return;
    }
    char line[400];
    int found = 0, hadError = 0, firstLine = 1;
    while (fgets(line, sizeof(line), ptr)) {
        stripNewline(line);
        if (firstLine) { firstLine = 0; fprintf(temp, "%s\n", line); continue; }
        if (line[0] == '\0') continue;

        char curIsbn[50];
        getField(line, 0, curIsbn, sizeof(curIsbn));
        if (streqi(curIsbn, inIsbn)) {
            found = 1;
            char p1[50], p2[50], p3[50], p4[50], p5[50];
            char p6[50], p7[50], p8[50], p9[300];
            getField(line, 1, p1, sizeof(p1));
            getField(line, 2, p2, sizeof(p2));
            getField(line, 3, p3, sizeof(p3));
            getField(line, 4, p4, sizeof(p4));
            getField(line, 5, p5, sizeof(p5));
            getField(line, 6, p6, sizeof(p6));
            getField(line, 7, p7, sizeof(p7));
            getField(line, 8, p8, sizeof(p8));
            getField(line, 9, p9, sizeof(p9));

            int oldTotal = strToNum(p6);
            int oldAvail = strToNum(p8);
            int borrowed = oldTotal - oldAvail;
            int newTotal = strToNum(inQty);
            int newAvail = newTotal - borrowed;

            if (newAvail < 0) {
                strcpy(displayBuffer, "Error: new qty less than current borrowed.");
                hadError = 1;
                fprintf(temp, "%s\n", line);
            } else {
                fprintf(temp, "%s|%s|%s|%s|%s|%s|%d|%d|%d|%s\n",
                        curIsbn, p1, p2, p3, p4, p5,
                        newTotal, borrowed, newAvail, p9);
                strcpy(displayBuffer, "Quantity updated.");
            }
        } else fprintf(temp, "%s\n", line);
    }
    fclose(ptr); fclose(temp);
    if (found && !hadError) {
        remove("books_Data.txt");
        rename("temp.txt", "books_Data.txt");
    } else {
        remove("temp.txt");
        if (!found) strcpy(displayBuffer, "Error: ISBN not found.");
    }
    memset(inIsbn, 0, sizeof(inIsbn));
    memset(inQty,  0, sizeof(inQty));
}


// ============================================================
//  REQUEST (BORROW) BOOK
// ============================================================

void PerformRequestBook(void) {
    if (strlen(inIsbn) == 0) {
        strcpy(displayBuffer, "Error: ISBN required.");
        return;
    }
    char username[50];
    getField(currentLoggedInUser, 0, username, sizeof(username));

    // Block: unpaid fine
    char fineStr[50];
    getField(currentLoggedInUser, 9, fineStr, sizeof(fineStr));
    int fineAmt = strToNum(fineStr);
    if (fineAmt > 0) {
        snprintf(displayBuffer, sizeof(displayBuffer),
                 "Blocked: pay your PKR %d fine before borrowing.", fineAmt);
        memset(inIsbn, 0, sizeof(inIsbn));
        return;
    }
    // Block: any overdue book
    char curBooks[300];
    getField(currentLoggedInUser, 8, curBooks, sizeof(curBooks));
    int today = getTodayDayNum();
    {
        int i = 0;
        while (curBooks[i] != '\0') {
            char tok[80]; int t = 0;
            while (curBooks[i] != '\0' && curBooks[i] != ',' && t < 79) tok[t++] = curBooks[i++];
            tok[t] = '\0';
            if (curBooks[i] == ',') i++;
            if (t == 0) continue;
            char isbn[50];
            int borrowDay = splitIdDay(tok, isbn, sizeof(isbn));
            if (borrowDay > 0 && today - borrowDay > LOAN_PERIOD_DAYS) {
                snprintf(displayBuffer, sizeof(displayBuffer),
                         "Blocked: book %s is overdue. Return it first.", isbn);
                memset(inIsbn, 0, sizeof(inIsbn));
                return;
            }
        }
    }
    // Batch limit
    char batchStr[3] = { username[0], username[1], '\0' };
    int batch = strToNum(batchStr);
    int limit = 1;
    if      (batch == 22) limit = 4;
    else if (batch == 23) limit = 3;
    else if (batch == 24) limit = 2;
    else if (batch == 25) limit = 1;

    int currentHeld = countTokens(curBooks);
    if (currentHeld >= limit) {
        snprintf(displayBuffer, sizeof(displayBuffer),
                 "Limit reached: batch %d may borrow %d. You hold %d.",
                 batch, limit, currentHeld);
        memset(inIsbn, 0, sizeof(inIsbn));
        return;
    }

    // Update books_Data
    FILE *ptr  = fopen("books_Data.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if (!ptr || !temp) {
        strcpy(displayBuffer, "File Error.");
        if (ptr)  fclose(ptr);
        if (temp) fclose(temp);
        return;
    }
    char line[400];
    int found = 0, firstLine = 1;
    while (fgets(line, sizeof(line), ptr)) {
        stripNewline(line);
        if (firstLine) { firstLine = 0; fprintf(temp, "%s\n", line); continue; }
        if (line[0] == '\0') continue;
        char curIsbn[50];
        getField(line, 0, curIsbn, sizeof(curIsbn));
        if (streqi(curIsbn, inIsbn)) {
            char avail[50], studList[300];
            getField(line, 8, avail,    sizeof(avail));
            getField(line, 9, studList, sizeof(studList));
            if (idInDatedList(studList, username)) {
                strcpy(displayBuffer, "You already have this book.");
                fprintf(temp, "%s\n", line);
                continue;
            }
            int av = strToNum(avail);
            if (av > 0) {
                av--; found = 1;
                char title[50], auth[50], sub[50], fac[50], sem[50], qty[50];
                getField(line, 1, title, sizeof(title));
                getField(line, 2, auth,  sizeof(auth));
                getField(line, 3, sub,   sizeof(sub));
                getField(line, 4, fac,   sizeof(fac));
                getField(line, 5, sem,   sizeof(sem));
                getField(line, 6, qty,   sizeof(qty));
                char newBorrowers[350];
                strncpy(newBorrowers, studList, sizeof(newBorrowers) - 1);
                newBorrowers[sizeof(newBorrowers) - 1] = '\0';
                appendDated(newBorrowers, sizeof(newBorrowers), username, today);
                int newBorrowed = strToNum(qty) - av;
                fprintf(temp, "%s|%s|%s|%s|%s|%s|%s|%d|%d|%s\n",
                        curIsbn, title, auth, sub, fac, sem, qty,
                        newBorrowed, av, newBorrowers);
            } else {
                strcpy(displayBuffer, "No copies available.");
                fprintf(temp, "%s\n", line);
            }
        } else fprintf(temp, "%s\n", line);
    }
    fclose(ptr); fclose(temp);

    if (found) {
        remove("books_Data.txt");
        rename("temp.txt", "books_Data.txt");

        FILE *sPtr  = fopen("studLogin.txt", "r");
        FILE *sTemp = fopen("studTemp.txt", "w");
        if (sPtr && sTemp) {
            char sLine[400];
            int sFirst = 1;
            while (fgets(sLine, sizeof(sLine), sPtr)) {
                stripNewline(sLine);
                if (sFirst) { sFirst = 0; fprintf(sTemp, "%s\n", sLine); continue; }
                if (sLine[0] == '\0') continue;
                char sId[50];
                getField(sLine, 0, sId, sizeof(sId));
                if (streqi(sId, username)) {
                    char p0[50], p1[50], p2[50], p3[50], p4[50];
                    char p5[50], p6[50], p7[50], p8[300], p9[50];
                    getField(sLine, 0, p0, sizeof(p0));
                    getField(sLine, 1, p1, sizeof(p1));
                    getField(sLine, 2, p2, sizeof(p2));
                    getField(sLine, 3, p3, sizeof(p3));
                    getField(sLine, 4, p4, sizeof(p4));
                    getField(sLine, 5, p5, sizeof(p5));
                    getField(sLine, 6, p6, sizeof(p6));
                    getField(sLine, 7, p7, sizeof(p7));
                    getField(sLine, 8, p8, sizeof(p8));
                    getField(sLine, 9, p9, sizeof(p9));
                    int bCount = strToNum(p5) + 1;
                    appendDated(p8, sizeof(p8), inIsbn, today);
                    char newLine[700];
                    snprintf(newLine, sizeof(newLine),
                             "%s|%s|%s|%s|%s|%d|%s|%s|%s|%s",
                             p0, p1, p2, p3, p4, bCount, p6, p7, p8, p9);
                    fprintf(sTemp, "%s\n", newLine);
                    strncpy(currentLoggedInUser, newLine,
                            sizeof(currentLoggedInUser) - 1);
                    currentLoggedInUser[sizeof(currentLoggedInUser) - 1] = '\0';
                } else fprintf(sTemp, "%s\n", sLine);
            }
            fclose(sPtr); fclose(sTemp);
            remove("studLogin.txt");
            rename("studTemp.txt", "studLogin.txt");
        }
        strcpy(displayBuffer, "Book requested successfully.");
    } else {
        remove("temp.txt");
        if (displayBuffer[0] == '\0') strcpy(displayBuffer, "ISBN not found.");
    }
    memset(inIsbn, 0, sizeof(inIsbn));
}


// ============================================================
//  RETURN BOOK
// ============================================================

void PerformReturnBook(void) {
    if (strlen(inIsbn) == 0) {
        strcpy(displayBuffer, "Error: ISBN required.");
        return;
    }
    char username[50];
    getField(currentLoggedInUser, 0, username, sizeof(username));

    int borrowDay = -1;
    {
        FILE *ptr = fopen("books_Data.txt", "r");
        if (!ptr) { strcpy(displayBuffer, "File Error."); return; }
        char line[400];
        int firstLine = 1;
        while (fgets(line, sizeof(line), ptr)) {
            stripNewline(line);
            if (firstLine) { firstLine = 0; continue; }
            if (line[0] == '\0') continue;
            char curIsbn[50];
            getField(line, 0, curIsbn, sizeof(curIsbn));
            if (streqi(curIsbn, inIsbn)) {
                char borrowers[300];
                getField(line, 9, borrowers, sizeof(borrowers));
                borrowDay = getDayForId(borrowers, username);
                break;
            }
        }
        fclose(ptr);
    }
    if (borrowDay < 0) {
        strcpy(displayBuffer, "You don't have this book.");
        memset(inIsbn, 0, sizeof(inIsbn));
        return;
    }
    int today = getTodayDayNum();
    int daysHeld = today - borrowDay;
    int wasLate = (daysHeld > LOAN_PERIOD_DAYS);
    int addFine = wasLate ? (daysHeld - LOAN_PERIOD_DAYS) * FINE_PER_DAY : 0;

    {
        FILE *ptr  = fopen("books_Data.txt", "r");
        FILE *temp = fopen("temp.txt", "w");
        if (!ptr || !temp) {
            strcpy(displayBuffer, "File Error.");
            if (ptr)  fclose(ptr);
            if (temp) fclose(temp);
            return;
        }
        char line[400];
        int firstLine = 1;
        while (fgets(line, sizeof(line), ptr)) {
            stripNewline(line);
            if (firstLine) { firstLine = 0; fprintf(temp, "%s\n", line); continue; }
            if (line[0] == '\0') continue;
            char curIsbn[50];
            getField(line, 0, curIsbn, sizeof(curIsbn));
            if (!streqi(curIsbn, inIsbn)) { fprintf(temp, "%s\n", line); continue; }

            char p1[50], p2[50], p3[50], p4[50], p5[50], p6[50], p8[50];
            char p9[300];
            getField(line, 1, p1, sizeof(p1));
            getField(line, 2, p2, sizeof(p2));
            getField(line, 3, p3, sizeof(p3));
            getField(line, 4, p4, sizeof(p4));
            getField(line, 5, p5, sizeof(p5));
            getField(line, 6, p6, sizeof(p6));
            getField(line, 8, p8, sizeof(p8));
            getField(line, 9, p9, sizeof(p9));
            (void)p8;

            char newBorrowers[300];
            removeFromDatedList(p9, username, newBorrowers, sizeof(newBorrowers));
            int qty = strToNum(p6);
            int newBorrowed = countTokens(newBorrowers);
            int newAvail = qty - newBorrowed;
            fprintf(temp, "%s|%s|%s|%s|%s|%s|%d|%d|%d|%s\n",
                    curIsbn, p1, p2, p3, p4, p5,
                    qty, newBorrowed, newAvail, newBorrowers);
        }
        fclose(ptr); fclose(temp);
        remove("books_Data.txt");
        rename("temp.txt", "books_Data.txt");
    }

    int newFineTotal = 0;
    {
        FILE *sPtr  = fopen("studLogin.txt", "r");
        FILE *sTemp = fopen("studTemp.txt", "w");
        if (sPtr && sTemp) {
            char sLine[400];
            int sFirst = 1;
            while (fgets(sLine, sizeof(sLine), sPtr)) {
                stripNewline(sLine);
                if (sFirst) { sFirst = 0; fprintf(sTemp, "%s\n", sLine); continue; }
                if (sLine[0] == '\0') continue;
                char sId[50];
                getField(sLine, 0, sId, sizeof(sId));
                if (!streqi(sId, username)) { fprintf(sTemp, "%s\n", sLine); continue; }
                char p0[50], p1[50], p2[50], p3[50], p4[50];
                char p5[50], p6[50], p7[50], p8[300], p9[50];
                getField(sLine, 0, p0, sizeof(p0));
                getField(sLine, 1, p1, sizeof(p1));
                getField(sLine, 2, p2, sizeof(p2));
                getField(sLine, 3, p3, sizeof(p3));
                getField(sLine, 4, p4, sizeof(p4));
                getField(sLine, 5, p5, sizeof(p5));
                getField(sLine, 6, p6, sizeof(p6));
                getField(sLine, 7, p7, sizeof(p7));
                getField(sLine, 8, p8, sizeof(p8));
                getField(sLine, 9, p9, sizeof(p9));

                int ontime = strToNum(p6);
                int late   = strToNum(p7);
                int fine   = strToNum(p9);
                if (wasLate) { late++; fine += addFine; }
                else         { ontime++; }
                newFineTotal = fine;

                char newCurrent[300];
                removeFromDatedList(p8, inIsbn, newCurrent, sizeof(newCurrent));

                char newLine[700];
                snprintf(newLine, sizeof(newLine),
                         "%s|%s|%s|%s|%s|%s|%d|%d|%s|%d",
                         p0, p1, p2, p3, p4, p5, ontime, late, newCurrent, fine);
                fprintf(sTemp, "%s\n", newLine);
                strncpy(currentLoggedInUser, newLine,
                        sizeof(currentLoggedInUser) - 1);
                currentLoggedInUser[sizeof(currentLoggedInUser) - 1] = '\0';
            }
            fclose(sPtr); fclose(sTemp);
            remove("studLogin.txt");
            rename("studTemp.txt", "studLogin.txt");
        }
    }

    if (wasLate) {
        snprintf(displayBuffer, sizeof(displayBuffer),
                 "Returned %d days late. Fine added: PKR %d.\nYour total fine is now PKR %d.",
                 daysHeld - LOAN_PERIOD_DAYS, addFine, newFineTotal);
    } else {
        snprintf(displayBuffer, sizeof(displayBuffer),
                 "Returned on time (held %d day%s). Thanks!",
                 daysHeld, daysHeld == 1 ? "" : "s");
    }
    memset(inIsbn, 0, sizeof(inIsbn));
}


// ============================================================
//  REMOVE BOOK (cascades to students)
// ============================================================

void PerformRemoveBook(void) {
    if (strlen(inIsbn) == 0) {
        strcpy(displayBuffer, "Error: ISBN required.");
        return;
    }
    char affectedBorrowers[300] = {0};
    int bookExists = 0;
    {
        FILE *ptr = fopen("books_Data.txt", "r");
        if (!ptr) { strcpy(displayBuffer, "File Error."); return; }
        char line[400];
        int firstLine = 1;
        while (fgets(line, sizeof(line), ptr)) {
            stripNewline(line);
            if (firstLine) { firstLine = 0; continue; }
            if (line[0] == '\0') continue;
            char curIsbn[50];
            getField(line, 0, curIsbn, sizeof(curIsbn));
            if (streqi(curIsbn, inIsbn)) {
                bookExists = 1;
                getField(line, 9, affectedBorrowers, sizeof(affectedBorrowers));
                break;
            }
        }
        fclose(ptr);
    }
    if (!bookExists) {
        strcpy(displayBuffer, "ISBN not found.");
        memset(inIsbn, 0, sizeof(inIsbn));
        return;
    }
    {
        FILE *ptr = fopen("books_Data.txt", "r");
        FILE *temp = fopen("temp.txt", "w");
        if (!ptr || !temp) {
            strcpy(displayBuffer, "File Error.");
            if (ptr)  fclose(ptr);
            if (temp) fclose(temp);
            return;
        }
        char line[400];
        int firstLine = 1;
        while (fgets(line, sizeof(line), ptr)) {
            stripNewline(line);
            if (firstLine) { firstLine = 0; fprintf(temp, "%s\n", line); continue; }
            if (line[0] == '\0') continue;
            char curIsbn[50];
            getField(line, 0, curIsbn, sizeof(curIsbn));
            if (!streqi(curIsbn, inIsbn)) fprintf(temp, "%s\n", line);
        }
        fclose(ptr); fclose(temp);
        remove("books_Data.txt");
        rename("temp.txt", "books_Data.txt");
    }
    if (strlen(affectedBorrowers) > 0) {
        FILE *sPtr  = fopen("studLogin.txt", "r");
        FILE *sTemp = fopen("studTemp.txt", "w");
        if (sPtr && sTemp) {
            char sLine[400];
            int sFirst = 1;
            while (fgets(sLine, sizeof(sLine), sPtr)) {
                stripNewline(sLine);
                if (sFirst) { sFirst = 0; fprintf(sTemp, "%s\n", sLine); continue; }
                if (sLine[0] == '\0') continue;
                char sId[50];
                getField(sLine, 0, sId, sizeof(sId));
                if (!idInDatedList(affectedBorrowers, sId)) {
                    fprintf(sTemp, "%s\n", sLine);
                    continue;
                }
                char p0[50], p1[50], p2[50], p3[50], p4[50];
                char p5[50], p6[50], p7[50], p8[300], p9[50];
                getField(sLine, 0, p0, sizeof(p0));
                getField(sLine, 1, p1, sizeof(p1));
                getField(sLine, 2, p2, sizeof(p2));
                getField(sLine, 3, p3, sizeof(p3));
                getField(sLine, 4, p4, sizeof(p4));
                getField(sLine, 5, p5, sizeof(p5));
                getField(sLine, 6, p6, sizeof(p6));
                getField(sLine, 7, p7, sizeof(p7));
                getField(sLine, 8, p8, sizeof(p8));
                getField(sLine, 9, p9, sizeof(p9));

                int bCount = strToNum(p5);
                int hadIt  = idInDatedList(p8, inIsbn);
                if (hadIt && bCount > 0) bCount--;

                char newCurrent[300];
                removeFromDatedList(p8, inIsbn, newCurrent, sizeof(newCurrent));

                char newLine[700];
                snprintf(newLine, sizeof(newLine),
                         "%s|%s|%s|%s|%s|%d|%s|%s|%s|%s",
                         p0, p1, p2, p3, p4, bCount, p6, p7, newCurrent, p9);
                fprintf(sTemp, "%s\n", newLine);
            }
            fclose(sPtr); fclose(sTemp);
            remove("studLogin.txt");
            rename("studTemp.txt", "studLogin.txt");
        }
    }
    strcpy(displayBuffer, "Book removed successfully.");
    memset(inIsbn, 0, sizeof(inIsbn));
}


// ============================================================
//  ADMIN: CLEAR A STUDENT'S FINE
// ============================================================

void PerformClearFine(void) {
    char studId[50];
    getField(viewedStudent, 0, studId, sizeof(studId));
    if (strlen(studId) == 0) return;

    FILE *sPtr  = fopen("studLogin.txt", "r");
    FILE *sTemp = fopen("studTemp.txt", "w");
    if (!sPtr || !sTemp) {
        strcpy(displayBuffer, "File Error.");
        if (sPtr)  fclose(sPtr);
        if (sTemp) fclose(sTemp);
        return;
    }
    char sLine[400];
    int sFirst = 1;
    while (fgets(sLine, sizeof(sLine), sPtr)) {
        stripNewline(sLine);
        if (sFirst) { sFirst = 0; fprintf(sTemp, "%s\n", sLine); continue; }
        if (sLine[0] == '\0') continue;
        char sId[50];
        getField(sLine, 0, sId, sizeof(sId));
        if (!streqi(sId, studId)) { fprintf(sTemp, "%s\n", sLine); continue; }
        char p0[50], p1[50], p2[50], p3[50], p4[50];
        char p5[50], p6[50], p7[50], p8[300];
        getField(sLine, 0, p0, sizeof(p0));
        getField(sLine, 1, p1, sizeof(p1));
        getField(sLine, 2, p2, sizeof(p2));
        getField(sLine, 3, p3, sizeof(p3));
        getField(sLine, 4, p4, sizeof(p4));
        getField(sLine, 5, p5, sizeof(p5));
        getField(sLine, 6, p6, sizeof(p6));
        getField(sLine, 7, p7, sizeof(p7));
        getField(sLine, 8, p8, sizeof(p8));
        char newLine[700];
        snprintf(newLine, sizeof(newLine),
                 "%s|%s|%s|%s|%s|%s|%s|%s|%s|0",
                 p0, p1, p2, p3, p4, p5, p6, p7, p8);
        fprintf(sTemp, "%s\n", newLine);
        strncpy(viewedStudent, newLine, sizeof(viewedStudent) - 1);
        viewedStudent[sizeof(viewedStudent) - 1] = '\0';
    }
    fclose(sPtr); fclose(sTemp);
    remove("studLogin.txt");
    rename("studTemp.txt", "studLogin.txt");
    strcpy(displayBuffer, "Fine cleared.");
}


// ============================================================
//  PROFILE-DRAWING (shared between student-self and admin-view)
// ============================================================

int DrawStudentProfile(const char *record, int startY) {
    int y = startY;
    int gap = 36;

    char id[50], name[50], fac[50], sem[50];
    char bb[50], ot[50], lt[50], curBooks[300], fine[50];
    getField(record, 0, id,       sizeof(id));
    getField(record, 2, name,     sizeof(name));
    getField(record, 3, sem,      sizeof(sem));
    getField(record, 4, fac,      sizeof(fac));
    getField(record, 5, bb,       sizeof(bb));
    getField(record, 6, ot,       sizeof(ot));
    getField(record, 7, lt,       sizeof(lt));
    getField(record, 8, curBooks, sizeof(curBooks));
    getField(record, 9, fine,     sizeof(fine));

    int labelX = 70;
    int valueX = 280;

    DT("Name",      labelX, y, 26, COLOR_MUTED); DT(name, valueX, y, 26, COLOR_TEXT);    y += gap;
    DT("ID",        labelX, y, 26, COLOR_MUTED); DT(id,   valueX, y, 26, COLOR_TEXT);    y += gap;
    DT("Faculty",   labelX, y, 26, COLOR_MUTED); DT(fac,  valueX, y, 26, COLOR_TEXT);    y += gap;
    DT("Semester",  labelX, y, 26, COLOR_MUTED); DT(sem,  valueX, y, 26, COLOR_TEXT);    y += gap + 4;

    DrawLine(labelX, y, SCREEN_WIDTH - 70, y, COLOR_BORDER);
    y += 12;

    int fineAmt = strToNum(fine);
    char buf[120];
    snprintf(buf, sizeof(buf), "Lifetime borrowed     %s", bb);
    DT(buf, labelX, y, 26, COLOR_TEXT); y += gap;
    snprintf(buf, sizeof(buf), "Returned on time      %s", ot);
    DT(buf, labelX, y, 26, COLOR_SUCCESS); y += gap;
    snprintf(buf, sizeof(buf), "Returned late         %s", lt);
    DT(buf, labelX, y, 18, strToNum(lt) > 0 ? COLOR_DANGER : COLOR_TEXT); y += gap;
    snprintf(buf, sizeof(buf), "Outstanding fine      PKR %d", fineAmt);
    DT(buf, labelX, y, 18, fineAmt > 0 ? COLOR_DANGER : COLOR_SUCCESS); y += gap + 4;

    DrawLine(labelX, y, SCREEN_WIDTH - 70, y, COLOR_BORDER);
    y += 12;
    DT("Currently borrowed:", labelX, y, 26, COLOR_TEXT); y += gap;

    if (curBooks[0] == '\0') {
        DT("  (none)", labelX, y, 26, COLOR_MUTED);
        y += gap;
    } else {
        int today = getTodayDayNum();
        int i = 0;
        // Hard cap so we never collide with the footer buttons.
        int yMax = SCREEN_HEIGHT - FOOTER_RESERVE - 30;
        while (curBooks[i] != '\0' && y < yMax) {
            char tok[80]; int t = 0;
            while (curBooks[i] != '\0' && curBooks[i] != ',' && t < 79) tok[t++] = curBooks[i++];
            tok[t] = '\0';
            if (curBooks[i] == ',') i++;
            if (t == 0) continue;
            char isbn[50];
            int borrowDay = splitIdDay(tok, isbn, sizeof(isbn));
            int held = (borrowDay > 0) ? (today - borrowDay) : 0;
            char row[140];
            if (borrowDay > 0 && held > LOAN_PERIOD_DAYS) {
                snprintf(row, sizeof(row),
                         "  %s    %d days held    OVERDUE %d", isbn, held, held - LOAN_PERIOD_DAYS);
                DT(row, labelX, y, 26, COLOR_DANGER);
            } else {
                snprintf(row, sizeof(row),
                         "  %s    %d days held    on time", isbn, held);
                DT(row, labelX, y, 26, COLOR_TEXT);
            }
            y += gap;
        }
    }
    return y;
}


// ============================================================
//  MAIN LOOP
// ============================================================

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Library Management System");
    SetTargetFPS(60);

    // Load a nice TTF system font and turn on smoothing so it doesn't look
    // pixelated when scaled to our actual draw sizes (16-26px).
    appFont = LoadAppFont();
    if (appFontLoaded)
        SetTextureFilter(appFont.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(COLOR_BG);

        // Header bar
        DrawRectangle(0, 0, SCREEN_WIDTH, HEADER_H, COLOR_HEADER);
        DT("Library Management System", 28, 20, 36, COLOR_HEADER_TEXT);

        // Footer button positions (used by most screens)
        float backY    = SCREEN_HEIGHT - 65;
        float backX    = 30;
        float logoutY  = SCREEN_HEIGHT - 65;
        float logoutX  = SCREEN_WIDTH - BTN_W_SMALL - 30;

        if (currentState == STATE_LOGIN) {
            // Centred card
            int cardX = 250, cardY = 90, cardW = 500, cardH = 440;
            DrawCard((Rectangle){cardX, cardY, cardW, cardH});
            DrawHeading("Sign in", cardX + 36, cardY + 28);
            DT("Enter your library credentials.", cardX + 36, cardY + 78, 22, COLOR_MUTED);

            DrawLabel("Username", cardX + 36, cardY + 118);
            if (GuiInput((Rectangle){cardX + 36, cardY + 148, cardW - 72, BTN_H},
                         usernameInput, MAX_INPUT_CHARS, activeBox == 1)) activeBox = 1;

            DrawLabel("Password", cardX + 36, cardY + 218);
            if (GuiInput((Rectangle){cardX + 36, cardY + 248, cardW - 72, BTN_H},
                         passwordInput, MAX_INPUT_CHARS, activeBox == 2)) activeBox = 2;

            float btnY = cardY + 330;
            if (GuiBtn((Rectangle){cardX + 36, btnY, 200, BTN_H}, "Student Login", BTN_PRIMARY)) {
                if (VerifyLogin(usernameInput, passwordInput, 0)) {
                    isAdmin = 0; currentState = STATE_STUDENT_MENU; displayBuffer[0] = '\0';
                } else strcpy(displayBuffer, "Invalid student credentials.");
            }
            if (GuiBtn((Rectangle){cardX + 252, btnY, 200, BTN_H}, "Admin Login", BTN_SECONDARY)) {
                if (VerifyLogin(usernameInput, passwordInput, 1)) {
                    isAdmin = 1; currentState = STATE_ADMIN_MENU; displayBuffer[0] = '\0';
                } else strcpy(displayBuffer, "Invalid admin credentials.");
            }
            DT(displayBuffer, cardX + 30, cardY + cardH + 12, 22, COLOR_DANGER);
        }

        else if (currentState == STATE_ADMIN_MENU) {
            // Centre headings
            const char *h = "Admin Dashboard";
            const char *sub = "Manage the library catalog and student records.";
            DT(h,   (SCREEN_WIDTH - MT(h,   32)) / 2, 100, 42, COLOR_TEXT);
            DT(sub, (SCREEN_WIDTH - MT(sub, 20)) / 2, 144, 28, COLOR_MUTED);

            // Centre buttons
            float bx = (SCREEN_WIDTH - BTN_W_MENU) / 2.0f;
            float y0 = 200;
            if (GuiBtn((Rectangle){bx, y0,                            BTN_W_MENU, BTN_H}, "Search Books",  BTN_PRIMARY))   currentState = STATE_SEARCH_MENU;
            if (GuiBtn((Rectangle){bx, y0 + 1*(BTN_H + BTN_GAP),     BTN_W_MENU, BTN_H}, "Add Book",      BTN_SECONDARY)) { currentState = STATE_ADD_BOOK;             displayBuffer[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 2*(BTN_H + BTN_GAP),     BTN_W_MENU, BTN_H}, "Update Book",   BTN_SECONDARY)) { currentState = STATE_UPDATE_BOOK;          displayBuffer[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 3*(BTN_H + BTN_GAP),     BTN_W_MENU, BTN_H}, "Remove Book",   BTN_DANGER))    { currentState = STATE_REMOVE_BOOK;          displayBuffer[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 4*(BTN_H + BTN_GAP),     BTN_W_MENU, BTN_H}, "View Student",  BTN_SECONDARY)) { currentState = STATE_VIEW_STUDENT_INPUT;   memset(inStudId, 0, sizeof(inStudId)); displayBuffer[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 5*(BTN_H + BTN_GAP),     BTN_W_MENU, BTN_H}, "Profile",       BTN_SECONDARY)) currentState = STATE_PROFILE;

            if (GuiBtn((Rectangle){logoutX, logoutY, BTN_W_SMALL, BTN_H}, "Logout", BTN_DANGER)) {
                currentState = STATE_LOGIN; displayBuffer[0] = '\0';
                memset(usernameInput, 0, sizeof(usernameInput));
                memset(passwordInput, 0, sizeof(passwordInput));
                isAdmin = 0;
            }
        }

        else if (currentState == STATE_STUDENT_MENU) {
            const char *h   = "Student Portal";
            const char *sub = "Search, borrow, or return books.";
            DT(h,   (SCREEN_WIDTH - MT(h,   32)) / 2, 100, 42, COLOR_TEXT);
            DT(sub, (SCREEN_WIDTH - MT(sub, 20)) / 2, 144, 28, COLOR_MUTED);

            float bx = (SCREEN_WIDTH - BTN_W_MENU) / 2.0f;
            float y0 = 220;
            if (GuiBtn((Rectangle){bx, y0,                        BTN_W_MENU, BTN_H}, "Search Books",  BTN_PRIMARY))   currentState = STATE_SEARCH_MENU;
            if (GuiBtn((Rectangle){bx, y0 + 1*(BTN_H + BTN_GAP), BTN_W_MENU, BTN_H}, "Request Book",  BTN_SECONDARY)) { currentState = STATE_REQUEST_BOOK; displayBuffer[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 2*(BTN_H + BTN_GAP), BTN_W_MENU, BTN_H}, "Return Book",   BTN_SUCCESS))   { currentState = STATE_RETURN_BOOK;  displayBuffer[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 3*(BTN_H + BTN_GAP), BTN_W_MENU, BTN_H}, "Profile",       BTN_SECONDARY)) currentState = STATE_PROFILE;

            if (GuiBtn((Rectangle){logoutX, logoutY, BTN_W_SMALL, BTN_H}, "Logout", BTN_DANGER)) {
                currentState = STATE_LOGIN; displayBuffer[0] = '\0';
                memset(usernameInput, 0, sizeof(usernameInput));
                memset(passwordInput, 0, sizeof(passwordInput));
                isAdmin = 0;
            }
        }

        else if (currentState == STATE_SEARCH_MENU) {
            const char *h   = "Search Books";
            const char *sub = "Pick what to search by:";
            DT(h,   (SCREEN_WIDTH - MT(h,   32)) / 2, 100, 42, COLOR_TEXT);
            DT(sub, (SCREEN_WIDTH - MT(sub, 20)) / 2, 144, 28, COLOR_MUTED);

            float bx = (SCREEN_WIDTH - BTN_W_MENU) / 2.0f;
            float y0 = 210;
            if (GuiBtn((Rectangle){bx, y0,                        BTN_W_MENU, BTN_H}, "By ISBN",           BTN_SECONDARY)) { currentSearchMode = 1; currentState = STATE_SEARCH_INPUT; searchInput[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 1*(BTN_H + BTN_GAP), BTN_W_MENU, BTN_H}, "By Title",          BTN_SECONDARY)) { currentSearchMode = 2; currentState = STATE_SEARCH_INPUT; searchInput[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 2*(BTN_H + BTN_GAP), BTN_W_MENU, BTN_H}, "By Author",         BTN_SECONDARY)) { currentSearchMode = 3; currentState = STATE_SEARCH_INPUT; searchInput[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 3*(BTN_H + BTN_GAP), BTN_W_MENU, BTN_H}, "By Faculty & Sem",  BTN_SECONDARY)) { currentSearchMode = 4; currentState = STATE_SEARCH_INPUT; searchInput[0] = '\0'; searchInput2[0] = '\0'; }
            if (GuiBtn((Rectangle){bx, y0 + 4*(BTN_H + BTN_GAP), BTN_W_MENU, BTN_H}, "By Subject",        BTN_SECONDARY)) { currentSearchMode = 5; currentState = STATE_SEARCH_INPUT; searchInput[0] = '\0'; }

            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY))
                currentState = isAdmin ? STATE_ADMIN_MENU : STATE_STUDENT_MENU;
        }

        else if (currentState == STATE_SEARCH_INPUT) {
            const char *h = "Search";
            DT(h, (SCREEN_WIDTH - MT(h, 32)) / 2, 100, 42, COLOR_TEXT);

            const char *lbl = "Query";
            if (currentSearchMode == 1) lbl = "ISBN";
            if (currentSearchMode == 2) lbl = "Title";
            if (currentSearchMode == 3) lbl = "Author";
            if (currentSearchMode == 4) lbl = "Faculty";
            if (currentSearchMode == 5) lbl = "Subject";

            float cx = (SCREEN_WIDTH - 400) / 2.0f;
            DrawLabel(lbl, (int)cx, 170);
            if (GuiInput((Rectangle){cx, 200, 400, BTN_H}, searchInput, 50, activeBox == 3)) activeBox = 3;
            float btnY = 280;
            if (currentSearchMode == 4) {
                DrawLabel("Semester", (int)cx, 272);
                if (GuiInput((Rectangle){cx, 302, 150, BTN_H}, searchInput2, 10, activeBox == 4)) activeBox = 4;
                btnY = 392;
            }
            float bx = (SCREEN_WIDTH - BTN_W_ACTION) / 2.0f;
            if (GuiBtn((Rectangle){bx, btnY, BTN_W_ACTION, BTN_H}, "Search", BTN_PRIMARY)) {
                PerformSearch(currentSearchMode, searchInput, searchInput2, isAdmin);
                currentState = STATE_SEARCH_RESULT;
            }
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY))
                currentState = STATE_SEARCH_MENU;
        }

        else if (currentState == STATE_SEARCH_RESULT) {
            const char *h = "Search Results";
            DT(h, (SCREEN_WIDTH - MT(h, 32)) / 2, 100, 42, COLOR_TEXT);
            int y = 155, lineH = 34, x = 60;
            char *p = displayBuffer;
            while (*p && y < SCREEN_HEIGHT - FOOTER_RESERVE - 20) {
                char lineBuf[300]; int n = 0;
                while (*p && *p != '\n' && n < (int)sizeof(lineBuf) - 1) lineBuf[n++] = *p++;
                lineBuf[n] = '\0';
                if (*p == '\n') p++;
                Color c = COLOR_TEXT;
                if (strncmp(lineBuf, "------", 6) == 0) { c = COLOR_BORDER; }
                else if (strncmp(lineBuf, "ISBN:", 5) == 0) c = COLOR_PRIMARY;
                else if (strncmp(lineBuf, "Borrowed By:", 12) == 0) c = COLOR_MUTED;
                DT(lineBuf, x, y, 18, c);
                y += lineH;
            }
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY))
                currentState = STATE_SEARCH_MENU;
        }

        else if (currentState == STATE_ADD_BOOK) {
            const char *h = "Add New Book";
            DT(h, (SCREEN_WIDTH - MT(h, 32)) / 2, 100, 42, COLOR_TEXT);
            float cx = (SCREEN_WIDTH - 400) / 2.0f;
            int y = 162, gap = 60, inputW = 400;
            DrawLabel("ISBN",     (int)cx, y - 22); if (GuiInput((Rectangle){cx, y, inputW, BTN_H}, inIsbn,  50, activeBox == 10)) activeBox = 10; y += gap;
            DrawLabel("Title",    (int)cx, y - 22); if (GuiInput((Rectangle){cx, y, inputW, BTN_H}, inTitle, 50, activeBox == 11)) activeBox = 11; y += gap;
            DrawLabel("Author",   (int)cx, y - 22); if (GuiInput((Rectangle){cx, y, inputW, BTN_H}, inAuth,  50, activeBox == 12)) activeBox = 12; y += gap;
            DrawLabel("Subject",  (int)cx, y - 22); if (GuiInput((Rectangle){cx, y, inputW, BTN_H}, inSub,   50, activeBox == 13)) activeBox = 13; y += gap;
            DrawLabel("Faculty",  (int)cx, y - 22); if (GuiInput((Rectangle){cx, y, inputW, BTN_H}, inFac,   50, activeBox == 14)) activeBox = 14; y += gap;
            DrawLabel("Semester", (int)cx, y - 22); if (GuiInput((Rectangle){cx, y, inputW, BTN_H}, inSem,   50, activeBox == 15)) activeBox = 15; y += gap;
            DrawLabel("Quantity", (int)cx, y - 22); if (GuiInput((Rectangle){cx, y, inputW, BTN_H}, inQty,   50, activeBox == 16)) activeBox = 16; y += gap;
            float bx = (SCREEN_WIDTH - BTN_W_ACTION) / 2.0f;
            if (GuiBtn((Rectangle){bx, y, BTN_W_ACTION, BTN_H}, "Add Book", BTN_PRIMARY)) PerformAddBook();
            DT(displayBuffer, (int)((SCREEN_WIDTH - MT(displayBuffer, 18)) / 2), y + BTN_H + 12, 18,
               (strstr(displayBuffer, "Error") || strstr(displayBuffer, "exists")) ? COLOR_DANGER : COLOR_SUCCESS);
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY)) currentState = STATE_ADMIN_MENU;
        }

        else if (currentState == STATE_UPDATE_BOOK) {
            const char *h = "Update Book Quantity";
            DT(h, (SCREEN_WIDTH - MT(h, 32)) / 2, 100, 42, COLOR_TEXT);
            float cx = (SCREEN_WIDTH - 400) / 2.0f;
            DrawLabel("ISBN",              (int)cx, 164);
            if (GuiInput((Rectangle){cx, 202, 400, BTN_H}, inIsbn, 50, activeBox == 40)) activeBox = 40;
            DrawLabel("New Total Quantity", (int)cx, 280);
            if (GuiInput((Rectangle){cx, 308, 400, BTN_H}, inQty,  50, activeBox == 41)) activeBox = 41;
            float bx = (SCREEN_WIDTH - BTN_W_ACTION) / 2.0f;
            if (GuiBtn((Rectangle){bx, 386, BTN_W_ACTION, BTN_H}, "Update", BTN_PRIMARY)) PerformUpdateBook();
            DT(displayBuffer, (int)((SCREEN_WIDTH - MT(displayBuffer, 22)) / 2), 456, 22,
               strstr(displayBuffer, "Error") ? COLOR_DANGER : COLOR_SUCCESS);
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY)) currentState = STATE_ADMIN_MENU;
        }

        else if (currentState == STATE_REMOVE_BOOK) {
            const char *h = "Remove Book";
            DT(h, (SCREEN_WIDTH - MT(h, 32)) / 2, 100, 42, COLOR_TEXT);
            float cx = (SCREEN_WIDTH - 400) / 2.0f;
            DrawLabel("ISBN", (int)cx, 172);
            if (GuiInput((Rectangle){cx, 202, 400, BTN_H}, inIsbn, 50, activeBox == 20)) activeBox = 20;
            float bx = (SCREEN_WIDTH - BTN_W_ACTION) / 2.0f;
            if (GuiBtn((Rectangle){bx, 282, BTN_W_ACTION, BTN_H}, "Remove", BTN_DANGER)) PerformRemoveBook();
            DT(displayBuffer, (int)((SCREEN_WIDTH - MT(displayBuffer, 22)) / 2), 354, 22,
               strstr(displayBuffer, "not found") || strstr(displayBuffer, "Error") ? COLOR_DANGER : COLOR_SUCCESS);
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY)) currentState = STATE_ADMIN_MENU;
        }

        else if (currentState == STATE_REQUEST_BOOK) {
            const char *h = "Request a Book";
            DT(h, (SCREEN_WIDTH - MT(h, 32)) / 2, 100, 42, COLOR_TEXT);
            float cx = (SCREEN_WIDTH - 400) / 2.0f;
            DrawLabel("ISBN", (int)cx, 172);
            if (GuiInput((Rectangle){cx, 202, 400, BTN_H}, inIsbn, 50, activeBox == 30)) activeBox = 30;
            float bx = (SCREEN_WIDTH - BTN_W_ACTION) / 2.0f;
            if (GuiBtn((Rectangle){bx, 282, BTN_W_ACTION, BTN_H}, "Request", BTN_PRIMARY)) PerformRequestBook();
            DT(displayBuffer, (int)((SCREEN_WIDTH - MT(displayBuffer, 22)) / 2), 354, 22,
               (strstr(displayBuffer, "Blocked") || strstr(displayBuffer, "Error") || strstr(displayBuffer, "not found") || strstr(displayBuffer, "No copies"))
               ? COLOR_DANGER : COLOR_SUCCESS);
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY)) currentState = STATE_STUDENT_MENU;
        }

        else if (currentState == STATE_RETURN_BOOK) {
            const char *h = "Return a Book";
            DT(h, (SCREEN_WIDTH - MT(h, 32)) / 2, 100, 42, COLOR_TEXT);
            float cx = (SCREEN_WIDTH - 400) / 2.0f;
            DrawLabel("ISBN", (int)cx, 172);
            if (GuiInput((Rectangle){cx, 202, 400, BTN_H}, inIsbn, 50, activeBox == 50)) activeBox = 50;
            float bx = (SCREEN_WIDTH - BTN_W_ACTION) / 2.0f;
            if (GuiBtn((Rectangle){bx, 282, BTN_W_ACTION, BTN_H}, "Return", BTN_SUCCESS)) PerformReturnBook();
            int y = 334;
            char *p = displayBuffer;
            while (*p && y < SCREEN_HEIGHT - FOOTER_RESERVE - 20) {
                char lineBuf[300]; int n = 0;
                while (*p && *p != '\n' && n < (int)sizeof(lineBuf) - 1) lineBuf[n++] = *p++;
                lineBuf[n] = '\0';
                if (*p == '\n') p++;
                Color c = strstr(lineBuf, "late") ? COLOR_DANGER : COLOR_SUCCESS;
                DT(lineBuf, (int)((SCREEN_WIDTH - MT(lineBuf, 18)) / 2), y, 18, c);
                y += 28;
            }
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY)) currentState = STATE_STUDENT_MENU;
        }

        else if (currentState == STATE_VIEW_STUDENT_INPUT) {
            const char *h = "View Student Record";
            DT(h, (SCREEN_WIDTH - MT(h, 32)) / 2, 100, 42, COLOR_TEXT);
            float cx = (SCREEN_WIDTH - 400) / 2.0f;
            DrawLabel("Student ID", (int)cx, 164);
            if (GuiInput((Rectangle){cx, 202, 400, BTN_H}, inStudId, 50, activeBox == 60)) activeBox = 60;
            float bx = (SCREEN_WIDTH - BTN_W_ACTION) / 2.0f;
            if (GuiBtn((Rectangle){bx, 282, BTN_W_ACTION, BTN_H}, "View", BTN_PRIMARY)) {
                if (LoadStudentRecord(inStudId)) { currentState = STATE_VIEW_STUDENT_RESULT; displayBuffer[0] = '\0'; }
                else strcpy(displayBuffer, "Student ID not found.");
            }
            DT(displayBuffer, (int)((SCREEN_WIDTH - MT(displayBuffer, 18)) / 2), 334, 26, COLOR_DANGER);
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY)) currentState = STATE_ADMIN_MENU;
        }

        else if (currentState == STATE_VIEW_STUDENT_RESULT) {
            const char *h   = "Student Record";
            const char *sub = "Admin view";
            DT(h,   (SCREEN_WIDTH - MT(h,   32)) / 2, 100, 42, COLOR_TEXT);
            DT(sub, (SCREEN_WIDTH - MT(sub, 20)) / 2, 144, 28, COLOR_MUTED);
            DrawStudentProfile(viewedStudent, 180);
            char fineFld[50];
            getField(viewedStudent, 9, fineFld, sizeof(fineFld));
            int fineAmt = strToNum(fineFld);
            DT(displayBuffer, (int)((SCREEN_WIDTH - MT(displayBuffer, 18)) / 2), SCREEN_HEIGHT - 98, 26, COLOR_SUCCESS);
            if (fineAmt > 0 && GuiBtn((Rectangle){logoutX - 30, logoutY, BTN_W_ACTION, BTN_H}, "Clear Fine", BTN_SUCCESS))
                PerformClearFine();
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY)) currentState = STATE_VIEW_STUDENT_INPUT;
        }

        else if (currentState == STATE_PROFILE) {
            const char *h = "Profile";
            DT(h, (SCREEN_WIDTH - MT(h, 32)) / 2, 100, 42, COLOR_TEXT);
            if (!isAdmin) {
                DrawStudentProfile(currentLoggedInUser, 155);
            } else {
                char id[50], name[50], role[50], email[50];
                getField(currentLoggedInUser, 0, id,    sizeof(id));
                getField(currentLoggedInUser, 2, name,  sizeof(name));
                getField(currentLoggedInUser, 3, role,  sizeof(role));
                getField(currentLoggedInUser, 4, email, sizeof(email));
                int y = 170, gap = 38;
                int lx = (SCREEN_WIDTH / 2) - 200;
                int vx = (SCREEN_WIDTH / 2) - 10;
                DT("Name",  lx, y, 28, COLOR_MUTED); DT(name,  vx, y, 28, COLOR_TEXT); y += gap;
                DT("ID",    lx, y, 28, COLOR_MUTED); DT(id,    vx, y, 28, COLOR_TEXT); y += gap;
                DT("Role",  lx, y, 28, COLOR_MUTED); DT(role,  vx, y, 28, COLOR_TEXT); y += gap;
                DT("Email", lx, y, 28, COLOR_MUTED); DT(email, vx, y, 28, COLOR_TEXT);
            }
            if (GuiBtn((Rectangle){backX, backY, BTN_W_SMALL, BTN_H}, "Back", BTN_SECONDARY))
                currentState = isAdmin ? STATE_ADMIN_MENU : STATE_STUDENT_MENU;
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}