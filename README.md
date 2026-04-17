# 🎵 Song Suggestion System

### ⚡ A Structured C-Based Music Recommendation Engine

<p align="center">
  <img src="images/banner.png" alt="Project Banner" width="100%">
</p>

---

## 📌 Overview

The **Song Suggestion System** is a command-line based application built in **C**, designed to simulate a lightweight music recommendation engine. It allows users to search, filter, and organize songs efficiently while demonstrating strong fundamentals of **data structures, file handling, and algorithmic thinking**.

This project goes beyond basic C programs by integrating **real-world system design concepts** like categorization, persistence, and user interaction.

---

## 🚀 Key Highlights

* 🔍 Intelligent **song search system** (case-insensitive)
* 🎧 Dynamic **song suggestions by artist & genre**
* ❤️ Persistent **favorites system**
* 📜 **History tracking** for user activity
* 📁 Exportable **playlist feature**
* ⚙️ Efficient **data organization using buckets**
* 💾 File-based storage for real-world simulation

---

## 🧠 Core Concepts Demonstrated

* Structured Programming using `struct`
* Array-based database design
* File handling (`fopen`, `fwrite`, `fread`)
* String processing & custom search logic
* Categorization using indexing (bucket system)
* CLI-based user interaction

---

## 🛠️ Tech Stack

| Category    | Technology           |
| ----------- | -------------------- |
| Language    | C (C11 Standard)     |
| Environment | CLI (Terminal-based) |
| Storage     | Text Files (`.txt`)  |
| Concepts    | DSA + File Handling  |

---

## 📂 Project Structure

```bash
Song-Suggestion-System/
│
├── song_suggester_clean_mac.c   # Main program
├── favorites.txt                # Stores liked songs
├── added_songs.txt              # User-added songs
├── playlists/                   # Exported playlists
├── images/                      # Screenshots (for README)
└── README.md
```

---

## ⚙️ Installation & Setup

### 1️⃣ Clone Repository

```bash
git clone https://github.com/your-username/song-suggestion-system.git
cd song-suggestion-system
```

### 2️⃣ Compile Code

```bash
gcc -std=c11 -Wall -Wextra song_suggester_clean_mac.c -o song_suggester
```

### 3️⃣ Run Program

```bash
./song_suggester
```

---

## 📸 Screenshots

> Add these after running your project

```
images/
├── menu.png
├── search.png
├── suggestions.png
└── playlist.png
```

---

## 🔍 Features Breakdown

### 🎧 Song Library

* Preloaded dataset
* Dynamic addition of songs
* Persistent storage across sessions

### 🔎 Search System

* Case-insensitive matching
* Multi-field search:

  * Title
  * Artist
  * Genre

### 🧠 Recommendation Engine

* Suggests songs based on:

  * Artist similarity
  * Genre grouping
* Optimized using bucket indexing

### ❤️ Favorites

* Mark/unmark songs
* Stored in `favorites.txt`

### 📜 History Tracking

* Tracks previously accessed songs
* Improves navigation experience

### 📁 Playlist Export

* Generate playlists
* Stored in `/playlists` directory

---

## ⚡ How It Works (Under the Hood)

* Songs stored in a **global array database**
* **Buckets** group songs by artist & genre
* Custom **string comparison logic** ensures flexible search
* Index-based tracking for:

  * Favorites
  * History
* File I/O enables **data persistence**

---

## 🎯 Use Cases

* 📚 Academic mini-project (C / DSA)
* 💼 Resume project for showcasing core programming skills
* 🧪 Practice for system design fundamentals
* 🎯 Understanding real-world data handling in low-level languages

---

## 📊 Why This Project Stands Out

Unlike basic CRUD C programs, this project:

* Simulates a **real recommendation system**
* Combines **DSA + system design**
* Implements **persistent storage**
* Provides a **complete user flow**

---

## 🔮 Future Enhancements

* 🎯 Ranking-based recommendations
* 🌐 Integration with music APIs
* 🖥️ GUI version (using GTK / Web frontend)
* 📊 Advanced filtering & sorting
* ⚡ Dynamic memory allocation (scalable system)

---

## ⚠️ Limitations

* CLI-only interface
* Fixed-size arrays (limited scalability)
* No real-time streaming integration

---

## 🤝 Contributing

Contributions are welcome!

```bash
# Fork the repo
# Create your branch
git checkout -b feature-name

# Commit changes
git commit -m "Added new feature"

# Push
git push origin feature-name
```

---

## 📜 License

This project is open-source and free to use for educational purposes.

---

## 👨‍💻 Author

Developed as part of hands-on practice in **C programming, data structures, and system-level problem solving**.

---

## ⭐ Final Note

This project reflects the transition from **basic coding → building systems**.
It’s not just about syntax — it’s about **thinking like a developer**.

