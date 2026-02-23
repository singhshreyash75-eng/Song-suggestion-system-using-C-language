/*
  song_suggester_clean_mac.c
  Cleaned single-file Song Suggestion System for macOS (Apple clang)
  - No windows.h, no Sleep(), no empty-body while loops
  - Uses clearInputBuffer() for flushing stdin safely (no warnings)
  - ANSI colors for macOS/Linux/Windows 10+ terminals
  - Features: seeded library, artist/genre suggestion, search, favorites,
    add-song persistence, export playlists, history, simple user switch
  Compile:
    gcc -std=c11 -Wall -Wextra song_suggester_clean_mac.c -o song_suggester_clean_mac
  Run:
    ./song_suggester_clean_mac
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  #include <unistd.h>
  #include <sys/stat.h>
#endif

/* ---------- CONFIG ---------- */
#define MAX_SONGS      1400
#define MAX_TITLE      128
#define MAX_NAME       64
#define MAX_GENRE      64
#define MAX_ARTISTS    350
#define MAX_GENRES     350
#define MAX_HISTORY    200
#define FAVORITES_FILE "favorites.txt"
#define ADDED_FILE     "added_songs.txt"
#define PLAYLIST_DIR   "playlists"

/* ---------- ANSI COLORS ---------- */
#define CLR_RESET  "\x1b[0m"
#define CLR_BOLD   "\x1b[1m"
#define CLR_RED    "\x1b[31m"
#define CLR_GREEN  "\x1b[32m"
#define CLR_YELLOW "\x1b[33m"
#define CLR_BLUE   "\x1b[34m"
#define CLR_MAG    "\x1b[35m"
#define CLR_CYAN   "\x1b[36m"

/* ---------- DATA STRUCTURES ---------- */
typedef struct {
    char title[MAX_TITLE];
    char artist[MAX_NAME];
    char genre[MAX_GENRE];
    int liked;
} Song;

typedef struct {
    char name[MAX_NAME];
    int indices[1200];
    int count;
} Bucket;

/* ---------- GLOBAL STORAGE ---------- */
static Song library_db[MAX_SONGS];
static int library_count = 0;

static Bucket artist_buckets[MAX_ARTISTS];
static int artist_count = 0;

static Bucket genre_buckets[MAX_GENRES];
static int genre_count = 0;

static int favorites[MAX_SONGS];
static int fav_count = 0;

static int history[MAX_HISTORY];
static int history_count = 0;

static char current_user[MAX_NAME] = "guest";

/* ---------- UTILITIES ---------- */

/* cross-platform millisecond sleep */
static void sleep_ms(unsigned int ms) {
#if defined(_WIN32)
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

/* safe fgets wrapper */
static void read_line(char *buf, size_t n) {
    if (!fgets(buf, (int)n, stdin)) {
        buf[0] = '\0';
        return;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
}

/* trim whitespace both ends */
static void trim_whitespace(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = '\0';
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

/* case-insensitive substring search (portable) */
static char *ci_strstr(const char *hay, const char *needle) {
    if (!*needle) return (char *)hay;
    size_t needle_len = strlen(needle);
    for (; *hay; ++hay) {
        if (tolower((unsigned char)*hay) == tolower((unsigned char)*needle)) {
            size_t i;
            for (i = 0; i < needle_len; ++i) {
                if (!hay[i] || tolower((unsigned char)hay[i]) != tolower((unsigned char)needle[i])) break;
            }
            if (i == needle_len) return (char*)hay;
        }
    }
    return NULL;
}

/* portable mkdir (ignore if exists) */
static void ensure_dir(const char *dname) {
#if defined(_WIN32)
    _mkdir(dname);
#else
    mkdir(dname, 0755);
#endif
}

/* ---------- INPUT BUFFER CLEAR (no warnings) ---------- */
/* Put this at top and use instead of empty-body while loops */
static void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* intentionally empty: consume characters until newline or EOF */
    }
}

/* ---------- LIBRARY MANAGEMENT ---------- */

static void add_song_to_library(const char *title, const char *artist, const char *genre) {
    if (library_count >= MAX_SONGS) return;
    strncpy(library_db[library_count].title, title, MAX_TITLE-1); library_db[library_count].title[MAX_TITLE-1] = '\0';
    strncpy(library_db[library_count].artist, artist, MAX_NAME-1); library_db[library_count].artist[MAX_NAME-1] = '\0';
    strncpy(library_db[library_count].genre, genre, MAX_GENRE-1); library_db[library_count].genre[MAX_GENRE-1] = '\0';
    library_db[library_count].liked = 0;
    library_count++;
}

/* rebuild artist/genre buckets from library */
static void build_buckets(void) {
    artist_count = genre_count = 0;
    for (int i = 0; i < library_count; ++i) {
        /* artist bucket */
        int ai = -1;
        for (int j = 0; j < artist_count; ++j) {
            if (strcmp(artist_buckets[j].name, library_db[i].artist) == 0) { ai = j; break; }
        }
        if (ai == -1) {
            if (artist_count < MAX_ARTISTS) {
                ai = artist_count;
                strncpy(artist_buckets[ai].name, library_db[i].artist, MAX_NAME-1);
                artist_buckets[ai].count = 0;
                artist_count++;
            }
        }
        if (ai >= 0 && artist_buckets[ai].count < 1200) artist_buckets[ai].indices[artist_buckets[ai].count++] = i;

        /* genre bucket */
        int gi = -1;
        for (int j = 0; j < genre_count; ++j) {
            if (strcmp(genre_buckets[j].name, library_db[i].genre) == 0) { gi = j; break; }
        }
        if (gi == -1) {
            if (genre_count < MAX_GENRES) {
                gi = genre_count;
                strncpy(genre_buckets[gi].name, library_db[i].genre, MAX_GENRE-1);
                genre_buckets[gi].count = 0;
                genre_count++;
            }
        }
        if (gi >= 0 && genre_buckets[gi].count < 1200) genre_buckets[gi].indices[genre_buckets[gi].count++] = i;
    }
}

/* ---------- PERSISTENCE ---------- */

/* save favorites to file */
static void save_favorites(void) {
    FILE *f = fopen(FAVORITES_FILE, "w");
    if (!f) return;
    for (int i = 0; i < fav_count; ++i) {
        int idx = favorites[i];
        if (idx >= 0 && idx < library_count) {
            Song *s = &library_db[idx];
            fprintf(f, "%s|%s|%s|%d\n", s->title, s->artist, s->genre, s->liked);
        }
    }
    fclose(f);
}

/* load favorites from file and match to library */
static void load_favorites(void) {
    FILE *f = fopen(FAVORITES_FILE, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char t[MAX_TITLE], a[MAX_NAME], g[MAX_GENRE];
        int liked = 0;
        char *p = strchr(line, '\n'); if (p) *p = '\0';
        char *tok = strtok(line, "|"); if (!tok) continue;
        strncpy(t, tok, MAX_TITLE-1);
        tok = strtok(NULL, "|"); if (!tok) continue;
        strncpy(a, tok, MAX_NAME-1);
        tok = strtok(NULL, "|"); if (!tok) continue;
        strncpy(g, tok, MAX_GENRE-1);
        tok = strtok(NULL, "|"); if (tok) liked = atoi(tok);
        for (int i = 0; i < library_count; ++i) {
            if (strcmp(library_db[i].title, t) == 0 && strcmp(library_db[i].artist, a) == 0) {
                int found = 0;
                for (int j = 0; j < fav_count; ++j) if (favorites[j] == i) { found = 1; break; }
                if (!found) favorites[fav_count++] = i;
                library_db[i].liked = liked;
                break;
            }
        }
    }
    fclose(f);
}

/* append user-added song to ADDED_FILE */
static void append_added_song(const Song *s) {
    FILE *f = fopen(ADDED_FILE, "a");
    if (!f) return;
    fprintf(f, "%s|%s|%s|%d\n", s->title, s->artist, s->genre, s->liked);
    fclose(f);
}

/* load persisted added songs into library */
static void load_added_songs(void) {
    FILE *f = fopen(ADDED_FILE, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char t[MAX_TITLE], a[MAX_NAME], g[MAX_GENRE];
        int liked = 0;
        char *p = strchr(line, '\n'); if (p) *p = '\0';
        char *tok = strtok(line, "|"); if (!tok) continue;
        strncpy(t, tok, MAX_TITLE-1);
        tok = strtok(NULL, "|"); if (!tok) continue;
        strncpy(a, tok, MAX_NAME-1);
        tok = strtok(NULL, "|"); if (!tok) continue;
        strncpy(g, tok, MAX_GENRE-1);
        tok = strtok(NULL, "|"); if (tok) liked = atoi(tok);
        int dup = 0;
        for (int i = 0; i < library_count; ++i) {
            if (strcmp(library_db[i].title, t) == 0 && strcmp(library_db[i].artist, a) == 0) { dup = 1; break; }
        }
        if (!dup) {
            if (library_count < MAX_SONGS) {
                strncpy(library_db[library_count].title, t, MAX_TITLE-1);
                strncpy(library_db[library_count].artist, a, MAX_NAME-1);
                strncpy(library_db[library_count].genre, g, MAX_GENRE-1);
                library_db[library_count].liked = liked;
                library_count++;
            }
        }
    }
    fclose(f);
}

/* ---------- DISPLAY HELPERS ---------- */

static void show_banner(void) {
    printf(CLR_CYAN CLR_BOLD);
    printf("============================================\n");
    printf("        SONG SUGGESTION SYSTEM (C)          \n");
    printf("============================================\n");
    printf(CLR_RESET);
    printf("User: %s\n\n", current_user);
    fflush(stdout);
}

static void spinner(const char *label, int ms, int cycles) {
    const char spin[] = "|/-\\";
    for (int c = 0; c < cycles; ++c) {
        for (int i = 0; i < 4; ++i) {
            printf("\r%s %c ", label, spin[i]); fflush(stdout);
            sleep_ms(ms);
        }
    }
    printf("\r%s done.   \n", label);
}

/* ---------- SEED LIBRARY (inside function to avoid macro misuse) ---------- */
static void seed_library(void) {
    library_count = 0;

    /* The Weeknd */
    add_song_to_library("Blinding Lights", "The Weeknd", "Pop");
    add_song_to_library("Save Your Tears", "The Weeknd", "Pop");
    add_song_to_library("I Was Never There", "The Weeknd", "R&B");
    add_song_to_library("Starboy", "The Weeknd", "Electropop");
    add_song_to_library("The Hills", "The Weeknd", "Alternative R&B");
    add_song_to_library("Can't Feel My Face", "The Weeknd", "Pop");
    add_song_to_library("After Hours", "The Weeknd", "Synthpop");
    add_song_to_library("In Your Eyes", "The Weeknd", "Pop");
    add_song_to_library("Die for You", "The Weeknd", "R&B");
    add_song_to_library("Call Out My Name", "The Weeknd", "R&B");

    /* Arijit Singh */
    add_song_to_library("Tum Hi Ho", "Arijit Singh", "Hindi Romance");
    add_song_to_library("Khairiyat", "Arijit Singh", "Hindi Romance");
    add_song_to_library("Kesariya", "Arijit Singh", "Hindi Romance");
    add_song_to_library("Channa Mereya", "Arijit Singh", "Hindi Romance");
    add_song_to_library("Phir Le Aya Dil", "Arijit Singh", "Hindi Romance");
    add_song_to_library("Agar Tum Saath Ho", "Arijit Singh", "Hindi Romance");
    add_song_to_library("Hamari Adhuri Kahani", "Arijit Singh", "Hindi Romance");
    add_song_to_library("Ae Dil Hai Mushkil", "Arijit Singh", "Hindi Romance");
    add_song_to_library("Samjhawan", "Arijit Singh", "Hindi Romance");
    add_song_to_library("Shayad", "Arijit Singh", "Hindi Romance");

    /* Taylor Swift */
    add_song_to_library("Lover", "Taylor Swift", "Pop");
    add_song_to_library("Willow", "Taylor Swift", "Indie Folk");
    add_song_to_library("Anti-Hero", "Taylor Swift", "Pop");
    add_song_to_library("Cardigan", "Taylor Swift", "Indie Folk");
    add_song_to_library("Blank Space", "Taylor Swift", "Pop");
    add_song_to_library("Style", "Taylor Swift", "Pop");
    add_song_to_library("Exile", "Taylor Swift", "Indie Folk");
    add_song_to_library("Delicate", "Taylor Swift", "Pop");
    add_song_to_library("Love Story", "Taylor Swift", "Pop");
    add_song_to_library("Wildest Dreams", "Taylor Swift", "Pop");
    add_song_to_library("Cruel Summer", "Taylor Swift", "Pop");
    add_song_to_library("Bejeweled", "Taylor Swift", "Pop");

    /* Pop / other artists */
    add_song_to_library("STAY", "The Kid LAROI & Justin Bieber", "Pop");
    add_song_to_library("Levitating", "Dua Lipa", "Pop");
    add_song_to_library("As It Was", "Harry Styles", "Pop");
    add_song_to_library("Watermelon Sugar", "Harry Styles", "Pop");
    add_song_to_library("Don't Start Now", "Dua Lipa", "Pop");
    add_song_to_library("Peaches", "Justin Bieber", "Pop");
    add_song_to_library("Good 4 U", "Olivia Rodrigo", "Pop");
    add_song_to_library("Bad Habits", "Ed Sheeran", "Pop");
    add_song_to_library("Break My Heart", "Dua Lipa", "Pop");
    add_song_to_library("Dance Monkey", "Tones and I", "Pop");

    /* R&B / alt R&B */
    add_song_to_library("Earned It", "The Weeknd", "R&B");
    add_song_to_library("Crew Love", "Drake", "R&B");
    add_song_to_library("Love Galore", "SZA", "R&B");
    add_song_to_library("Location", "Khalid", "R&B");
    add_song_to_library("Thinkin Bout You", "Frank Ocean", "R&B");
    add_song_to_library("Pink + White", "Frank Ocean", "Alternative R&B");
    add_song_to_library("Nights", "Frank Ocean", "Alternative R&B");
    add_song_to_library("Prototype", "Outkast", "R&B");

    /* Dream pop */
    add_song_to_library("K.", "Cigarettes After Sex", "Dream Pop");
    add_song_to_library("Apocalypse", "Cigarettes After Sex", "Dream Pop");
    add_song_to_library("Space Song", "Beach House", "Dream Pop");
    add_song_to_library("Myth", "Beach House", "Dream Pop");
    add_song_to_library("Norway", "Beach House", "Dream Pop");
    add_song_to_library("Heavenly", "Cigarettes After Sex", "Dream Pop");

    /* Psychedelic */
    add_song_to_library("Let It Happen", "Tame Impala", "Psychedelic");
    add_song_to_library("The Less I Know The Better", "Tame Impala", "Psychedelic");
    add_song_to_library("Feels Like We Only Go Backwards", "Tame Impala", "Psychedelic");
    add_song_to_library("Elephant", "Tame Impala", "Psychedelic");
    add_song_to_library("Borderline", "Tame Impala", "Psychedelic");

    /* Indie / extra */
    add_song_to_library("Heat Waves", "Glass Animals", "Indie");
    add_song_to_library("Circles", "Post Malone", "Pop");
    add_song_to_library("Someone You Loved", "Lewis Capaldi", "Pop");
    add_song_to_library("Sunflower", "Post Malone & Swae Lee", "Hip-Hop");
    add_song_to_library("Lost", "Frank Ocean", "Alternative R&B");
    add_song_to_library("Retrograde", "James Blake", "Electronic");
    add_song_to_library("Dark Red", "Steve Lacy", "Indie");
    add_song_to_library("Moth To A Flame", "Swedish House Mafia", "Electronic");
    add_song_to_library("Positions", "Ariana Grande", "Pop");
    add_song_to_library("Good Days", "SZA", "R&B");
    add_song_to_library("Butter", "BTS", "Pop");
    add_song_to_library("On & On", "Erykah Badu", "Neo Soul");
    add_song_to_library("Electric Feel", "MGMT", "Psychedelic");
    add_song_to_library("Chocolate", "The 1975", "Indie");
    add_song_to_library("Somebody Else", "The 1975", "Indie");
    add_song_to_library("Let Her Go", "Passenger", "Indie");
    add_song_to_library("Stressed Out", "Twenty One Pilots", "Alternative");
    add_song_to_library("Believer", "Imagine Dragons", "Rock");
    add_song_to_library("Counting Stars", "OneRepublic", "Pop");
    add_song_to_library("Holocene", "Bon Iver", "Indie Folk");
    add_song_to_library("Skinny Love", "Bon Iver", "Indie Folk");
    add_song_to_library("Breathe Me", "Sia", "Indie Pop");
    add_song_to_library("Atlas Hands", "Benjamin Francis Leftwich", "Indie");
    add_song_to_library("Work Song", "Hozier", "Indie");
    add_song_to_library("Rivers and Roads", "The Head and The Heart", "Indie Folk");
    add_song_to_library("Ho Hey", "The Lumineers", "Folk Rock");
    add_song_to_library("Fast Car", "Tracy Chapman", "Folk");

    /* more to make it large (add 20+ extras) */
    add_song_to_library("Radioactive", "Imagine Dragons", "Rock");
    add_song_to_library("Demons", "Imagine Dragons", "Rock");
    add_song_to_library("Yellow", "Coldplay", "Alternative");
    add_song_to_library("Paradise", "Coldplay", "Alternative");
    add_song_to_library("Viva La Vida", "Coldplay", "Alternative");
    add_song_to_library("Clocks", "Coldplay", "Alternative");
    add_song_to_library("Fix You", "Coldplay", "Alternative");

    add_song_to_library("Do I Wanna Know?", "Arctic Monkeys", "Indie");
    add_song_to_library("R U Mine?", "Arctic Monkeys", "Indie");
    add_song_to_library("505", "Arctic Monkeys", "Indie");

    add_song_to_library("Young and Beautiful", "Lana Del Rey", "Dream Pop");
    add_song_to_library("Video Games", "Lana Del Rey", "Dream Pop");
    add_song_to_library("Born to Die", "Lana Del Rey", "Dream Pop");
    add_song_to_library("Summertime Sadness", "Lana Del Rey", "Dream Pop");

    /* rebuild buckets */
    build_buckets();
}

/* ---------- SUGGESTION / UI / LOGIC ---------- */

/* print single song */
static void print_song(int idx) {
    if (idx < 0 || idx >= library_count) return;
    Song *s = &library_db[idx];
    printf(CLR_GREEN "%s" CLR_RESET " by " CLR_BLUE "%s" CLR_RESET " [%s]%s\n",
           s->title, s->artist, s->genre, s->liked ? CLR_RED " ♥" CLR_RESET : "");
}

/* pick random element from indices[] of length n */
static int pick_random(const int indices[], int n) {
    if (n <= 0) return -1;
    int r = rand() % n;
    return indices[r];
}

/* suggest by artist menu */
static void suggest_by_artist_menu(void) {
    if (artist_count == 0) { printf(CLR_YELLOW "No artists available.\n" CLR_RESET); return; }
    printf("\nArtists:\n");
    for (int i = 0; i < artist_count; ++i) {
        printf(" %2d) %s (%d)\n", i+1, artist_buckets[i].name, artist_buckets[i].count);
    }
    printf("Choose artist number (0 cancel): ");
    int k;
    if (scanf("%d", &k) != 1) { clearInputBuffer(); return; }
    clearInputBuffer();
    if (k <= 0 || k > artist_count) return;
    int idx = pick_random(artist_buckets[k-1].indices, artist_buckets[k-1].count);
    if (idx >= 0) {
        printf("\nSuggested:\n"); print_song(idx);
        /* add to history */
        if (history_count < MAX_HISTORY) history[history_count++] = idx;
        else { memmove(history, history+1, sizeof(int)*(MAX_HISTORY-1)); history[MAX_HISTORY-1] = idx; }
        /* ask favorite */
        printf("Add to favorites? (y/n): ");
        char ans[8];
        read_line(ans, sizeof(ans));
        if (ans[0] == 'y' || ans[0] == 'Y') {
            int already = 0;
            for (int i = 0; i < fav_count; ++i) if (favorites[i] == idx) { already = 1; break; }
            if (!already) { favorites[fav_count++] = idx; library_db[idx].liked = 1; save_favorites(); printf(CLR_GREEN "Added.\n" CLR_RESET); }
            else printf(CLR_YELLOW "Already favorite.\n" CLR_RESET);
        }
    } else printf(CLR_RED "No songs for that artist.\n" CLR_RESET);
}

/* suggest by genre menu */
static void suggest_by_genre_menu(void) {
    if (genre_count == 0) { printf(CLR_YELLOW "No genres available.\n" CLR_RESET); return; }
    printf("\nGenres:\n");
    for (int i = 0; i < genre_count; ++i) {
        printf(" %2d) %s (%d)\n", i+1, genre_buckets[i].name, genre_buckets[i].count);
    }
    printf("Choose genre number (0 cancel): ");
    int k;
    if (scanf("%d", &k) != 1) { clearInputBuffer(); return; }
    clearInputBuffer();
    if (k <= 0 || k > genre_count) return;
    int idx = pick_random(genre_buckets[k-1].indices, genre_buckets[k-1].count);
    if (idx >= 0) {
        printf("\nSuggested:\n"); print_song(idx);
        if (history_count < MAX_HISTORY) history[history_count++] = idx;
        else { memmove(history, history+1, sizeof(int)*(MAX_HISTORY-1)); history[MAX_HISTORY-1] = idx; }
        printf("Add to favorites? (y/n): ");
        char ans[8];
        read_line(ans, sizeof(ans));
        if (ans[0] == 'y' || ans[0] == 'Y') {
            int already = 0;
            for (int i = 0; i < fav_count; ++i) if (favorites[i] == idx) { already = 1; break; }
            if (!already) { favorites[fav_count++] = idx; library_db[idx].liked = 1; save_favorites(); printf(CLR_GREEN "Added.\n" CLR_RESET); }
            else printf(CLR_YELLOW "Already favorite.\n" CLR_RESET);
        }
    } else printf(CLR_RED "No songs for that genre.\n" CLR_RESET);
}

/* search menu */
static void menu_search(void) {
    char q[256];
    printf("\nSearch (title or artist): ");
    read_line(q, sizeof(q)); trim_whitespace(q);
    if (!q[0]) return;
    int results[1000]; int rc = 0;
    for (int i = 0; i < library_count && rc < 1000; ++i) {
        if (ci_strstr(library_db[i].title, q) || ci_strstr(library_db[i].artist, q)) results[rc++] = i;
    }
    if (rc == 0) { printf(CLR_YELLOW "No results.\n" CLR_RESET); return; }
    printf(CLR_BOLD "Found %d results:\n" CLR_RESET, rc);
    for (int i = 0; i < rc; ++i) { printf(" %2d) ", i+1); print_song(results[i]); }
    printf("Add result to favorites? Enter number (0 skip): ");
    int sel;
    if (scanf("%d", &sel) != 1) { clearInputBuffer(); return; }
    clearInputBuffer();
    if (sel <= 0 || sel > rc) return;
    int idx = results[sel-1];
    int already = 0;
    for (int i = 0; i < fav_count; ++i) if (favorites[i] == idx) { already = 1; break; }
    if (!already) { favorites[fav_count++] = idx; library_db[idx].liked = 1; save_favorites(); printf(CLR_GREEN "Added.\n" CLR_RESET); }
    else printf(CLR_YELLOW "Already favorite.\n" CLR_RESET);
}

/* favorites listing and actions */
static void list_favorites(void) {
    if (fav_count == 0) { printf(CLR_YELLOW "No favorites yet.\n" CLR_RESET); return; }
    printf("\nFavorites:\n");
    for (int i = 0; i < fav_count; ++i) { printf(" %2d) ", i+1); print_song(favorites[i]); }
}

static void favorites_menu(void) {
    while (1) {
        printf("\nFavorites Menu:\n 1) List favorites\n 2) Remove favorite\n 3) Export favorites to M3U\n 0) Back\nChoice: ");
        int c;
        if (scanf("%d", &c) != 1) { clearInputBuffer(); return; }
        clearInputBuffer();
        if (c == 0) return;
        if (c == 1) list_favorites();
        else if (c == 2) {
            list_favorites(); if (fav_count == 0) continue;
            printf("Enter favorite number to remove: ");
            int r; if (scanf("%d", &r) != 1) { clearInputBuffer(); continue; }
            clearInputBuffer();
            if (r < 1 || r > fav_count) { printf(CLR_RED "Invalid.\n" CLR_RESET); continue; }
            int idx = favorites[r-1];
            for (int j = r-1; j < fav_count-1; ++j) favorites[j] = favorites[j+1];
            fav_count--;
            if (idx >= 0 && idx < library_count) library_db[idx].liked = 0;
            save_favorites();
            printf(CLR_GREEN "Removed.\n" CLR_RESET);
        } else if (c == 3) {
            char fname[128];
            printf("Enter filename (myfav.m3u): ");
            read_line(fname, sizeof(fname)); trim_whitespace(fname);
            if (!fname[0]) { printf(CLR_RED "Filename required.\n" CLR_RESET); continue; }
            ensure_dir(PLAYLIST_DIR);
            char path[256]; snprintf(path, sizeof(path), "%s/%s", PLAYLIST_DIR, fname);
            FILE *f = fopen(path, "w");
            if (!f) { printf(CLR_RED "Failed to write %s: %s\n" CLR_RESET, path, strerror(errno)); continue; }
            fprintf(f, "#EXTM3U\n");
            for (int i = 0; i < fav_count; ++i) { Song *s = &library_db[favorites[i]]; fprintf(f, "#EXTINF:-1,%s - %s\n", s->artist, s->title); fprintf(f, "%s - %s\n", s->artist, s->title); }
            fclose(f); printf(CLR_GREEN "Exported to %s\n" CLR_RESET, path);
        } else printf(CLR_RED "Unknown option.\n" CLR_RESET);
    }
}

/* add song interactive */
static void add_song_interactive(void) {
    Song s;
    printf("Enter song title: "); read_line(s.title, sizeof(s.title)); trim_whitespace(s.title);
    if (!s.title[0]) { printf(CLR_RED "Title empty.\n" CLR_RESET); return; }
    printf("Enter artist name: "); read_line(s.artist, sizeof(s.artist)); trim_whitespace(s.artist);
    if (!s.artist[0]) { printf(CLR_RED "Artist empty.\n" CLR_RESET); return; }
    printf("Enter genre: "); read_line(s.genre, sizeof(s.genre)); trim_whitespace(s.genre);
    if (!s.genre[0]) { printf(CLR_RED "Genre empty.\n" CLR_RESET); return; }
    s.liked = 0;
    for (int i = 0; i < library_count; ++i) if (strcmp(library_db[i].title, s.title) == 0 && strcmp(library_db[i].artist, s.artist) == 0) { printf(CLR_YELLOW "Already exists.\n" CLR_RESET); return; }
    if (library_count >= MAX_SONGS) { printf(CLR_RED "Library full.\n" CLR_RESET); return; }
    library_db[library_count++] = s;
    append_added_song(&s);
    build_buckets();
    printf(CLR_GREEN "Added and saved.\n" CLR_RESET);
}

/* view history */
static void show_history_menu(void) {
    if (history_count == 0) { printf(CLR_YELLOW "No history.\n" CLR_RESET); return; }
    printf("\nRecently suggested:\n");
    for (int i = 0; i < history_count; ++i) { printf(" %2d) ", i+1); print_song(history[i]); }
    printf("Options: 1) Clear history  0) Back\nChoice: ");
    int c; if (scanf("%d", &c) != 1) { clearInputBuffer(); return; }
    clearInputBuffer();
    if (c == 1) { history_count = 0; printf(CLR_GREEN "Cleared.\n" CLR_RESET); }
}

/* switch user */
static void switch_user(void) {
    char uname[MAX_NAME];
    printf("Enter username: "); read_line(uname, sizeof(uname)); trim_whitespace(uname);
    if (!uname[0]) { printf(CLR_YELLOW "Canceled.\n" CLR_RESET); return; }
    strncpy(current_user, uname, MAX_NAME-1); current_user[MAX_NAME-1] = '\0';
    printf(CLR_GREEN "Switched to %s\n" CLR_RESET, current_user);
}

/* library summary */
static void show_library_summary(void) {
    printf("\nLibrary Summary: Songs=%d Artists=%d Genres=%d Favorites=%d\n", library_count, artist_count, genre_count, fav_count);
}

/* main menu */
static void main_menu(void) {
    while (1) {
        show_banner();
        printf("\nMain Menu:\n 1) Suggest by Artist\n 2) Suggest by Genre\n 3) Search\n 4) Add Song\n 5) Favorites\n 6) Export / Playlists\n 7) History\n 8) Switch User\n 9) Library Summary\n 0) Exit\nChoice: ");
        int choice;
        if (scanf("%d", &choice) != 1) { clearInputBuffer(); continue; }
        clearInputBuffer();
        switch (choice) {
            case 0: spinner("Saving favorites", 40, 6); save_favorites(); printf(CLR_GREEN "Goodbye!\n" CLR_RESET); return;
            case 1: suggest_by_artist_menu(); break;
            case 2: suggest_by_genre_menu(); break;
            case 3: menu_search(); break;
            case 4: add_song_interactive(); break;
            case 5: favorites_menu(); break;
            case 6: favorites_menu(); break;
            case 7: show_history_menu(); break;
            case 8: switch_user(); break;
            case 9: show_library_summary(); break;
            default: printf(CLR_RED "Invalid choice.\n" CLR_RESET);
        }
        printf("\nPress ENTER to continue..."); fflush(stdout);
        getchar();
    }
}

/* ---------- MAIN ---------- */

int main(void) {
    srand((unsigned int)time(NULL));
    seed_library();
    load_added_songs();
    build_buckets();
    load_favorites();
    main_menu();
    return 0;
}
