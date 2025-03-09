#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <sys/types.h>
#define _GNU_SOURCE      
#include <dirent.h>
#define QUOTE_DIR "../categories"
#define COLOR_RED "\x1b[31m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_RESET "\x1b[0m"
#define IMG_CACHE_DIR "/tmp/aniqt_cache"
#define ANILIST_API "https://graphql.anilist.co"

typedef struct {
    char name[50];
    char path[256];
    int score;
} ImageMatch;

// Updated memory struct and callback
struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

// this new function to extract image URL from response
char *extract_image_url(const char *json_str) {
    static char image_url[512];
    char *data_start = strstr(json_str, "\"large\":\"");
    
    if (data_start) {
        data_start += 9; // Skip "large":"
        char *end = strchr(data_start, '"');
        if (end) {
            size_t len = end - data_start;
            if (len < sizeof(image_url)) {
                strncpy(image_url, data_start, len);
                image_url[len] = '\0';
                return image_url;
            }
        }
    }
    return NULL;
}

char *fetch_author_image(const char *author_name) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};
    static char image_url[512] = {0};
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl = curl_easy_init();
    if (curl) {
        // GraphQL query
        char query[1024];
        snprintf(query, sizeof(query),
                "{\"query\":\"query{Character(search:\\\"%s\"){name{full}image{large}}}\"}",
                author_name);
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, ANILIST_API);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        
        res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            char *img_url = extract_image_url(chunk.memory);
            if (img_url) {
                strncpy(image_url, img_url, sizeof(image_url) - 1);
            }
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    free(chunk.memory);
    return image_url[0] ? image_url : NULL;
}

// note this is still non functional ._.
void show_author_image(const char *author) {
    mkdir(IMG_CACHE_DIR, 0755);
    
    char cache_path[1024];
    snprintf(cache_path, sizeof(cache_path), "%s/%s.jpg", IMG_CACHE_DIR, author);
    
    // Check if image is already cached
    if (access(cache_path, F_OK) != 0) {
        char *image_url = fetch_author_image(author);
        if (image_url) {
            // download and cache the image
            CURL *curl = curl_easy_init();
            if (curl) {
                FILE *fp = fopen(cache_path, "wb");
                if (fp) {
                    curl_easy_setopt(curl, CURLOPT_URL, image_url);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                    curl_easy_perform(curl);
                    fclose(fp);
                }
                curl_easy_cleanup(curl);
            }
        }
    }
    
    // Try to display the image if it exists
    if (access(cache_path, F_OK) == 0) {
        // Try different terminal image viewers in order of preference
        const char *viewers[] = {
            "kitty icat --align left",
            "catimg",
            "timg",
            "chafa"
        };
        
        for (size_t i = 0; i < sizeof(viewers) / sizeof(viewers[0]); i++) {
            char cmd[2048];
            snprintf(cmd, sizeof(cmd), "which %s >/dev/null 2>&1", 
                    strtok(strdup(viewers[i]), " "));
            
            if (system(cmd) == 0) {
                snprintf(cmd, sizeof(cmd), "%s %s", viewers[i], cache_path);
                if (system(cmd) == 0) break;
            }
        }
    }
}

void normalize_name(char *str) {
    int j = 0;
    for (int i = 0; str[i]; i++) {
        if (isalnum(str[i]) || str[i] == ' ') {
            str[j++] = tolower(str[i]);
        }
    }
    str[j] = '\0';
}

void display_quote(const char *quote, const char *author) {
    printf("\033[2J\033[H");  // Clear screen
    
    show_author_image(author);
    int padding = 40;  
    int max_line_length = 50;  
    printf("\033[5;%dH", padding);  
    printf("\033[3m"); 

    int line_length = 0;
    for (int i = 0; quote[i]; i++) {
        putchar(quote[i]);
        line_length++;

        if (line_length >= max_line_length && quote[i] == ' ') {
            printf("\n\033[%dC", padding);  
            line_length = 0;
        }

        if (quote[i] == '.' || quote[i] == '!') usleep(100000);  // Pause for effect
        fflush(stdout);
    }
    printf(COLOR_RESET "\n\n" COLOR_CYAN "~ " COLOR_RED);

    // to print the author's name right under the quote ^^
    printf("\033[7;%dH", padding);  // just to move cursor to row 7, column padding
    for (int i = 0; author[i]; i++) {
        putchar(author[i]);
        usleep(50000);  
        fflush(stdout);
    }
    printf(COLOR_RESET "\n\n");
}

void get_random_quote(const char *category) {
    char path[256], line[512], *quote, *author;
    snprintf(path, sizeof(path), "%s/%s.txt", QUOTE_DIR, category);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        printf(COLOR_RED "\nCategory not found! Try -l to list\n" COLOR_RESET);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        count++;
    }
    int chosen = rand() % count;
    
    fseek(fp, 0, SEEK_SET);
    for (int i = 0; fgets(line, sizeof(line), fp); i++) {
        if (i == chosen) break;
    }

    quote = strtok(line, "|");
    author = strtok(NULL, "\n");
    while (author && (*author == ' ' || *author == '-')) author++;
    
    display_quote(quote, author);
    fclose(fp);
}

void list_categories() {
    // List of anime-related categories >_<
    const char *categories[] = {
        "shonen",       // Popular shonen anime quotes
        "shoujo",       // Romantic and emotional quotes
        "isekai",       // Quotes from isekai anime
        "mecha",        // Mecha anime quotes (e.g., Gundam, Evangelion)
        "slice_of_life", // Heartwarming slice-of-life quotes
        "fantasy",      // Fantasy anime quotes
        "sci_fi",       // Sci-fi anime quotes
        "horror",       // Dark and spooky anime quotes
        "comedy",       // Funny and lighthearted quotes
        "drama",        // Deep and emotional quotes
        "sports",       // Motivational sports anime quotes
        "villains",     // Iconic villain quotes
        "heroes",       // Inspirational hero quotes
        "philosophy",   // Thought-provoking quotes
        "nostalgia",    // Quotes from classic anime
        "romance",      // Romantic and heartfelt quotes
        "action",       // High-energy action quotes
        "adventure",    // Adventure-themed quotes
        "magic",        // Quotes about magic and wonder
        "friendship",   // Quotes about friendship
        "determination",// Quotes about perseverance
        "sadness",      // Emotional and tear-jerking quotes
        "hope",         // Quotes about hope and optimism
        "revenge",      // Quotes about revenge
        "sacrifice",    // Quotes about sacrifice
        "growth",       // Quotes about personal growth
        "legacy",       // Quotes about leaving a legacy
        "dreams",       // Quotes about dreams and aspirations
        "war",          // Quotes about war and conflict
        "peace"         // Quotes about peace and harmony
    };

    int num_categories = sizeof(categories) / sizeof(categories[0]);

    printf(COLOR_CYAN "Available categories:\n" COLOR_RESET);
    for (int i = 0; i < num_categories; i++) {
        printf("  %s\n", categories[i]);
    }
}

// help page lol
void show_help_page() {
    printf(COLOR_CYAN);
    printf("     _    _   _ ___ ___ _____ \n");
    printf("    / \\  | \\ | |_ _/ _ \\_   _|\n");
    printf("   / _ \\ |  \\| || | | | || |  \n");
    printf("  / ___ \\| |\\  || | |_| || |  \n");
    printf(" /_/   \\_\\_| \\_|___\\__\\_\\|_|  \n");
    printf(COLOR_RESET);
    
    printf(COLOR_CYAN "\nWelcome to ANIQT - Anime Quote Terminal\n\n" COLOR_RESET);
    
    printf(COLOR_CYAN "Usage: " COLOR_RESET "aniqt [category] [-options]\n\n");
    
    printf(COLOR_CYAN "Options:\n" COLOR_RESET);
    printf("  -h  Show this help message\n");
    printf("  -l  List available categories\n");
    printf("  -r  Random category\n\n");
    
    printf(COLOR_MAGENTA "Created with " COLOR_RED "❤️ " COLOR_MAGENTA "by " COLOR_CYAN "Hinoko77" COLOR_MAGENTA "(@GitHub)\n" COLOR_RESET);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    system("clear");

    if (argc < 2 || !strcmp(argv[1], "-h")) {
       show_help_page(); 
        return 0;
    }
    
    if (!strcmp(argv[1], "-l")) {
        list_categories();
        return 0;
    }
    
    if (!strcmp(argv[1], "-r")) {
        struct dirent **files;
        int count = scandir(QUOTE_DIR, &files, NULL, alphasort);
        if (count < 3) return 1;  // Skip . and ..
        get_random_quote(files[2 + rand() % (count - 2)]->d_name);
        return 0;
    }

    get_random_quote(argv[1]);
    return 0;
}
