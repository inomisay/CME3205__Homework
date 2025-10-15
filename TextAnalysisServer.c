#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>

#define INPUT_CHARACTER_LIMIT 30
#define TEMP_INPUT_CHARACTER_LIMIT 31
#define OUTPUT_CHARACTER_LIMIT 200
#define PORT_NUMBER 60000
#define LEVENSHTEIN_LIST_LIMIT 5
#define DICTIONARY_FILE "basic_english_2000.txt"

// Function declarations
int  levenshteinDistance(const char *word1, const char *word2);
void *processWord(void *arg);
void loadDictionary();
void addToDictionary(const char *word);
void toLowerCase(char *str);
int  isValidInput(const char *input);
void processInput(int client_fd, char *buffer);

// Global dictionary
char **dictionary = NULL;
int dictionarySize = 0;
pthread_mutex_t dictionaryMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char word[INPUT_CHARACTER_LIMIT];
    int client_fd;
    char *outputBuffer;
} ThreadArgs;

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[INPUT_CHARACTER_LIMIT];
    char originalInput[INPUT_CHARACTER_LIMIT]; // To store the unmodified input string
    // Temporary buffer to hold the input
    char tempBuffer[TEMP_INPUT_CHARACTER_LIMIT];  // Extra space for finding owerflow (101)

    // Load dictionary
    loadDictionary();

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_NUMBER);

    // Bind the socket; exit on failure with an error message.
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", PORT_NUMBER);

    // Accept client connection
    while ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) >= 0) {
        send(client_fd, "Hello, this is Text Analysis Server!\n", 38, 0);
        send(client_fd, "Please enter your input string:\n", 32, 0);        

        // Receive input
        memset(tempBuffer,0,sizeof(tempBuffer));
        read(client_fd, tempBuffer, sizeof(tempBuffer));  // Read into temporary buffer
        // Remove newline character if exists
        tempBuffer[strcspn(tempBuffer, "\n")] = 0;

        // Printing tempBuffers lenght
        printf("************\nBefor process input : %d\n",strlen(tempBuffer));
        printf("TempBuffer lenght is : ");
        processInput(client_fd,tempBuffer);
        printf("After process input : %d\n",strlen(tempBuffer));

        // Check if the input length exceeds the limit
        if (strlen(tempBuffer) > INPUT_CHARACTER_LIMIT) {
            printf("\nRecived INPUT : %s\n************\n",tempBuffer);
            char *errorMsg = "ERROR: Input string is longer than INPUT_CHARACTER_LIMIT!\n";
            send(client_fd, errorMsg, strlen(errorMsg), 0);
            close(client_fd);
            continue;  // Skip further processing
        }
        // Server checks if input contains unsupported characters.
        if (isValidInput(tempBuffer) == 0) {
            printf("\nRecived INPUT : %s\n************\n",tempBuffer);
            char *errorMsg = "ERROR: Input string contains unsupported characters!\n";
            send(client_fd, errorMsg, strlen(errorMsg), 0);
            close(client_fd);
            continue;
        }
        // Check if it is null
        if (isValidInput(tempBuffer) == 2) {
            printf("\nRecived INPUT :  NULL \n************\n");
            char *errorMsg = "ERROR: Input string is Null!\n";
            send(client_fd, errorMsg, strlen(errorMsg), 0);
            close(client_fd);
            continue;
        }

        // Copy the valid input to the main buffer
        memset(buffer, 0, sizeof(tempBuffer)-1); // The main buffer must be 100 becouse of the limitation
        strncpy(buffer, tempBuffer, INPUT_CHARACTER_LIMIT);

        // Call processInput method to process the received input
        printf("\nBuffer lenght is : ");
        processInput(client_fd,buffer);
        // Printing the Written input
        printf("\nResived INPUT   : %s\n",tempBuffer);
        // Convert the input string to lowercase
        toLowerCase(buffer); 

        // Copy the original input string for later use
        strncpy(originalInput, buffer, INPUT_CHARACTER_LIMIT);

        // For final output
        char outputBuffer[OUTPUT_CHARACTER_LIMIT] = {0}; 
        char *outputPtr = outputBuffer;

        // Tokenize input and process each word sequentially
        char *token = strtok(buffer, " ");
        pthread_t thread;
        ThreadArgs args;

        // Process each tokenized word: copy it to args, create a thread to process the word, 
        // wait for the thread to finish, append the processed word to the output buffer, and move to the next token.
        while (token != NULL) {
            strncpy(args.word, token, INPUT_CHARACTER_LIMIT - 1);
            args.word[INPUT_CHARACTER_LIMIT - 1] = '\0';
            args.client_fd = client_fd;
            args.outputBuffer = outputPtr;

            pthread_create(&thread, NULL, processWord, &args);
            pthread_join(thread, NULL); // Wait for the thread to finish

            // Append the processed word to the output buffer
            strcat(outputBuffer, args.word);
            strcat(outputBuffer, " "); // Add space between words

            // Move to the next word
            token = strtok(NULL, " ");
        }

        // Trim the trailing space from outputBuffer
        outputBuffer[strcspn(outputBuffer, "\n")] = '\0';

        // Send final input and output to client
        char finalOutput[OUTPUT_CHARACTER_LIMIT];        
        printf(  "Lowercased INPUT: %s\n",originalInput);
        printf(  "OUTPUT          : %s\n************\n", outputBuffer);
        snprintf(finalOutput, 300, "\nINPUT: %s\nOUTPUT: %s\n", originalInput, outputBuffer);
        send(client_fd, finalOutput, strlen(finalOutput), 0);

        // Send a farewell message to the client and close the connection.
        send(client_fd, "\nThank you for using Text Analysis Server! Good bye!\r\n\n", 52 + 4, 0);
        close(client_fd);
    }

    // Close the server socket and terminate the program.
    close(server_fd);
    return 0;
}

// Load dictionary from file
void loadDictionary() {
    // Open the dictionary file for reading. Exit with an error message if the file is not found.
    FILE *file = fopen(DICTIONARY_FILE, "r");
    if (!file) {
        perror("ERROR: Dictionary file not found!"); // Graceful error message
        exit(EXIT_FAILURE);
    }

    // Count the number of lines in the dictionary file to determine the size of the dictionary.
    char line[OUTPUT_CHARACTER_LIMIT];
    while (fgets(line, sizeof(line), file)) {
        dictionarySize++;
    }
    rewind(file); // Reset the file pointer to the beginning of the file for subsequent reading.

    // Allocate memory for the dictionary, storing each line as a string in the array.
    dictionary = (char **)malloc(dictionarySize * sizeof(char *));
    for (int i = 0; i < dictionarySize; i++) {
        if (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0; // Remove newline character from the line.
            dictionary[i] = strdup(line); // Duplicate the line and store it in the dictionary.
        }
    }

    // Close the dictionary file after processing.
    fclose(file);
}

// Levenshtein Distance calculation
int levenshteinDistance(const char *word1, const char *word2) {
    int length1 = strlen(word1), length2 = strlen(word2);
    int dp[length1 + 1][length2 + 1];// dp is dynamic programing 

    // Initialize base cases
    for (int i = 0; i <= length1; i++) dp[i][0] = i;
    for (int j = 0; j <= length2; j++) dp[0][j] = j;

    // Fill DP table
    for (int i = 1; i <= length1; i++) {
        for (int j = 1; j <= length2; j++) {
            int cost = (word1[i - 1] == word2[j - 1]) ? 0 : 1;
            dp[i][j] = fmin(dp[i - 1][j] + 1,          // Deletion
                        fmin(dp[i][j - 1] + 1,         // Insertion
                             dp[i - 1][j - 1] + cost)); // Substitution
        }
    }
    return dp[length1][length2];
}

// Process a word (thread function)
void *processWord(void *arg) {
    // Extract the arguments passed to the thread, including the word to process and the client file descriptor.
    ThreadArgs *args = (ThreadArgs *)arg;
    char *word = args->word;
    int client_fd = args->client_fd;

    // Initialize an array to store the Levenshtein distances between the given word and each word in the dictionary.
    int distances[dictionarySize];
    for (int i = 0; i < dictionarySize; i++) {
        // Calculate and store the Levenshtein distance between the input word and the current dictionary word.
        distances[i] = levenshteinDistance(word, dictionary[i]);
    }

    // Find closest matches
    // Initialize arrays to store the closest words and their corresponding Levenshtein distances.
    char closest[LEVENSHTEIN_LIST_LIMIT][OUTPUT_CHARACTER_LIMIT];
    int closestDistances[LEVENSHTEIN_LIST_LIMIT];
    for (int i = 0; i < LEVENSHTEIN_LIST_LIMIT; i++) {
        closestDistances[i] = INT_MAX; // Set initial distances to maximum value.
    }

    // Find the closest matches by comparing each dictionary word's distance.
    for (int i = 0; i < dictionarySize; i++) {
        for (int j = 0; j < LEVENSHTEIN_LIST_LIMIT; j++) {

            // If the current word has a smaller distance than the one in the list, insert it.
            if (distances[i] < closestDistances[j]) {

                // Shift existing entries down to make space for the new closer word.
                for (int k = LEVENSHTEIN_LIST_LIMIT - 1; k > j; k--) {
                    closestDistances[k] = closestDistances[k - 1];
                    strcpy(closest[k], closest[k - 1]);
                }

                // Update the closest word and its distance in the list.
                closestDistances[j] = distances[i];
                strcpy(closest[j], dictionary[i]);
                break;
            }
        }
    }

    // If the word is not in the dictionary, prompt the user
    // Check if the word exists in the dictionary by comparing it case-insensitively with each entry.
    char response[OUTPUT_CHARACTER_LIMIT] = "";
    int found = 0;
    for (int i = 0; i < dictionarySize; i++) {
        if (strcasecmp(word, dictionary[i]) == 0) {
            found = 1; // Mark the word as found if there's a match.
            break;
        }
    }

    // Construct a response message starting with the input word.
    snprintf(response, sizeof(response), "\nWORD: %s\nMATCHES: ", word);

    // Append the closest matches and their Levenshtein distances to the response.
    for (int i = 0; i < LEVENSHTEIN_LIST_LIMIT; i++) {
        char match[OUTPUT_CHARACTER_LIMIT];
        snprintf(match, sizeof(match), "%s (%d)", closest[i], closestDistances[i]);
        strcat(response, match); // Add the match to the response.
        if (i < LEVENSHTEIN_LIST_LIMIT - 1) strcat(response, ", "); // Add a comma between matches.
    }
    strcat(response, "\n"); // Add a newline at the end of the response.

    // Send the constructed response message to the client.
    send(client_fd, response, strlen(response), 0);

    // If the word is not found in the dictionary, prompt the user to add it.
    if (!found) {
        snprintf(response, sizeof(response), "\nWORD %s is not present in dictionary. Do you want to add this word to dictionary? (y/N): ", word);
        send(client_fd, response, strlen(response), 0);

        // Wait for user response
        char user_response[10];
        memset(user_response, 0, sizeof(user_response)); // Initialize the response buffer.
        int bytes_read = read(client_fd, user_response, sizeof(user_response) - 1); // Read the user's input.
        if (bytes_read <= 0) {
            perror("Error reading user response"); // Handle read error or client disconnection.
            pthread_exit(NULL); // Exit the thread.
        }
        user_response[bytes_read] = '\0'; // Null-terminate the response

        // Check if the user wants to add the word to the dictionary.
        if (tolower(user_response[0]) == 'y') {
            addToDictionary(word); // Add the word to the dictionary.
            send(client_fd, "Word added to dictionary.\n", 26, 0);
        } else {
            // Replace the word with the closest match and notify the user.
            snprintf(response, sizeof(response), "Replaced with: %s\n", closest[0]);
            strcpy(word, closest[0]); // Replace with the closest match
            send(client_fd, response, strlen(response), 0);
        }
    }

    // Exit the thread after processing.
    pthread_exit(NULL);
}

// Add a word to the dictionary
void addToDictionary(const char *word) {
    
     pthread_mutex_lock(&dictionaryMutex); // Lock the mutex for thread safety

    // Find the correct position to insert the new word
    int position = 0;
    while (position < dictionarySize && strcmp(dictionary[position], word) < 0) {
        position++;
    }

    // Increment dictionary size and reallocate memory
    dictionarySize++;
    dictionary = realloc(dictionary, dictionarySize * sizeof(char *));

    // Shift the words after the insertion point to make space for the new word
    for (int i = dictionarySize - 1; i > position; i--) {
        dictionary[i] = dictionary[i - 1];
    }

    // Insert the new word at the correct position
    dictionary[position] = strdup(word);

    // Rewrite the dictionary file to maintain alphabetical order
    FILE *file = fopen(DICTIONARY_FILE, "w");
    if (file) {
        for (int i = 0; i < dictionarySize; i++) {
            fprintf(file, "%s\n", dictionary[i]);
        }
        fclose(file);
    } else {
        perror("ERROR: Unable to write to dictionary file!"); // Handle file opening failure
    }

    pthread_mutex_unlock(&dictionaryMutex); // Unlock the mutex
}

// A helper function to check input validity
int isValidInput(const char *input) {

    // If the written input is null
    if(input == NULL || input[0] == '\0'){
        return 2;
    }
    // Find first charachter after spaces
    int a = 0;
    while (isspace(input[a])) {
        a++;
    }

    // Check if the input only contain spaces or there is charachter
    if (input[a] == '\0') {
        return 0; // Invalid character found
    }

    for (int i = 0; input[i] != '\0'; i++) {
        if (!isalpha(input[i]) && !isspace(input[i])) {
            return 0; // Invalid character found
        }
    }
    return 1; // All characters are valid
}

// Change to LoweCase 
void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

// Process input by removing '\r' and other necessary changes
void processInput(int client_fd, char *buffer) {
    char *ptr = buffer;
    int flag = 0;
    while (*ptr) {
        if (*ptr == '\r') {
            flag = 1;
            memmove(ptr, ptr + 1, strlen(ptr));
        } else {
            ptr++;
        }
    }
    
    // Process the cleaned buffer as usual
    int b = strlen(buffer);
    printf("%d\n", b);
    if (flag == 1 ){
        printf("The string Contain : 'stash r'\n");
    }
    send(client_fd, "Response: OK\r\n", 15, 0);
}

// cd /home/sara/Desktop/HW/
//gcc copy.c -o copy -pthread -lm
//./copy
//telnet 127.0.0.1 60000
//nc 127.0.0.1 60000
//This is a test string that exceeds the maximum allowed character limit by a significant margin and it will trigger an error