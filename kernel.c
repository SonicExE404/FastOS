static unsigned short *video_memory = (unsigned short*)0xB8000;
static unsigned int cursor = 0;
void chatbot();
void update_cursor();
char scancode_to_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,
    '\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
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
    unsigned char sc;
    do {
        status = inb(0x64);
    } while ((status & 1) == 0);

    sc = inb(0x60);
    if (sc & 0x80)
        return 0;                                                  
    return scancode_to_ascii[sc];
}

int strcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 1;
        a++;
        b++;
    }
    return (*a == *b) ? 0 : 1;
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

    if (strcmp(command, "help") == 0) {
        print("\nAvailable commands:\n");
        print("  help  - show all commands\n");
        print("  echo  - print text\n");
        print("  clear - clear the screen\n");
        print("  chat - just A chatBot ^_^");
        return;
    }

    if (command[0] == 'e' && command[1] == 'c' && command[2] == 'h' && command[3] == 'o'){
        print("\n");
        print(command + 5);
        print("\n");
        return;
    };

    if (strcmp(command, "chat") == 0) {
        clear_screen();
        chatbot();
        clear_screen();
    };

    
    print("\nUnknown command\n");
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

int str_equal(const char *a, const char *b) {
    while(*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if(ca != cb) return 0;
        a++; b++;
    }
    return (*a == 0 && *b == 0);
}

int match_intent(char words[][MAX_INPUT], int word_count) {
    int best_score = 0;
    int best_index = intent_count - 1; 

    for(int i = 0; i < intent_count-1; i++) { 
        int score = 0;
        for(int w = 0; w < word_count; w++) {
            for(int k = 0; k < intents[i].keyword_count; k++) {
                if(str_equal(words[w], intents[i].keywords[k])) score++;
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
        if(str_equal(input, "exit")) {
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
