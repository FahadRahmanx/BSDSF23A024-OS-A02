/*
* Programming Assignment 02: lsv1.4.0
* Features:
*   - Default vertical (down-then-across) display
*   - -l : Long listing format
*   - -x : Horizontal (across) display
*   - -a : Include hidden files
*   - Sorted output using qsort()
*
* Usage examples:
*       $ lsv1.4.0
*       $ lsv1.4.0 -l
*       $ lsv1.4.0 -x
*       $ lsv1.4.0 -a -x /etc /home
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

void do_ls(const char *dir, int show_all);
void do_ls_long(const char *dir, int show_all);
void do_ls_horizontal(const char *dir, int show_all);
void print_permissions(mode_t mode);
int get_terminal_width(void);

// Comparison function for qsort
int compare_filenames(const void *a, const void *b)
{
    const char *fa = *(const char **)a;
    const char *fb = *(const char **)b;
    return strcmp(fa, fb);
}

enum DisplayMode { DEFAULT_MODE, LONG_MODE, HORIZONTAL_MODE };

int main(int argc, char const *argv[])
{
    int opt;
    enum DisplayMode mode = DEFAULT_MODE;
    int show_all = 0;

    // Add -a option for hidden files
    while ((opt = getopt(argc, (char * const *)argv, "lxa")) != -1)
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
            default:
                fprintf(stderr, "Usage: %s [-l | -x] [-a] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        if (mode == LONG_MODE)
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

    return 0;
}

// Default (vertical / down-then-across)
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
                printf("%-*s", col_width, filenames[idx]);
        }
        printf("\n");
    }

    for (int i = 0; i < file_count; i++)
        free(filenames[i]);
    free(filenames);
}

// Horizontal (-x)
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
        printf("%-*s", col_width, filenames[i]);
        current_width += col_width;
    }
    printf("\n");

    for (int i = 0; i < file_count; i++)
        free(filenames[i]);
    free(filenames);
}

// Long-listing (-l)
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

        printf("%s\n", filenames[i]);
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
