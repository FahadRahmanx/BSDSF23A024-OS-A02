/*
* Programming Assignment 02: lsv1.6.0
* Features:
*   - Default vertical (down-then-across) display
*   - -l : Long listing format
*   - -x : Horizontal (across) display
*   - -a : Include hidden files
*   - -R : Recursive directory listing
*   - Sorted output using qsort()
*   - Colored output based on file type
*
* Usage examples:
*       $ lsv1.6.0
*       $ lsv1.6.0 -l
*       $ lsv1.6.0 -x
*       $ lsv1.6.0 -a -x /etc /home
*       $ lsv1.6.0 -R /etc
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>

extern int errno;

/* ---------- [TASK 2: ANSI COLOR MACROS] ---------- */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;34m"   // bold blue
#define COLOR_GREEN   "\033[1;32m"   // bold green
#define COLOR_RED     "\033[1;31m"   // bold red
#define COLOR_PINK    "\033[1;35m"   // bold pink
#define COLOR_REVERSE "\033[7m"

/* ---------- Function Prototypes ---------- */
void do_ls(const char *dir, int show_all);
void do_ls_long(const char *dir, int show_all);
void do_ls_horizontal(const char *dir, int show_all);
void print_permissions(mode_t mode);
int get_terminal_width(void);
void print_colored_name(const char *dir, const char *name);

/* ---------- [TASK 3 & 4: COLORING LOGIC] ---------- */
void print_colored_name(const char *dir, const char *name) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    struct stat st;
    if (lstat(path, &st) == -1) {
        printf("%s", name);
        return;
    }

    const char *color = COLOR_RESET;

    if (S_ISDIR(st.st_mode))
        color = COLOR_BLUE;
    else if (S_ISLNK(st.st_mode))
        color = COLOR_PINK;
    else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISSOCK(st.st_mode))
        color = COLOR_REVERSE;
    else if (st.st_mode & S_IXUSR)
        color = COLOR_GREEN;
    else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip"))
        color = COLOR_RED;

    printf("%s%s%s", color, name, COLOR_RESET);
}

/* ---------- Comparison Function ---------- */
int compare_filenames(const void *a, const void *b)
{
    const char *fa = *(const char **)a;
    const char *fb = *(const char **)b;
    return strcasecmp(fa, fb);   
}

enum DisplayMode { DEFAULT_MODE, LONG_MODE, HORIZONTAL_MODE };

/* ---------- Prototype for Recursive Function ---------- */
void do_ls_recursive(const char *dir, int show_all, enum DisplayMode mode, int recursive_flag);

int main(int argc, char const *argv[])
{
    int opt;
    enum DisplayMode mode = DEFAULT_MODE;
    int show_all = 0;
    int recursive_flag = 0; // [NEW FLAG for -R]

    // Add -a and -R option for hidden and recursive files
    while ((opt = getopt(argc, (char * const *)argv, "lxaR")) != -1)
    {
        switch (opt)
        {
            case 'l':
                mode = LONG_MODE;
                break;
            case 'x':
                mode = HORIZONTAL_MODE;
                break;
            case 'a':
                show_all = 1;
                break;
            case 'R':
                recursive_flag = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l | -x] [-a] [-R] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        if (recursive_flag)
            do_ls_recursive(".", show_all, mode, recursive_flag);
        else if (mode == LONG_MODE)
            do_ls_long(".", show_all);
        else if (mode == HORIZONTAL_MODE)
            do_ls_horizontal(".", show_all);
        else
            do_ls(".", show_all);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (recursive_flag)
                do_ls_recursive(argv[i], show_all, mode, recursive_flag);
            else
            {
                printf("Directory listing of %s : \n", argv[i]);
                if (mode == LONG_MODE)
                    do_ls_long(argv[i], show_all);
                else if (mode == HORIZONTAL_MODE)
                    do_ls_horizontal(argv[i], show_all);
                else
                    do_ls(argv[i], show_all);
                puts("");
            }
        }
    }

    return 0;
}

/* ---------- Default (vertical / down-then-across) ---------- */
void do_ls(const char *dir, int show_all)
{
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    struct dirent *entry;
    char **filenames = NULL;
    int file_count = 0;
    int max_len = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (!show_all && entry->d_name[0] == '.')
            continue;

        filenames = realloc(filenames, sizeof(char *) * (file_count + 1));
        filenames[file_count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len)
            max_len = len;
        file_count++;
    }
    closedir(dp);

    if (file_count == 0)
        return;

    qsort(filenames, file_count, sizeof(char *), compare_filenames);

    int term_width = get_terminal_width();
    int spacing = 2;
    int col_width = max_len + spacing;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (file_count + num_cols - 1) / num_cols;

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_cols; col++)
        {
            int idx = col * num_rows + row;
            if (idx < file_count)
            {
                print_colored_name(dir, filenames[idx]);
                int padding = col_width - (int)strlen(filenames[idx]);
                for (int s = 0; s < padding; s++) printf(" ");
            }
        }
        printf("\n");
    }

    for (int i = 0; i < file_count; i++)
        free(filenames[i]);
    free(filenames);
}

/* ---------- Horizontal (-x) ---------- */
void do_ls_horizontal(const char *dir, int show_all)
{
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    struct dirent *entry;
    char **filenames = NULL;
    int file_count = 0;
    int max_len = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (!show_all && entry->d_name[0] == '.')
            continue;

        filenames = realloc(filenames, sizeof(char *) * (file_count + 1));
        filenames[file_count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len)
            max_len = len;
        file_count++;
    }
    closedir(dp);

    if (file_count == 0)
        return;

    qsort(filenames, file_count, sizeof(char *), compare_filenames);

    int term_width = get_terminal_width();
    int spacing = 2;
    int col_width = max_len + spacing;
    int current_width = 0;

    for (int i = 0; i < file_count; i++)
    {
        int next_width = current_width + col_width;
        if (next_width > term_width)
        {
            printf("\n");
            current_width = 0;
        }
        print_colored_name(dir, filenames[i]);
        int padding = col_width - (int)strlen(filenames[i]);
        for (int s = 0; s < padding; s++) printf(" ");
        current_width += col_width;
    }
    printf("\n");

    for (int i = 0; i < file_count; i++)
        free(filenames[i]);
    free(filenames);
}

/* ---------- Long-listing (-l) ---------- */
void do_ls_long(const char *dir, int show_all)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    char **filenames = NULL;
    int file_count = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (!show_all && entry->d_name[0] == '.')
            continue;
        filenames = realloc(filenames, sizeof(char *) * (file_count + 1));
        filenames[file_count++] = strdup(entry->d_name);
    }

    if (file_count == 0)
    {
        closedir(dp);
        return;
    }

    qsort(filenames, file_count, sizeof(char *), compare_filenames);

    long total_blocks = 0;
    for (int i = 0; i < file_count; i++)
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, filenames[i]);
        struct stat st;
        if (stat(path, &st) == -1)
            continue;
        total_blocks += st.st_blocks;
    }

    printf("total %ld\n", total_blocks / 2);

    for (int i = 0; i < file_count; i++)
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, filenames[i]);
        struct stat st;
        if (stat(path, &st) == -1)
        {
            perror("stat failed");
            continue;
        }

        print_permissions(st.st_mode);
        printf(" %3ld ", (long)st.st_nlink);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);

        printf("%-8s %-8s ", 
            pw ? pw->pw_name : "unknown", 
            gr ? gr->gr_name : "unknown");

        printf("%8ld ", (long)st.st_size);

        char timebuf[64];
        struct tm *tm_info = localtime(&st.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);
        printf("%s ", timebuf);

        print_colored_name(dir, filenames[i]);
        printf("\n");
    }

    for (int i = 0; i < file_count; i++)
        free(filenames[i]);
    free(filenames);

    closedir(dp);
}

void print_permissions(mode_t mode)
{
    if (S_ISDIR(mode))  printf("d");
    else if (S_ISLNK(mode)) printf("l");
    else printf("-");

    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

int get_terminal_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
        return 80;
    return w.ws_col;
}

/* ---------- [NEW FUNCTION: Recursive Listing for -R] ---------- */
void do_ls_recursive(const char *dir, int show_all, enum DisplayMode mode, int recursive_flag)
{
    printf("\n%s:\n", dir);

    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    struct dirent *entry;
    char **filenames = NULL;
    int file_count = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (!show_all && entry->d_name[0] == '.')
            continue;

        filenames = realloc(filenames, sizeof(char *) * (file_count + 1));
        filenames[file_count++] = strdup(entry->d_name);
    }
    closedir(dp);

    if (file_count == 0)
        return;

    qsort(filenames, file_count, sizeof(char *), compare_filenames);

    // Use existing display logic
    if (mode == LONG_MODE)
        do_ls_long(dir, show_all);
    else if (mode == HORIZONTAL_MODE)
        do_ls_horizontal(dir, show_all);
    else
        do_ls(dir, show_all);

    // Recursively visit subdirectories
    for (int i = 0; i < file_count; i++)
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, filenames[i]);
        struct stat st;
        if (lstat(path, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode) && strcmp(filenames[i], ".") != 0 && strcmp(filenames[i], "..") != 0)
            do_ls_recursive(path, show_all, mode, recursive_flag);

        free(filenames[i]);
    }
    free(filenames);
}
