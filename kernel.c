static unsigned short *video_memory = (unsigned short*)0xB8000;
static unsigned int cursor = 0;
void chatbot();
void update_cursor();
#define MIN(a, b) ({ \
    unsigned int _a = (unsigned int)(a); \
    unsigned int _b = (unsigned int)(b); \
    _a < _b ? _a : _b; \
})

#define MAX(a, b) ({ \
    unsigned int _a = (unsigned int)(a); \
    unsigned int _b = (unsigned int)(b); \
    _a > _b ? _a : _b; \
})
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// Scancodes for the Shift keys (Make codes)
#define LEFT_SHIFT_MAKE 0x2A
#define RIGHT_SHIFT_MAKE 0x36
// Scancodes for the Shift keys (Break codes)
#define LEFT_SHIFT_BREAK (LEFT_SHIFT_MAKE | 0x80)
#define RIGHT_SHIFT_BREAK (RIGHT_SHIFT_MAKE | 0x80)
//global shift state
static int is_shift_pressed = 0;
char scancode_to_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,
    '\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
};
char scancode_to_ascii_shift[128] = {
    // 0, Esc, ! , @, #, $, %, ^, &, *, (, ), _, +, Backspace, Tab
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b', '\t',
    // Q, W, E, R, T, Y, U, I, O, P, {, }, Enter, Ctrl
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    // A, S, D, F, G, H, J, K, L, :, ", ~, 0
    'A','S','D','F','G','H','J','K','L',':','\"','~',0,
    // |, Z, X, C, V, B, N, M, <, >, ?, 0, *, 0, Space
    '|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',
};
void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
void clear_screen() {
    for (int i = 0; i < 80*25; i++) {
        video_memory[i] = (0x0F << 8) | ' ';
    }
    cursor = 0;
    update_cursor();
}

void update_cursor() {
    unsigned short pos = cursor;

    outb(0x3D4, 14); 
    outb(0x3D5, (pos >> 8) & 0xFF);

    outb(0x3D4, 15);
    outb(0x3D5, pos & 0xFF);
}
void print(const char *s){
    while(*s){
    if (*s == '\n') {
        cursor = (cursor / 80 + 1) * 80;
    } else {
        video_memory[cursor++] = (0x0F << 8) | *s;
        update_cursor();
    }
    s++;
    }
};
unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
};
char get_key() {
    unsigned char status;
    unsigned char scancode; // Renamed 'sc' to 'scancode' for clarity

    // Wait for the keyboard buffer to be ready (bit 0 is set)
    do {
        status = inb(KEYBOARD_STATUS_PORT);
    } while ((status & 1) == 0);

    // Read the scancode from the data port
    scancode = inb(KEYBOARD_DATA_PORT);

    // --- Shift Key Handling ---

    // Check if the scancode is for pressing LEFT_SHIFT
    if (scancode == LEFT_SHIFT_MAKE || scancode == RIGHT_SHIFT_MAKE) {
        is_shift_pressed = 1;
        return 0; // Return 0 to indicate a non-printable key press
    }

    // Check if the scancode is for releasing LEFT_SHIFT (Break Code)
    else if (scancode == LEFT_SHIFT_BREAK || scancode == RIGHT_SHIFT_BREAK) {
        is_shift_pressed = 0;
        return 0; // Return 0 to indicate a non-printable key release
    }

    // --- Printable Key Handling ---

    // Check for Break Code (Key Release) for a printable key
    // Break codes have the MSB set (scancode & 0x80)
    if (scancode & 0x80) {
        // We only care about key presses for character input
        return 0;
    }

    // Get the ASCII character based on the current Shift state
    if (is_shift_pressed) {
        return scancode_to_ascii_shift[scancode];
    } else {
        return scancode_to_ascii[scancode];
    }
}

unsigned int strlen(const char *s) {
    unsigned int len = 0;
    while (*s != '\0') {
        len++;
        s++;
    }
    return len;
}

int strncmp(const char *s1, const char *s2, unsigned int n) {
    while (n > 0 && *s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }

    if (n == 0) {
        return 0;
    } else {
        return *(const unsigned char *)s1 - *(const unsigned char *)s2;
    }
}

// It returns the standard <0, 0, or >0.
int strcmp(const char *a, const char *b) {
    unsigned int len_a = strlen(a);
    unsigned int len_b = strlen(b);

    // Determine the length of the common prefix (the shorter length)
    unsigned int common_len = MIN(len_a, len_b);

    // Call strncmp to compare the strings up to the shorter length
    int result = strncmp(a, b, common_len);

    // If strncmp found a difference, return that result immediately.
    if (result != 0) {
        return result;
    }

    // If strncmp found no difference (result == 0), the strings only differ in length.
    // The shorter string is lexicographically "less than" the longer one.
    
    // Example: strcmp("apple", "apples")
    // common_len = 5. strncmp returns 0.
    // len_a = 5, len_b = 6. 
    // len_a - len_b = 5 - 6 = -1 (Negative: "apple" < "apples")
    
    return (int)(len_a - len_b);
}

void execute_command(char *command) {

    if (strcmp(command, "clear") == 0) {
        for (int i = 0; i < 80*25; i++) {
            video_memory[i] = (0x0F << 8) | ' ';
            update_cursor();
        }
        cursor = 0;
        return;
    }
    else if (strcmp(command, "help") == 0) {
        print("\nAvailable commands:\n");
        print("  help  - show all commands\n");
        print("  echo  - print text\n");
        print("  clear - clear the screen\n");
        print("  chat - just A chatBot ^_^");
        return;
    }
    else if (strncmp(command, "echo", 4) == 0) {
        // If the first 4 characters match "echo", we proceed with the command.
    
        // Note: command + 5 skips the "echo " part (4 chars for "echo" + 1 char for space).
        print("\n");
        print(command + 5); 
        print("\n");
        return;
    }
    else if (strcmp(command, "chat") == 0) {
        clear_screen();
        chatbot();
        clear_screen();
    }
    else print("\nUnknown command\n");
};
void read_line(char *buffer) {
    int i = 0;
    char c;

    while (1) {
        c = get_key();
        if (c == 0) continue;
        if (c == '\n') break;
        else if (c == '\b') {
            if (i > 0) {
                i--;
                if (cursor > 0) cursor--;
                video_memory[cursor] = (0x0F << 8) | ' ';
                update_cursor();
            }
        }else { 
            buffer[i++] = c;
            video_memory[cursor++] = (0x0F << 8) | c;
            update_cursor();
        }
    }
    buffer[i] = '\0';                 
};

#define MAX_KEYWORDS 10
#define MAX_INPUT 64

typedef struct {
    const char *name;
    const char *keywords[MAX_KEYWORDS];
    int keyword_count;
    const char *response;
} Intent;

Intent intents[] = {
    {"greeting", {"hello", "hi", "hey"}, 3, "Hello! How can I help you today?\n"},
    {"os", {"os", "system", "operating"}, 3, "of course the best OS is FastOS.\n"},
    {"what",{"wait","hear","understand"},3, "What??\n"},
    {"default", {}, 0, "I don't understand that yet, but I'm learning!\n"}
};
int intent_count = sizeof(intents)/sizeof(Intent);

int match_intent(char words[][MAX_INPUT], int word_count) {
    int best_score = 0;
    int best_index = intent_count - 1; 

    for(int i = 0; i < intent_count-1; i++) { 
        int score = 0;
        for(int w = 0; w < word_count; w++) {
            for(int k = 0; k < intents[i].keyword_count; k++) {
                if(strcmp(words[w], intents[i].keywords[k]) == 0) score++;
            }
        }
        if(score > best_score) {
            best_score = score;
            best_index = i;
        }
    }
    return best_index;
}

int split_words(char *input, char words[][MAX_INPUT]) {
    int count = 0;
    char *p = input;
    while(*p) {
        while(*p == ' ') p++;
        if(*p == 0) break;

        int i = 0;
        while(*p && *p != ' ' && i < MAX_INPUT-1) {
            words[count][i++] = *p++;
        }
        words[count][i] = 0;
        count++;
    }
    return count;
}


void chatbot() {
    char input[MAX_INPUT];
    char words[10][MAX_INPUT]; 
    // thnx to the AI to turn my chatBot from python file to c ^_^
    print(" FastChat is ready! Type 'exit' to exit.\n");

    while(1) {
        print("you: ");
        read_line(input);
        if(strcmp(input, "exit") == 0) {
            print("Exiting chat...\n");
            break;  
        }
        int word_count = split_words(input, words);
        int intent_index = match_intent(words, word_count);
        print("\nFastChat: ");
        print(intents[intent_index].response);
        print("\n");
    };
};

char input[60];
void shell() {
    while (1 == 1) {
        print("\nFastOS> "); 
        read_line(input); 
        execute_command(input);  
    }
};

void kmain() {
    print("Welcome to FastOS !\n");
    shell();
}
