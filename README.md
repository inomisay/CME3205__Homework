# CME 3205: Multithreaded Text Analysis Server

This repository contains the C implementation of a multithreaded text analysis server, developed as an assignment for the CME 3205 Operating Systems course. The server provides spell-checking and correction functionality for text input received over a network connection.

---

## 📋 About The Project

This project is a simple yet powerful text analysis server that operates on a Linux environment. It listens for incoming connections on a specified port and, upon receiving a text string from a client like `telnet`, it performs the following actions:

1.  **Parses the input** into individual words.
2.  **Spawns a separate thread** for each word to analyze it concurrently.
3.  **Calculates the Levenshtein Distance** for each word against a pre-defined dictionary (`basic_english_2000.txt`) to find the closest matches.
4.  **Provides suggestions** for misspelled words.
5.  **Interactively learns** by allowing the user to add new, unrecognized words to the dictionary.
6.  **Returns the corrected text** to the client.

The core of the project demonstrates key operating systems concepts, including socket programming for network communication and multithreading for concurrent processing.

---

## ✨ Core Features

-   **Socket-Based Server**: Listens on port `60000` for client connections.
-   **Concurrent Analysis**: Uses pthreads to handle multiple words from the input string simultaneously, improving efficiency.
-   **Levenshtein Distance Algorithm**: Implements the classic algorithm to measure the difference between two sequences of characters and find the most likely correct spellings.
-   **Interactive Dictionary**: If a word is not found, the server prompts the user to either add it to the dictionary or replace it with the closest suggestion.
-   **Robust Error Handling**: The server gracefully handles errors such as a missing dictionary file, oversized input, and invalid characters.
-   **Persistent Learning**: The dictionary file (`basic_english_2000.txt`) is updated when a user chooses to add a new word.

---

## 🛠️ Technologies Used

-   **Language**: C
-   **Environment**: Linux
-   **Core Concepts**:
    -   Socket Programming (TCP/IP)
    -   Multithreading (pthreads)
    -   File I/O
    -   String Manipulation
    -   Levenshtein Distance Algorithm

---

## ⚙️ How to Run

### Prerequisites
1.  A Linux environment (or a virtual machine running Linux).
2.  `gcc` compiler installed.
3.  The dictionary file `basic_english_2000.txt` must be present in the same directory as the server executable. You can download it [here](https://people.sc.fsu.edu/~jburkardt/datasets/words/words.html).

### Compilation
Open your terminal and compile the source code using `gcc`. Make sure to link the pthread library.

    gcc your_source_file.c -o text_server -lpthread

### Execution
Running the application requires two separate terminal windows: one for the server and one for the client.

1.  **Start the Server**<br>
    In your first terminal, run the compiled executable. The server will start and wait for a client to connect. Keep this terminal open.

        ./text_server

2.  **Connect with a Client**<br>
    Open a **new** terminal window and use `telnet` to connect to the server on port 60000.

        telnet localhost 60000
        
Once connected, you can follow the server's prompts to send text for analysis.

---
## 🖥️ Example Interaction

Below is a sample session demonstrating how a user interacts with the server via `telnet`.

    $ telnet localhost 60000
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    Hello, this is Text Analysis Server!
    Please enter your input string:
    Hello their how are yu

    WORD 01: hello
    MATCHES: bell (2), cell (2), help (2), hill (2), hollow (2)
    WORD hello is not present in dictionary.
    Do you want to add this word to dictionary? (y/N): y

    WORD 02: their
    MATCHES: chair (2), hair (2), other (2), shear (2), tear (2)
    WORD their is not present in dictionary.
    Do you want to add this word to dictionary? (y/N): n

    WORD 03: how
    MATCHES: how (0), cow (1), low (1), now (1), show (1)

    WORD 04: are
    MATCHES: age (1), arc (1), area (1), arm (1), art (1)
    WORD are is not present in dictionary.
    Do you want to add this word to dictionary? (y/N): y

    WORD 05: yu
    MATCHES: you (1), a (2), as (2), at (2), be (2)
    WORD yu is not present in dictionary.
    Do you want to add this word to dictionary? (y/N): n

    INPUT: hello their how are yu
    OUTPUT: hello chair how are you

    Thank you for using Text Analysis Server! Good Bye!
    Connection closed by foreign host.
